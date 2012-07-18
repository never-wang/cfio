/****************************************************************************
 *       Filename:  id.c
 *
 *    Description:  manage client's id assign, and server's id map, include
 *		    ncid, varid, dimid
 *
 *        Version:  1.0
 *        Created:  04/27/2012 02:50:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>

#include "id.h"
#include "debug.h"
#include "times.h"
#include "quickhash.h"

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
#define final(a, b, c)		    \
    do{				    \
	c ^= b; c -= rot(b, 14);    \
	a ^= c; a -= rot(c, 11);    \
	b ^= a; b -= rot(a, 25);    \
	c ^= b; c -= rot(b, 16);    \
	a ^= c; a -= rot(c,  4);    \
	b ^= a; b -= rot(a, 14);    \
	c ^= b; c -= rot(b, 24);    \
    } while (0)
#define mix(a, b, c)			    \
    do{					    \
	a -= c; a ^= rot(c,  4); c += b;    \
	b -= a; b ^= rot(a,  6); a += c;    \
	c -= b; c ^= rot(b,  8); b += a;    \
	a -= c; a ^= rot(c, 16); c += b;    \
	b -= a; b ^= rot(a, 19); a += c;    \
	c -= b; c ^= rot(b,  4); b += a;    \
    } while (0)

static int open_nc_a;  /* amount of opened nc file */
static iofw_nc_t *open_nc;
static struct qhash_table *map_table;

static int _compare(void *key, struct qhash_head *link)
{
    assert(NULL != key);
    assert(NULL != link);

    iofw_id_val_t *val = qlist_entry(link, iofw_id_val_t, hash_link);

    if(0 == memcmp(key, val, sizeof(iofw_id_key_t)))
    {
        return 1;
    }

    return 0;
}

static int _hash(void *key, int table_size)
{
    iofw_id_key_t *_key = key;
    int a, b, c;

    debug(DEBUG_ID, "(%d, %d, %d)", _key->client_nc_id,
	    _key->client_dim_id, _key->client_var_id);
    
    a = _key->client_nc_id; b = _key->client_dim_id; c = _key->client_var_id;
    final(a, b, c);

    int h = (int)(c & (table_size - 1));

    debug(DEBUG_ID, "Hash = %d", h);
    return h;
}

static void _val_free(iofw_id_val_t *val)
{
    if(NULL != val)
    {
	if(NULL != val->var)
	{
	    if(NULL != val->var->dimsids)
	    {
		free(val->var->dimsids);
		val->var->dimsids = NULL;
	    }
	    if(NULL != val->var->data)
	    {
		free(val->var->data);
		val->var->dimsids = NULL;
	    }
	    free(val->var);
	    val->var = NULL;
	}
	free(val);
	val = NULL;
    }
}

static void _inc_src_index(
	const int ndims, const size_t ele_size, 
	const size_t *dst_dims_len, char **dst_addr, 
	const size_t *src_dims_len, size_t *src_index)
{
    int dim;
    size_t sub_size, last_sub_size;

    dim = 0;
    src_index[dim] ++;

    sub_size = ele_size;
    while(src_index[dim] >= src_dims_len[dim])
    {
	*dst_addr -= src_dims_len[dim] * sub_size;
	src_index[dim] = 0;
	sub_size *= dst_dims_len[dim];
	dim ++;
	src_index[dim] ++;
    }

    *dst_addr += sub_size;
}

/**
 * @brief: put the src data array into the dst data array
 *
 * @param ndims: number of dimensions for the variable
 * @param ele_size: size of each element in the variable array
 * @param dst_dims_len: length of each dimemsion fo the variable
 * @param dst_data: pointer to the dst data array
 * @param src_start: start index of the src data array in the dst data array
 * @param src_count: count of teh src data array
 * @param src_data: pointer to the src data array
 */
static void _put_var(
	int ndims, size_t ele_size,
	size_t *dst_dims_len, char *dst_data, 
	size_t *src_start, size_t *src_count,
	char *src_data)
{
    size_t i;
    size_t src_len;
    size_t dst_start, sub_size;
    size_t *src_index;

    assert(NULL != dst_dims_len);
    assert(NULL != dst_data);
    assert(NULL != src_start);
    assert(NULL != src_count);
    assert(NULL != src_data);

    src_len = 1;
    for(i = 0; i < ndims; i ++)
    {
	src_len * = src_count[i];
    }

    src_index = malloc(sizeof(size_t) * ndims);
    if(NULL == src_index)
    {
	debug(DEBUG_ID, "malloc for src_index fail.");
	return IOFW_ID_ERROR_MALLOC;
    }
    for(i = 0; i < ndims; i ++)
    {
	src_index[i] = 0;
    }

    sub_size = 1;
    dst_start = 0;
    for(i = 0; i < ndims; i ++)
    {
	dst_start += src_start[i] * sub_size;
	sub_size *= dst_dims_len[i];
    }
    dst_data += ele_size * dst_start;

    for(i = 0; i < src_len ; i ++)
    {
	memcpy(dst_data, src_data, ele_size);
	_inc_src_index(ndims, ele_size, dst_dims_len, &dst_data, 
		src_count, src_index);
	src_data += ele_size;
    }
}

int iofw_id_init(int flag)
{
    open_nc = NULL;
    map_table = NULL;

    switch(flag)
    {
	case IOFW_ID_INIT_CLIENT :
	    open_nc = malloc(MAX_OPEN_NC_NUM * sizeof(iofw_nc_t));
	    memset(open_nc, 0, MAX_OPEN_NC_NUM * sizeof(iofw_nc_t));
	    break;
	case IOFW_ID_INIT_SERVER :
	    map_table = qhash_init(_compare,_hash, MAP_HASH_TABLE_SIZE);
	    break;
	default :
	    return IOFW_ID_ERROR_WRONG_FLAG;
    }

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_final()
{
    if(NULL != open_nc)
    {
	free(open_nc);
	open_nc = NULL;
    }

    if(NULL != map_table)
    {
	qhash_destroy_and_finalize(map_table, iofw_id_val_t, hash_link, _val_free);
	map_table = NULL;
    }

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_assign_nc(int *nc_id)
{
    int i;

    open_nc_a ++;
    if(open_nc_a > MAX_OPEN_NC_NUM)
    {
	open_nc_a --;
	return - IOFW_ID_ERROR_TOO_MANY_OPEN;
    }

    for(i = 0; i < MAX_OPEN_NC_NUM; i ++)
    {
	if(!open_nc[i].used)
	{
	    open_nc[i].used = 1;
	    open_nc[i].var_a = 0;
	    open_nc[i].dim_a = 0;
	    *nc_id = open_nc[i].id = i + 1;
	    break;
	}
    }
    
    debug(DEBUG_ID, "assign nc_id = %d", *nc_id);

    assert(i != MAX_OPEN_NC_NUM);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_assign_dim(int ncid, int *dimid)
{
    assert(open_nc[ncid - 1].id == ncid);
    
    *dimid = (++ open_nc[ncid - 1].dim_a);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_assign_var(int ncid, int *varid)
{
    assert(open_nc[ncid - 1].id == ncid);
    
    *varid = (++ open_nc[ncid - 1].var_a);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_map_nc(
	int client_ncid, int server_ncid)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_nc_id = client_ncid;
    val->nc = malloc(sizeof(iofw_id_nc_t));
    val->nc->nc_id = server_nc_id;
    val->nc->recv_client_num = 0;
    val->nc_status = DEFINE_MODE;

    qhash_add(map_table, &key, &(val->hash_link));

    debug(DEBUG_ID, "map ((%d, 0, 0)->(%d, 0, 0)", 
	     client_ncid, server_ncid);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_map_dim(
	int client_ncid, int client_dimid, 
	int server_ncid, int server_dimid, int dim_len)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;
    key.client_dim_id = client_dimid;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_nc_id = client_ncid;
    val->client_dim_id = client_dimid;
    val->dim = malloc(sizeof(iofw_id_dim_t));
    val->dim->nc_id = server_ncid;
    val->dim->dim_id = server_dimid;
    val->dim->dim_len = dim_len;
    val->dim->recv_client_num = 0;

    qhash_add(map_table, &key, &(val->hash_link));
    
    debug(DEBUG_ID, "map ((%d, %d, 0)->(%d, %d, 0))",
	    client_ncid, client_dimid, server_ncid, server_dimid);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_map_var(
	int client_ncid, int client_varid,
	int server_ncid, int server_varid,
	int ndims, int *dims_len, size_t ele_size)
{
    int i;
    size_t data_size;

    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;
    key.client_var_id = client_varid;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_nc_id = client_ncid;
    val->client_var_id = client_varid;

    val->var = malloc(sizeof(iofw_id_var_t));
    val->var->nc_id = server_ncid;
    val->var->var_id = server_varid;
    val->var->recv_client_num = 0;
    val->var->ndims = ndims;
    val->var->dims_len = malloc(sizeof(int) * ndims);
    memcpy(val->var->dims_len, dims_len, sizeof(int) * ndims);
    val->var->ele_size = ele_size;

    data_size = 1;
    for(i = 0; i < ndims; i ++)
    {
	data_size *= dimsids[i];
    }
    val->var->data = malloc(ele_size * dimids); 

    qhash_add(map_table, &key, &(val->hash_link));
    
    debug(DEBUG_ID, "map ((%d, 0, %d)->(%d, 0, %d))",  
	    client_ncid, client_varid, server_ncid, server_varid);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_get_nc(
	int client_ncid, iofw_id_nc_t **nc)
{
    assert(NULL != server_ncid);

    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get nc (%d, 0, 0) null", client_ncid);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	assert(val->var != NULL);
	*nc = val->nc;
    
	debug(DEBUG_ID, "get (%d, 0, 0)", client_ncid);
	
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_dim(
	int client_ncid, int client_dimid, 
	iofw_id_dim_t **dim)
{
    assert(NULL != server_ncid);
    assert(NULL != server_dimid);

    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;
    key.client_dim_id = client_dimid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get dim (%d, %d, 0) null" ,
		client_ncid, client_dimid);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	*dim = val->dim;
	assert(val->dim != NULL);
	debug(DEBUG_ID, "get (%d, %d, 0)", client_ncid, client_dimid);
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_var(
	int client_ncid, int client_varid, 
	iofw_id_var_t **var)
{
    assert(NULL != server_ncid);
    assert(NULL != server_varid);

    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;
    key.client_var_id = client_varid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get var (%d, 0, %d) null",
		client_ncid, client_varid);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	assert(val->var != NULL);
	*var = val->var;
	debug(DEBUG_ID, "get (%d, 0, %d)",
		client_ncid, client_varid, *server_ncid );
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_put_var(
	int client_ncid, int client_varid,
	size_t *start, size_t *count, 
	char *data)
{
    assert(NULL != start);
    assert(NULL != count);
    assert(NULL != data);

    iofw_id_key_t key;
    ifow_id_val_t *val;
    struct qhash_head *link;
    iofw_id_var_t *var;
    int src_indx, dst_index;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_ncid;
    key.client_var_id = client_varid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "Can't find var (%d, 0, %d)", 
		client_ncid, client_varid);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	var = val->var;
	
	for(i = 0; i < var->ndims; i ++)
	{
	    if(start[i] + count[i] >= var->dims_len[i])
	    {
		debug(DEBUG_ID, "Index exceeds : (start[%d] = %lu) + "
			"(count[%d] = %lu) >= (var(%d, 0, %d)->dims_len[%d] "
			"= %lu)", i, start[i], i, count[i]
			client_ncid, client_varid, i, var->dims_len[i]);
		return IOFW_ID_ERROR_EXCEED_BOUND;
	    }
	}
	
	_put_var(var->ndims, var->ele_size, var->dims_len, var->data,
		start, count, data);

	debug(DEBUG_ID, "put var ((%d, 0, %d)", 
		client_ncid, client_varid, *server_ncid,);
	return IOFW_ID_ERROR_NONE;
    }
}
	

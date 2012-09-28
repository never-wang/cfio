/****************************************************************************
 *       Filename:  id.c
 *
 *    Description:  manage client's id assign, and server's id map, include
 *		    nc_id, var_id, dim_id
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
#include "iofw_types.h"
#include "debug.h"
#include "times.h"
#include "quickhash.h"
#include "map.h"

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

    a = _key->client_nc_id; b = _key->client_dim_id; c = _key->client_var_id;
    final(a, b, c);

    int h = (int)(c & (table_size - 1));

    return h;
}

static void _val_free(iofw_id_val_t *val)
{
    if(NULL != val)
    {
	if(NULL != val->var)
	{
	    if(NULL != val->var->start)
	    {
		free(val->var->start);
		val->var->start = NULL;
	    }
	    if(NULL != val->var->count)
	    {
		free(val->var->count);
		val->var->count = NULL;
	    }
	    if(NULL != val->var->recv_data)
	    {
		free(val->var->recv_data);
		val->var->recv_data = NULL;
	    }
	    if(NULL != val->var->data)
	    {
		free(val->var->data);
		val->var->data = NULL;
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

    dim = ndims - 1;
    src_index[dim] ++;

    sub_size = ele_size;
    while(src_index[dim] >= src_dims_len[dim])
    {
	*dst_addr -= (src_dims_len[dim] - 1) * sub_size;
	src_index[dim] = 0;
	sub_size *= dst_dims_len[dim];
	dim --;
	src_index[dim] ++;
    }

    *dst_addr += sub_size;
}

/**
 * @brief: put the src data array into the dst data array, src and dst both are
 *	sub-array of a total data array
 *
 * @param ndims: number of dimensions for the variable
 * @param ele_size: size of each element in the variable array
 * @param dst_start: start index of the dst data array
 * @param dst_count: count of teh dst data array
 * @param dst_data: pointer to the dst data array
 * @param src_start: start index of the src data array
 * @param src_count: count of teh src data array
 * @param src_data: pointer to the src data array
 */
static void _put_var(
	int ndims, size_t ele_size,
	size_t *dst_start, size_t *dst_count, char *dst_data, 
	size_t *src_start, size_t *src_count, char *src_data)
{
    int i;
    size_t src_len;
    size_t dst_offset, sub_size;
    size_t *src_index;

    //float *_data = src_data;
    //for(i = 0; i < 4; i ++)
    //{
    //    printf("%f, ", _data[i]);
    //}
    //printf("\n");

    assert(NULL != dst_start);
    assert(NULL != dst_count);
    assert(NULL != dst_data);
    assert(NULL != src_start);
    assert(NULL != src_count);
    assert(NULL != src_data);

    src_len = 1;
    for(i = 0; i < ndims; i ++)
    {
	src_len *= src_count[i];
    }

    src_index = malloc(sizeof(size_t) * ndims);
    if(NULL == src_index)
    {
	debug(DEBUG_ID, "malloc for src_index fail.");
	return;
	//return IOFW_ID_ERROR_MALLOC;
    }
    for(i = 0; i < ndims; i ++)
    {
	src_index[i] = 0;
    }

    sub_size = 1;
    dst_offset = 0;
    for(i = ndims - 1; i >= 0; i --)
    {
	dst_offset += (src_start[i] - dst_start[i]) * sub_size;
	sub_size *= dst_count[i];
    }
    //debug(DEBUG_ID, "dst_offset = %d", dst_offset);
    dst_data += ele_size * dst_offset;

    for(i = 0; i < src_len - 1; i ++)
    {
	memcpy(dst_data, src_data, ele_size);
	_inc_src_index(ndims, ele_size, dst_count, &dst_data, 
		src_count, src_index);
	src_data += ele_size;
    }
    memcpy(dst_data, src_data, ele_size);
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

int iofw_id_assign_dim(int nc_id, int *dim_id)
{
    assert(open_nc[nc_id - 1].id == nc_id);
    
    *dim_id = (++ open_nc[nc_id - 1].dim_a);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_assign_var(int nc_id, int *var_id)
{
    assert(open_nc[nc_id - 1].id == nc_id);
    
    *var_id = (++ open_nc[nc_id - 1].var_a);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_map_nc(
	int client_nc_id, int server_nc_id)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_nc_id = client_nc_id;
    val->nc = malloc(sizeof(iofw_id_nc_t));
    val->nc->nc_id = server_nc_id;
    val->nc->nc_status = DEFINE_MODE;

    qhash_add(map_table, &key, &(val->hash_link));
    INIT_QLIST_HEAD(&(val->link));

    debug(DEBUG_ID, "map ((%d, 0, 0)->(%d, 0, 0)", 
	     client_nc_id, server_nc_id);

    return IOFW_ID_ERROR_NONE;
}

/**
 * name : addr_copy
 **/
int iofw_id_map_dim(
	char *name, 
	int client_nc_id, int client_dim_id, 
	int server_nc_id, int server_dim_id)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;
    iofw_id_val_t *nc_val;
    struct qhash_head *link;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;

    link = qhash_search(map_table, &key);
    nc_val = qlist_entry(link, iofw_id_val_t, hash_link);
    
    key.client_dim_id = client_dim_id;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_nc_id = client_nc_id;
    val->client_dim_id = client_dim_id;
    val->dim = malloc(sizeof(iofw_id_dim_t));
    memset(val->dim, 0, sizeof(iofw_id_dim_t));
    val->dim->name = name;
    val->dim->nc_id = server_nc_id;
    val->dim->dim_id = server_dim_id;

    qhash_add(map_table, &key, &(val->hash_link));
    qlist_add_tail(&(val->link), &(nc_val->link));
    
    debug(DEBUG_ID, "map ((%d, %d, 0)->(%d, %d, 0))",
	    client_nc_id, client_dim_id, server_nc_id, server_dim_id);

    return IOFW_ID_ERROR_NONE;
}

/**
 * name, start, count , dim_ids : addr_copy
 **/
int iofw_id_map_var(
	char *name, 
	int client_nc_id, int client_var_id,
	int server_nc_id, int server_var_id,
	int ndims, int *dim_ids,
	size_t *start, size_t *count,
	int data_type, int client_num)
{
    int i;
    size_t data_size;
    iofw_id_val_t *nc_val;

    iofw_id_key_t key;
    iofw_id_val_t *val;
    struct qhash_head *link;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;
    
    link = qhash_search(map_table, &key);
    nc_val = qlist_entry(link, iofw_id_val_t, hash_link);
    
    key.client_var_id = client_var_id;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_nc_id = client_nc_id;
    val->client_var_id = client_var_id;

    val->var = malloc(sizeof(iofw_id_var_t));
    val->var->name = name;
    val->var->nc_id = server_nc_id;
    val->var->var_id = server_var_id;
    val->var->ndims = ndims;
    val->var->client_num = client_num;
    val->var->dim_ids = dim_ids;
    val->var->start = start;
    val->var->count = count;

    assert(client_num > 0);
    val->var->recv_data = malloc(sizeof(iofw_id_data_t) * client_num);
    memset(val->var->recv_data, 0, sizeof(iofw_id_data_t) * client_num);
    val->var->data_type = data_type;
    
    val->var->data = NULL;

    qhash_add(map_table, &key, &(val->hash_link));
    qlist_add_tail(&(val->link), &(nc_val->link));
    
    debug(DEBUG_ID, "map ((%d, 0, %d)->(%d, 0, %d))",  
	    client_nc_id, client_var_id, server_nc_id, server_var_id);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_get_val(
	int client_nc_id, int client_var_id, int client_dim_id,
	iofw_id_val_t **val)
{
    iofw_id_key_t key;
    struct qhash_head *link;
    
    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;
    key.client_var_id = client_var_id;
    key.client_dim_id = client_dim_id;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get nc (%d, 0, 0) null", client_nc_id);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	*val = qlist_entry(link, iofw_id_val_t, hash_link);
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_nc(
	int client_nc_id, iofw_id_nc_t **nc)
{
    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get nc (%d, 0, 0) null", client_nc_id);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	assert(val->nc != NULL);
	*nc = val->nc;
    
	debug(DEBUG_ID, "get (%d, 0, 0)", client_nc_id);
	
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_dim(
	int client_nc_id, int client_dim_id, 
	iofw_id_dim_t **dim)
{
    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;
    key.client_dim_id = client_dim_id;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get dim (%d, %d, 0) null" ,
		client_nc_id, client_dim_id);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	*dim = val->dim;
	assert(val->dim != NULL);
	debug(DEBUG_ID, "get (%d, %d, 0)", client_nc_id, client_dim_id);
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_var(
	int client_nc_id, int client_var_id, 
	iofw_id_var_t **var)
{
    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;
    key.client_var_id = client_var_id;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get var (%d, 0, %d) null",
		client_nc_id, client_var_id);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	assert(val->var != NULL);
	*var = val->var;
	debug(DEBUG_ID, "get (%d, 0, %d)", client_nc_id, client_var_id);
	return IOFW_ID_ERROR_NONE;
    }
}
/**
 * start, count , data : addr_copy
 **/

int iofw_id_put_var(
	int client_nc_id, int client_var_id,
	int client_index,
	size_t *start, size_t *count, 
	char *data)
{
    assert(NULL != start);
    assert(NULL != count);
    assert(NULL != data);

    iofw_id_key_t key;
    iofw_id_val_t *val;
    struct qhash_head *link;
    iofw_id_var_t *var;
    int src_indx, dst_index;
    int i;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_nc_id = client_nc_id;
    key.client_var_id = client_var_id;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "Can't find var (%d, 0, %d)", 
		client_nc_id, client_var_id);
	return IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	var = val->var;

	for(i = 0; i < var->ndims; i ++)
	{
	    if(start[i] + count[i] > var->start[i] + var->count[i] ||
		    start[i] < var->start[i])
	    {
		debug(DEBUG_ID, "Index exceeds : (start[%d] = %lu), "
			"(count[%d] = %lu) ; (var(%d, 0, %d) : (start[%d] = "
			"= %lu), (count[%d] = %lu", i, start[i], i, count[i],
			client_nc_id, client_var_id, 
			i, var->start[i], i, var->count[i]);
		return IOFW_ID_ERROR_EXCEED_BOUND;
	    }
	}

	//float *_data = data;
	//for(i = 0; i < 4; i ++)
	//{
	//    printf("%f, ", _data[i]);
	//}
	//printf("\n");

	if(NULL != var->recv_data[client_index].buf)
	{
	    free(var->recv_data[client_index].buf);
	}
	if(NULL != var->recv_data[client_index].start)
	{
	    free(var->recv_data[client_index].start);
	}
	if(NULL != var->recv_data[client_index].count)
	{
	    free(var->recv_data[client_index].count);
	}
	var->recv_data[client_index].buf = data;
	var->recv_data[client_index].start = start;
	var->recv_data[client_index].count = count;
	debug(DEBUG_ID, "client_index = %d", client_index);

	debug(DEBUG_ID, "put var ((%d, 0, %d)", client_nc_id, client_var_id);
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_merge_var_data(iofw_id_var_t *var)
{
    int i, j;
    size_t ele_size;
    
    for(i = 0; i < var->client_num; i++)
    {
	debug(DEBUG_ID, "merge data of client(%d)", i);

	assert(NULL != var->recv_data[i].buf);
	assert(NULL != var->recv_data[i].start);
	assert(NULL != var->recv_data[i].count);

	for(j = 0; j < var->ndims; j ++)
	{
	    debug(DEBUG_IO, "dim %d: start(%lu), count(%lu)", j, 
		    var->recv_data[i].start[j],var->recv_data[i].count[j]);
	}
    
	iofw_types_size(ele_size, var->data_type);
	_put_var(var->ndims, ele_size, 
		var->start, var->count, var->data,
		var->recv_data[i].start, var->recv_data[i].count,
		var->recv_data[i].buf);
	//float *_data = var->recv_data[i].buf;
	//for(j = 0; j < 4; j ++)
	//{
	//    printf("%f, ", _data[j]);
	//}
	//printf("\n");

	free(var->recv_data[i].buf);	
	var->recv_data[i].buf = NULL;	
	free(var->recv_data[i].start);	
	var->recv_data[i].start = NULL;	
	free(var->recv_data[i].count);	
	var->recv_data[i].count = NULL;	
	
	//_data = var->data;
	//for(j = 0; j < 12; j ++)
	//{
	//    printf("%f, ", _data[j]);
	//}
	//printf("\n");
    }

    return IOFW_ID_ERROR_NONE;
}

void iofw_id_val_free(iofw_id_val_t *val)
{
    int i;
    iofw_id_data_t *recv_data;

    if(NULL != val)
    {
	if(NULL != val->nc)
	{
	    free(val->nc);
	    val->nc = NULL;
	}
	if(NULL != val->dim)
	{
	    if(NULL != val->dim->name)
	    {
		free(val->dim->name);
		val->dim->name = NULL;
	    }
	    free(val->dim);
	    val->dim = NULL;
	}
	if(NULL != val->var)
	{
	    if(NULL != val->var->name)
	    {
		free(val->var->name);
		val->var->name = NULL;
	    }
	    if(NULL != val->var->dim_ids)
	    {
		free(val->var->dim_ids);
		val->var->dim_ids = NULL;
	    }
	    if(NULL != val->var->start)
	    {
		free(val->var->start);
		val->var->start = NULL;
	    }
	    if(NULL != val->var->count)
	    {
		free(val->var->count);
		val->var->count = NULL;
	    }
	    recv_data = val->var->recv_data;
	    if(NULL != recv_data)
	    {
		for(i = 0; i < val->var->client_num; i ++)
		{
		    if(NULL != recv_data[i].buf)
		    {
			free(recv_data[i].buf);
			recv_data[i].buf = NULL;
		    }
		    if(NULL != recv_data[i].start)
		    {
			free(recv_data[i].start);
			recv_data[i].start = NULL;
		    }
		    if(NULL != recv_data[i].count)
		    {
			free(recv_data[i].count);
			recv_data[i].count = NULL;
		    }
		}
		free(recv_data);
		recv_data = NULL;
	    }
	    if(NULL != val->var->data)
	    {
		free(val->var->data);
		val->var->data = NULL;
	    }
	    free(val->var);
	    val->var = NULL;
	}
	free(val);
	val = NULL;
    }
}
	

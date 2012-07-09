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

static int compare(void *key, struct qhash_head *link)
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

static int hash(void *key, int table_size)
{
    iofw_id_key_t *_key = key;
    int a, b, c;

    debug(DEBUG_ID, "(%d, %d, %d, %d)", _key->client_id, _key->client_nc_id,
	    _key->client_dim_id, _key->client_var_id);
    
    a = _key->client_id; b = _key->client_nc_id; c = _key->client_dim_id;
    mix(a, b, c);
    a += _key->client_var_id;
    final(a, b, c);

    int h = (int)(c & (table_size - 1));

    debug(DEBUG_ID, "Hash = %d", h);
    return h;
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
	    map_table = qhash_init(compare, hash, MAP_HASH_TABLE_SIZE);
	    break;
	default :
	    return -IOFW_ID_ERROR_WRONG_FLAG;
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
	qhash_destroy_and_finalize(map_table, iofw_id_val_t, hash_link, free);
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

int iofw_id_map_nc(int client_id, 
	int client_ncid, int server_ncid)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_id = client_id;		
    key.client_nc_id = client_ncid;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_id = client_id;
    val->client_nc_id = client_ncid;
    val->server_nc_id = server_ncid;

    qhash_add(map_table, &key, &(val->hash_link));

    debug(DEBUG_ID, "map ((%d, %d, 0, 0)->(%d, 0, 0)", 
	    client_id, client_ncid, server_ncid);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_map_dim(int client_id,
	int client_ncid, int client_dimid, 
	int server_ncid, int server_dimid)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_id = client_id;		
    key.client_nc_id = client_ncid;
    key.client_dim_id = client_dimid;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_id = client_id;
    val->client_nc_id = client_ncid;
    val->client_dim_id = client_dimid;
    val->server_nc_id = server_ncid;
    val->server_dim_id = server_dimid;

    qhash_add(map_table, &key, &(val->hash_link));
    
    debug(DEBUG_ID, "map ((%d, %d, %d, 0)->(%d, %d, 0))", client_id, 
	    client_ncid, client_dimid, server_ncid, server_dimid);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_map_var(int client_id,
	int client_ncid, int client_varid,
	int server_ncid, int server_varid)
{
    iofw_id_key_t key;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_id = client_id;		
    key.client_nc_id = client_ncid;
    key.client_var_id = client_varid;

    val = malloc(sizeof(iofw_id_val_t));
    memset(val, 0, sizeof(iofw_id_val_t));
    val->client_id = client_id;
    val->client_nc_id = client_ncid;
    val->client_var_id = client_varid;
    val->server_nc_id = server_ncid;
    val->server_var_id = server_varid;

    qhash_add(map_table, &key, &(val->hash_link));
    
    debug(DEBUG_ID, "map ((%d, %d, 0, %d)->(%d, 0, %d))", client_id, 
	    client_ncid, client_varid, server_ncid, server_varid);

    return IOFW_ID_ERROR_NONE;
}

int iofw_id_get_nc(int client_id, 
	int client_ncid, int *server_ncid)
{
    assert(NULL != server_ncid);

    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_id = client_id;
    key.client_nc_id = client_ncid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get nc (%d, %d, 0, 0) null", client_id, client_ncid);
	return -IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	*server_ncid = val->server_nc_id;
    
	debug(DEBUG_ID, "get ((%d, %d, 0, 0)->(%d, 0, 0)", client_id,
		client_ncid, *server_ncid);
	
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_dim(int client_id,
	int client_ncid, int client_dimid, 
	int *server_ncid, int *server_dimid)
{
    assert(NULL != server_ncid);
    assert(NULL != server_dimid);

    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_id = client_id;
    key.client_nc_id = client_ncid;
    key.client_dim_id = client_dimid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get dim (%d, %d, %d, 0) null", client_id,
		client_ncid, client_dimid);
	return -IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	*server_ncid = val->server_nc_id;
	*server_dimid = val->server_dim_id;
	debug(DEBUG_ID, "get ((%d, %d, %d, 0)->(%d, %d, 0))", client_id, 
		client_ncid, client_dimid, *server_ncid, *server_dimid);
	return IOFW_ID_ERROR_NONE;
    }
}

int iofw_id_get_var(int client_id,
	int client_ncid, int client_varid, 
	int *server_ncid, int *server_varid)
{
    assert(NULL != server_ncid);
    assert(NULL != server_varid);

    iofw_id_key_t key;
    struct qhash_head *link;
    iofw_id_val_t *val;

    memset(&key, 0, sizeof(iofw_id_key_t));
    key.client_id = client_id;
    key.client_nc_id = client_ncid;
    key.client_var_id = client_varid;

    if(NULL == (link = qhash_search(map_table, &key)))
    {
	debug(DEBUG_ID, "get var (%d, %d, 0, %d) null", client_id,
		client_ncid, client_varid);
	return -IOFW_ID_ERROR_GET_NULL;
    }else
    {
	val = qlist_entry(link, iofw_id_val_t, hash_link);
	*server_ncid = val->server_nc_id;
	*server_varid = val->server_var_id;
	debug(DEBUG_ID, "get ((%d, %d, 0, %d)->(%d, 0, %d))", client_id, 
		client_ncid, client_varid, *server_ncid, *server_varid);
	return IOFW_ID_ERROR_NONE;
    }
}

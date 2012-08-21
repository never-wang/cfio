/*
 * =====================================================================================
 *
 *       Filename:  unmap.c
 *
 *    Description:  map an io_forward operation into an 
 *    		    real operation 
 *
 *        Version:  1.0
 *        Created:  12/20/2011 04:53:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#include <netcdf.h>
#include <assert.h>

#include "io.h"
#include "id.h"
#include "msg.h"
#include "buffer.h"
#include "debug.h"
#include "mpi.h"
#include "quickhash.h"
#include "iofw_types.h"

extern int client_num;

static struct qhash_table *io_table;

static int _compare(void *key, struct qhash_head *link)
{
    assert(NULL != key);
    assert(NULL != link);

    iofw_io_val_t *val = qlist_entry(link, iofw_io_val_t, hash_link);

    if(0 == memcmp(key, val, sizeof(iofw_id_key_t)))
    {
	return 1;
    }

    return 0;
}

static int _hash(void *key, int table_size)
{
    iofw_io_key_t *_key = key;
    int a, b, c;

    a = _key->func_code; b = _key->client_nc_id; c = _key->client_dim_id;
    mix(a, b, c);
    a += _key->client_var_id;
    final(a, b, c);

    int h = (int)(c & (table_size - 1));

    return h;
}

static void _free(iofw_io_val_t *val)
{
    if(NULL != val)
    {
	free(val);
	val = NULL;
    }
}

/**
 * @brief: add a new client id in the io request bitmap, which means an IO request
 *	   is recived from the client
 *
 * @param bitmap: 
 * @param client_id: 
 */
static inline void _add_bitmap(uint8_t *bitmap, int client_id)
{
    int i, j;

    assert(client_id < client_num);
    assert(bitmap != NULL);

    i = client_id >> 3;
    j = client_id - (i << 3);

    bitmap[i] |= (1 << j);
}

/**
 * @brief: judge whether the bitmap is full, if a bitmap is full, the io request
 *	   it is refer to can be handled by server
 *
 * @param bitmap: 
 *
 * @return: error code
 */
static inline int _bitmap_full(uint8_t *bitmap)
{
    int size, tail;
    int i;

    size = client_num >> 3;
    tail = client_num - (size << 3);

    for(i = 0; i < size; i ++)
    {
	if(bitmap[i] != 255)
	{
	    return 0;
	}
    }

    if(bitmap[i] != ((1 << tail) - 1))
    {
	return 0;
    }

    return 1;
}

/**
 * @brief: recv a io request from client , so put a new io request into the hash
 *	   table
 *
 * @param client_id: client id
 * @param func_code: function code of the io request
 * @param client_nc_id: client nc id
 * @param client_dim_id: client dimension id, if the io request is variable 
 *			 function, the arg will be set to 0
 * @param client_var_id: client variable id, if the io request is dimension 
 *		         function, the arg will be set to 0
 * @param io_info: the put hash value, store all informtion of the IO request
 *
 * @return: error code
 */
static inline int _recv_client_io(
	int client_id, int func_code,
	int client_nc_id, int client_dim_id, int client_var_id,
	iofw_io_val_t **io_info)
{
    iofw_io_key_t key;
    iofw_io_val_t *val;
    qlist_head_t *link;

    key.func_code = func_code;
    key.client_nc_id = client_nc_id;
    key.client_dim_id = client_dim_id;
    key.client_var_id = client_var_id;

    if(NULL == (link = qhash_search(io_table, &key)))
    {
	val = malloc(sizeof(iofw_io_val_t));

	memcpy(val, &key, sizeof(iofw_io_key_t));
	val->client_bitmap = malloc((client_num >> 3) + 1);
	memset(val->client_bitmap, 0, (client_num >> 3) + 1);
	qhash_add(io_table, &key, &(val->hash_link));

    }else
    {
	val = qlist_entry(link, iofw_io_val_t, hash_link);
    }
    _add_bitmap(val->client_bitmap, client_id);
    *io_info = val;

    return IOFW_IO_ERROR_NONE;
}

/**
 * @brief: remove an handled io request from hash table
 *
 * @param io_info: the handled io request
 *
 * @return: error code
 */
static inline int _remove_client_io(
	iofw_io_val_t *io_info)
{
    assert(io_info != NULL);

    qhash_del(&io_info->hash_link);

    if(NULL != io_info->client_bitmap) 
    {
	free(io_info->client_bitmap);
	io_info->client_bitmap = NULL;
    }

    free(io_info);

    return IOFW_IO_ERROR_NONE;
}

int iofw_io_init()
{
    io_table = qhash_init(_compare, _hash, IO_HASH_TABLE_SIZE);

    return IOFW_IO_ERROR_NONE;
}

int iofw_io_final()
{
    if(NULL != io_table)
    {
	qhash_destroy_and_finalize(io_table, iofw_io_val_t, hash_link, _free);
	io_table = NULL;
    }

    return IOFW_IO_ERROR_NONE;
}

int iofw_io_client_done(int client_id, int *server_done)
{
    int func_code = CLIENT_END_IO;
    iofw_io_val_t *io_info;

    _recv_client_io(client_id, func_code, 0, 0, 0, &io_info);

    if(_bitmap_full(io_info->client_bitmap))
    {
	*server_done = 1;
    }

    return IOFW_IO_ERROR_NONE;	
}

int iofw_io_nc_create(int client_id)
{
    int ret, cmode;
    char *path = NULL;
    int nc_id, client_nc_id;
    iofw_id_nc_t *nc;
    iofw_io_val_t *io_info;
    int return_code;
    int func_code = FUNC_NC_CREATE;

    iofw_msg_unpack_nc_create(&path,&cmode, &client_nc_id);

    _recv_client_io(client_id, func_code, client_nc_id, 0, 0, &io_info);
    
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
    {
	iofw_id_map_nc(client_nc_id, IOFW_ID_NC_INVALID);
    }

    if(_bitmap_full(io_info->client_bitmap))
    {
	ret = nc_create(path,cmode,&nc_id);	
	if(ret != NC_NOERR)
	{
	    error("Error happened when open %s error(%s)", 
		    path, nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;

	    goto RETURN;
	}

	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
	{
	    iofw_id_map_nc(client_nc_id, nc_id);
	}else
	{
	    assert(IOFW_ID_NC_INVALID == nc->nc_id);

	    nc->nc_id = nc_id;
	}
	
	_remove_client_io(io_info);
    }

    return_code = IOFW_IO_ERROR_NONE;

RETURN:
    if(NULL != path)
    {
	free(path);
	path = NULL;
    }
    return return_code;
}

int iofw_io_nc_def_dim(int client_id)
{
    int ret = 0;
    int dim_id;
    int client_nc_id, client_dim_id;
    iofw_id_nc_t *nc;
    iofw_id_dim_t *dim;
    iofw_io_val_t *io_info;
    size_t len;
    char *name = NULL;

    int func_code = FUNC_NC_DEF_DIM;
    int return_code;

    iofw_msg_unpack_nc_def_dim(&client_nc_id, &name, &len, &client_dim_id);

    _recv_client_io(
	    client_id, func_code, client_nc_id, client_dim_id, 0, &io_info);
	
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
    {
	return_code = IOFW_IO_ERROR_INVALID_NC;
	debug(DEBUG_IO, "Invalid NC ID.");
	goto RETURN;
    }

    if(IOFW_ID_ERROR_GET_NULL == 
	    iofw_id_get_dim(client_nc_id, client_dim_id, &dim))
    {
	iofw_id_map_dim(client_nc_id, client_dim_id, IOFW_ID_NC_INVALID, 
		IOFW_ID_DIM_INVALID, len);
    }

    if(_bitmap_full(io_info->client_bitmap))
    {
	if(IOFW_ID_NC_INVALID == nc->nc_id)
	{
	    return_code = IOFW_IO_ERROR_INVALID_NC;
	    debug(DEBUG_IO, "Invalid NC ID.");
	    goto RETURN;
	}
	if(nc->nc_status != DEFINE_MODE)
	{
	    return_code = IOFW_IO_ERROR_NC_NOT_DEFINE;
	    debug(DEBUG_IO, "Only can define dim in DEFINE_MODE.");
	    goto RETURN;
	}

	ret = nc_def_dim(nc->nc_id,name,len,&dim_id);
	if( ret != NC_NOERR )
	{
	    error("def dim(%s) error(%s)",name,nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;
	    goto RETURN;
	}
	
	if(IOFW_ID_ERROR_GET_NULL == 
		iofw_id_get_dim(client_nc_id, client_dim_id, &dim))
	{
	    iofw_id_map_dim(client_nc_id, client_dim_id, nc->nc_id, 
		    dim_id, len);
	}else
	{
	    assert(IOFW_ID_DIM_INVALID == dim->dim_id);
	    /**
	     * TODO this should be a return error
	     **/
	    assert(dim->dim_len == len);

	    dim->nc_id = nc->nc_id;
	    dim->dim_id = dim_id;
	}

	_remove_client_io(io_info);
    }

    return_code = IOFW_IO_ERROR_NONE;

RETURN :
    if(NULL != name)
    {
	free(name);
	name = NULL;
    }
    return return_code;
}

int iofw_io_nc_def_var(int client_id)
{
    int ret = 0, i;
    int nc_id, var_id, ndims;
    int client_nc_id, client_var_id;
    iofw_id_nc_t *nc;
    iofw_id_dim_t **dims = NULL; 
    iofw_id_var_t *var = NULL;
    iofw_io_val_t *io_info = NULL;
    int *dim_ids = NULL, *client_dim_ids = NULL;
    size_t *dims_len = NULL;
    char *name = NULL;
    nc_type xtype;

    int func_code = FUNC_NC_DEF_VAR;
    int return_code;

    ret = iofw_msg_unpack_nc_def_var(&client_nc_id, &name, &xtype, &ndims, 
	    &client_dim_ids, &client_var_id);
    if( ret < 0 )
    {
	error("unpack_msg_def_var failed");
	return IOFW_IO_ERROR_MSG_UNPACK;
    }
    dims = malloc(ndims * sizeof(iofw_id_dim_t *));
    dims_len = malloc(ndims * sizeof(size_t));
    dim_ids = malloc(ndims * sizeof(int));

    _recv_client_io(
	    client_id, func_code, client_nc_id, 0, client_var_id, &io_info);

    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
    {
	return_code = IOFW_IO_ERROR_INVALID_NC;
	debug(DEBUG_IO, "Invalid NC ID.");
	goto RETURN;
    }
	
    for(i = 0; i < ndims; i ++)
    {
	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_dim(
		    client_nc_id, client_dim_ids[i], &dims[i]))
	{
	    debug(DEBUG_IO, "Invalid Dim.");
	    return_code = IOFW_IO_ERROR_INVALID_DIM;
	    goto RETURN;
	}
    }

    if(IOFW_ID_ERROR_GET_NULL == 
	    iofw_id_get_var(client_nc_id, client_var_id, &var))
    {
	for(i = 0; i < ndims; i ++)
	{
	    dims_len[i] = dims[i]->dim_len;
	}
	iofw_id_map_var(client_nc_id, client_var_id, IOFW_ID_NC_INVALID, 
		IOFW_ID_VAR_INVALID, ndims, dims_len, xtype);
    }

    if(_bitmap_full(io_info->client_bitmap))
    {
	if(IOFW_ID_NC_INVALID == nc->nc_id)
	{
	    return_code = IOFW_IO_ERROR_INVALID_NC;
	    debug(DEBUG_IO, "Invalid NC ID.");
	    goto RETURN;
	}

	if(nc->nc_status != DEFINE_MODE)
	{
	    return_code = IOFW_IO_ERROR_NC_NOT_DEFINE;
	    debug(DEBUG_IO, "Only can define dim in DEFINE_MODE.");
	    goto RETURN;
	}

	for(i = 0; i < ndims; i ++)
	{
	    assert(nc->nc_id == dims[i]->nc_id);

	    dim_ids[i] = dims[i]->dim_id;
	}
	
	ret = nc_def_var(nc->nc_id,name,xtype,ndims,dim_ids,&var_id);
	if(ret != NC_NOERR)
	{
	    error("def var(%s) error(%s)",name,nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;
	    goto RETURN;
	}

	if(IOFW_ID_ERROR_GET_NULL ==
		iofw_id_get_var(client_nc_id, client_var_id, &var))
	{
	    iofw_id_map_var(client_nc_id, client_var_id, nc->nc_id, var_id,
		    ndims, dims_len, xtype);
	}else
	{
	    assert(IOFW_ID_VAR_INVALID == var->var_id);
	    assert(ndims == var->ndims);

	    var->nc_id = nc->nc_id;
	    var->var_id = var_id;
	}

	_remove_client_io(io_info);
    }
    return_code = IOFW_IO_ERROR_NONE;

RETURN :
    if(name != NULL)
    {
	free(name);
	name = NULL;
    }
    if(dims != NULL)
    {
	free(dims);
	dims = NULL;
    }
    if(client_dim_ids != NULL)
    {
	free(client_dim_ids);
	client_dim_ids = NULL;
    }
    if(dims_len != NULL)
    {
	free(dims_len);
	dims_len = NULL;
    }
    if(dim_ids != NULL)
    {
	free(dim_ids);
	dim_ids = NULL;
    }
    return return_code;
}

int iofw_io_nc_enddef(int client_id)
{
    int client_nc_id, ret;
    iofw_id_nc_t *nc;
    iofw_io_val_t *io_info;

    int func_code = FUNC_NC_ENDDEF;

    ret = iofw_msg_unpack_nc_enddef(&client_nc_id);
    if( ret < 0 )
    {
	error("unapck msg error");
	return IOFW_IO_ERROR_MSG_UNPACK;
    }

    _recv_client_io(client_id, func_code, client_nc_id, 0, 0, &io_info);

    if(_bitmap_full(io_info->client_bitmap))
    {

	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
	{
	    debug(DEBUG_IO, "Invalid NC.");
	    return IOFW_IO_ERROR_INVALID_NC;
	}

	if(DEFINE_MODE == nc->nc_status)
	{
	    ret = nc_enddef(nc->nc_id);
	    if(ret < 0)
	    {
		error("nc_enddef error(%s)",nc_strerror(ret));
		return IOFW_IO_ERROR_NC;
	    }

	    nc->nc_status = DATA_MODE;
	}
	_remove_client_io(io_info);
    }
    return IOFW_IO_ERROR_NONE;
}

int iofw_io_nc_put_vara(int client_id)
{
    int i,ret = 0, ndims;
    iofw_id_nc_t *nc;
    iofw_id_var_t *var;
    iofw_io_val_t *io_info;
    int client_nc_id, client_var_id;
    size_t *start, *count;
    size_t data_size;
    char *data;
    int data_len, data_type;

    int func_code = FUNC_NC_PUT_VARA;
    int return_code;

    //    ret = iofw_unpack_msg_extra_data_size(h_buf, &data_size);
    ret = iofw_msg_unpack_nc_put_vara(
	    &client_nc_id, &client_var_id, &ndims, &start, &count,
	    &data_len, &data_type, &data);	

    //float *_data = data;
    //for(i = 0; i < 4; i ++)
    //{
    //    printf("%f, ", _data[i]);
    //}
    //printf("\n");

    if( ret < 0 )
    {
	error("unpack_msg_put_vara_float failure");
	return IOFW_IO_ERROR_MSG_UNPACK;
    }

    _recv_client_io(
	    client_id, func_code, client_nc_id, 0, client_var_id, &io_info);

    //TODO  check whether data_type is right
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_put_var(
		client_nc_id, client_var_id, start, count, (char*)data))
    {
	return_code = IOFW_IO_ERROR_INVALID_VAR;
	debug(DEBUG_IO, "Invalid var.");
	goto RETURN;
    }

    if(_bitmap_full(io_info->client_bitmap))
    {

	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc) ||
		IOFW_ID_NC_INVALID == nc->nc_id)
	{
	    return_code = IOFW_IO_ERROR_INVALID_NC;
	    debug(DEBUG_IO, "Invalid nc.");
	    goto RETURN;
	}
	if(IOFW_ID_ERROR_GET_NULL == 
		iofw_id_get_var(client_nc_id, client_var_id, &var) ||
		IOFW_ID_VAR_INVALID == var->var_id)
	{
	    return_code = IOFW_IO_ERROR_INVALID_VAR;
	    debug(DEBUG_IO, "Invalid var.");
	    goto RETURN;
	}

	if(ndims != var->ndims)
	{
	    debug(DEBUG_IO, "wrong ndims.");
	    return_code = IOFW_IO_ERROR_WRONG_NDIMS;
	    goto RETURN;
	}

	for(i = 0; i < ndims; i ++)
	{
	    start[i] = 0;
	}
	switch(var->data_type)
	{
	    case IOFW_BYTE :
		break;
	    case IOFW_CHAR :
		break;
	    case IOFW_SHORT :
		ret = nc_put_vara_short(
			nc->nc_id, var->var_id, start, var->dims_len, (short*)var->data);
		break;
	    case IOFW_INT :
		ret = nc_put_vara_int(
			nc->nc_id, var->var_id, start, var->dims_len, (int*)var->data);
		break;
	    case IOFW_FLOAT :
		ret = nc_put_vara_float(
			nc->nc_id, var->var_id, start, var->dims_len, (float*)var->data);
		break;
	    case IOFW_DOUBLE :
		ret = nc_put_vara_double(
			nc->nc_id, var->var_id, start, var->dims_len, (double*)var->data);
		break;
	}

	if( ret != NC_NOERR )
	{
	    error("write nc(%d) var (%d) failure(%s)",
		    nc->nc_id,var->var_id,nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;
	}
	_remove_client_io(io_info);
    }

    return_code = IOFW_IO_ERROR_NONE;	

RETURN :

    if(NULL != start)
    {
	free(start);
	start = NULL;
    }
    debug_mark(DEBUG_IO);
    if(NULL != count)
    {
	free(count);
	count = NULL;
    }

    debug_mark(DEBUG_IO);
    return return_code;

}

int iofw_io_nc_close(int client_id)
{
    int client_nc_id, nc_id, ret;
    iofw_id_nc_t *nc;
    iofw_io_val_t *io_info;
    int func_code = FUNC_NC_CLOSE;

    ret = iofw_msg_unpack_nc_close(&client_nc_id);

    if( ret < 0 )
    {
	error("unpack close error\n");
	return ret;
    }

    _recv_client_io(client_id, func_code, client_nc_id, 0, 0, &io_info);


    if(_bitmap_full(io_info->client_bitmap))
    {
	/*TODO handle memory free*/

	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
	{
	    debug(DEBUG_IO, "Invalid NC.");
	    return IOFW_IO_ERROR_INVALID_NC;
	}
	ret = nc_close(nc->nc_id);
	debug_mark(DEBUG_IO);


	if( ret != NC_NOERR )
	{
	    error("close nc(%d) file failure,%s\n",nc->nc_id,nc_strerror(ret));
	    return IOFW_IO_ERROR_NC;
	}
	_remove_client_io(io_info);
    }
    debug_mark(DEBUG_IO);
    return IOFW_IO_ERROR_NONE;
}


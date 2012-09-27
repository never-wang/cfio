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
#include "mpi.h"

static struct qhash_table *io_table;
static int server_id;

static int _compare(void *key, struct qhash_head *link)
{
    assert(NULL != key);
    assert(NULL != link);

    iofw_io_val_t *val = qlist_entry(link, iofw_io_val_t, hash_link);

    if(0 == memcmp(key, val, sizeof(iofw_io_key_t)))
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

    assert(client_id < iofw_map_get_client_amount());
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
    int is_full;
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    iofw_map_is_bitmap_full(rank, bitmap, &is_full);

    return is_full;
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
    int client_num;

    key.func_code = func_code;
    key.client_nc_id = client_nc_id;
    key.client_dim_id = client_dim_id;
    key.client_var_id = client_var_id;

    client_num = iofw_map_get_client_amount();

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

static inline int _handle_def(iofw_id_val_t *val)
{
    int ret, i;
    iofw_id_nc_t *nc;
    iofw_id_dim_t *dim;
    iofw_id_var_t *var;
    size_t data_size, ele_size;

    if(NULL != val->dim)
    {
	dim = val->dim;
	iofw_id_get_nc(val->client_nc_id, &nc);
	assert(nc->nc_id != IOFW_ID_NC_INVALID);
	dim->nc_id = nc->nc_id;
	debug(DEBUG_IO, "dim_len = %d", dim->dim_len);
        ret = nc_def_dim(nc->nc_id,dim->name,dim->dim_len,&dim->dim_id);
        if( ret != NC_NOERR )
        {
	    debug(DEBUG_IO, "%d", dim->nc_id);
            error("def dim(%s) error(%s)",dim->name,nc_strerror(ret));
            return IOFW_IO_ERROR_NC;
        }
	return IOFW_IO_ERROR_NONE;
    }

    if(NULL != val->var)
    {
	var = val->var;
	for(i = 0; i < var->ndims; i ++)
	{
	    iofw_id_get_dim(val->client_nc_id, var->dim_ids[i], &dim);
	    var->dim_ids[i] = dim->dim_id;
	}
	var->nc_id = dim->nc_id;
	ret = nc_def_var(var->nc_id, var->name, var->data_type, var->ndims,
		var->dim_ids,&var->var_id);
	if(ret != NC_NOERR)
	{
	    error("def var(%s) error(%s)",var->name,nc_strerror(ret));
	    return IOFW_IO_ERROR_NC;
	}
	data_size = 1;
	for(i = 0; i < var->ndims; i ++)
	{
	    data_size *= var->count[i];
	    //debug(DEBUG_IO, "dim %d: start(%lu), count(%lu)", 
	    //        i, var->start[i], var->count[i]);
	}
	iofw_types_size(ele_size, var->data_type);
	var->data = malloc(ele_size * data_size);

	debug(DEBUG_IO, "malloc for var->data, size = %lu * %lu", 
		ele_size ,  data_size);
	return IOFW_IO_ERROR_NONE;
    }

    error("no def can do , this is impossible.");
    return IOFW_IO_ERROR_NC;
}
/**
 * @brief: update a var's start and count, still store in cur_start and cur_count
 *
 * @param ndims: number of dim for the var
 * @param cur_start: current start of the var
 * @param cur_count: current count of the var
 * @param new_start: new start which is to be updated into cur_start 
 * @param new_count: new count which is to be updated into cur_count
 */
void _updata_start_and_count(int ndims, 
	size_t *cur_start, size_t *cur_count,
	size_t *new_start, size_t *new_count)
{
    assert(NULL != cur_start);
    assert(NULL != cur_count);
    assert(NULL != new_start);
    assert(NULL != new_count);

    int min_start, max_end;
    int i;

    for(i = 0; i < ndims; i ++)
    {
	min_start = (cur_start[i] < new_start[i]) ? cur_start[i]:new_start[i];
	max_end = cur_start[i] + cur_count[i] > new_start[i] + new_count[i] ? 
	    cur_start[i] + cur_count[i] : new_start[i] + new_count[i];
	cur_start[i] = min_start;
	cur_count[i] = max_end - min_start;
    }
}

int iofw_io_init()
{
    io_table = qhash_init(_compare, _hash, IO_HASH_TABLE_SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &server_id);

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
	debug(DEBUG_IO, "bit map full");
	*server_done = 1;
    }

    return IOFW_IO_ERROR_NONE;	
}

int iofw_io_create(int client_id)
{
    int ret, cmode;
    char *_path = NULL;
    int nc_id, client_nc_id;
    iofw_id_nc_t *nc;
    iofw_io_val_t *io_info;
    int return_code;
    int func_code = FUNC_NC_CREATE;
    char *path;

    iofw_msg_unpack_create(&_path,&cmode, &client_nc_id);

    /* TODO  */
    path = malloc(strlen(_path) + 32);
    sprintf(path, "%s-%d", _path, server_id);

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
	debug(DEBUG_IO, "nc create(%s) success", path);
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

int iofw_io_def_dim(int client_id)
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

    iofw_msg_unpack_def_dim(&client_nc_id, &name, &len, &client_dim_id);

    //_recv_client_io(
    //        client_id, func_code, client_nc_id, client_dim_id, 0, &io_info);
	
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc))
    {
	return_code = IOFW_IO_ERROR_INVALID_NC;
	debug(DEBUG_IO, "Invalid NC ID.");
	goto RETURN;
    }

    if(IOFW_ID_ERROR_GET_NULL == 
            iofw_id_get_dim(client_nc_id, client_dim_id, &dim))
    {
        iofw_id_map_dim(name, client_nc_id, client_dim_id, IOFW_ID_NC_INVALID, 
        	IOFW_ID_DIM_INVALID);
	debug_mark(DEBUG_IO);
    }else
    {
	free(name);
    }

    //if(_bitmap_full(io_info->client_bitmap))
    //{
    //    if(IOFW_ID_NC_INVALID == nc->nc_id)
    //    {
    //        return_code = IOFW_IO_ERROR_INVALID_NC;
    //        debug(DEBUG_IO, "Invalid NC ID.");
    //        goto RETURN;
    //    }
    //    if(nc->nc_status != DEFINE_MODE)
    //    {
    //        return_code = IOFW_IO_ERROR_NC_NOT_DEFINE;
    //        debug(DEBUG_IO, "Only can define dim in DEFINE_MODE.");
    //        goto RETURN;
    //    }

    //    ret = def_dim(nc->nc_id,name,len,&dim_id);
    //    if( ret != NC_NOERR )
    //    {
    //        error("def dim(%s) error(%s)",name,nc_strerror(ret));
    //        return_code = IOFW_IO_ERROR_NC;
    //        goto RETURN;
    //    }
    //    
    //    if(IOFW_ID_ERROR_GET_NULL == 
    //    	iofw_id_get_dim(client_nc_id, client_dim_id, &dim))
    //    {
    //        iofw_id_map_dim(client_nc_id, client_dim_id, nc->nc_id, 
    //    	    dim_id, len);
    //    }else
    //    {
    //        assert(IOFW_ID_DIM_INVALID == dim->dim_id);
    //        /**
    //         * TODO this should be a return error
    //         **/
    //        assert(dim->dim_len == len);

    //        dim->nc_id = nc->nc_id;
    //        dim->dim_id = dim_id;
    //    }

    //    _remove_client_io(io_info);
    //    debug(DEBUG_IO, "define dim(%s) success", name);
    //}

    return_code = IOFW_IO_ERROR_NONE;

RETURN :
    return return_code;
}

int iofw_io_def_var(int client_id)
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
    size_t *start = NULL, *count = NULL;
    nc_type xtype;
    int client_num;

    int func_code = FUNC_NC_DEF_VAR;
    int return_code;

    ret = iofw_msg_unpack_def_var(&client_nc_id, &name, &xtype, &ndims, 
	    &client_dim_ids, &start, &count, &client_var_id);

    if( ret < 0 )
    {
	error("unpack_msg_def_var failed");
	return IOFW_IO_ERROR_MSG_UNPACK;
    }
    dims = malloc(ndims * sizeof(iofw_id_dim_t *));
    dims_len = malloc(ndims * sizeof(size_t));
    dim_ids = malloc(ndims * sizeof(int));

    //_recv_client_io(
    //        client_id, func_code, client_nc_id, 0, client_var_id, &io_info);

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
	/**
	 *the fisrst def var msg arrive
	 **/
	iofw_map_get_client_num_of_server(server_id, &client_num);
	iofw_id_map_var(name, client_nc_id, client_var_id, 
		IOFW_ID_NC_INVALID, IOFW_ID_VAR_INVALID, 
		ndims, client_dim_ids, start, count, xtype, client_num);
	/**
	 *set each dim's len for the var
	 **/
	for(i = 0; i < ndims; i ++)
	{
	    if(count[i] > dims[i]->dim_len)
	    {
		dims[i]->dim_len = count[i];
	    }
	}
    }else
    {
	/**
	 *update var's start, count and each dim's len
	 **/
	for(i = 0; i < ndims; i ++)
	{
	    /**
	     *TODO handle exceed
	     **/
	    debug(DEBUG_IO, "Pre var dim %d: start(%lu), count(%lu)", 
		    i, var->start[i], var->count[i]);
	    debug(DEBUG_IO, "New var dim %d: start(%lu), count(%lu)", 
		    i, start[i], count[i]);
	}
	_updata_start_and_count(ndims, var->start, var->count, start, count);
	for(i = 0; i < ndims; i ++)
	{
	    if(var->count[i] > dims[i]->dim_len)
	    {
		dims[i]->dim_len = var->count[i];
	    }
	}
	for(i = 0; i < ndims; i ++)
	{
	    debug(DEBUG_IO, "Now var dim %d: start(%lu), count(%lu)", 
		    i, var->start[i], var->count[i]);
	}
	free(start);
	free(count);
	free(client_dim_ids);
    }

    //if(_bitmap_full(io_info->client_bitmap))
    //{
    //    if(IOFW_ID_NC_INVALID == nc->nc_id)
    //    {
    //        return_code = IOFW_IO_ERROR_INVALID_NC;
    //        debug(DEBUG_IO, "Invalid NC ID.");
    //        goto RETURN;
    //    }

    //    if(nc->nc_status != DEFINE_MODE)
    //    {
    //        return_code = IOFW_IO_ERROR_NC_NOT_DEFINE;
    //        debug(DEBUG_IO, "Only can define dim in DEFINE_MODE.");
    //        goto RETURN;
    //    }

    //    for(i = 0; i < ndims; i ++)
    //    {
    //        assert(nc->nc_id == dims[i]->nc_id);

    //        dim_ids[i] = dims[i]->dim_id;
    //    }
    //    
    //    ret = def_var(nc->nc_id,name,xtype,ndims,dim_ids,&var_id);
    //    if(ret != NC_NOERR)
    //    {
    //        error("def var(%s) error(%s)",name,nc_strerror(ret));
    //        return_code = IOFW_IO_ERROR_NC;
    //        goto RETURN;
    //    }

    //    if(IOFW_ID_ERROR_GET_NULL ==
    //    	iofw_id_get_var(client_nc_id, client_var_id, &var))
    //    {
    //        iofw_map_get_client_num_of_server(server_id, &client_num);
    //        iofw_id_map_var(client_nc_id, client_var_id, nc->nc_id, var_id,
    //    	    ndims, dims_len, xtype, client_num);
    //    }else
    //    {
    //        assert(IOFW_ID_VAR_INVALID == var->var_id);
    //        assert(ndims == var->ndims);

    //        var->nc_id = nc->nc_id;
    //        var->var_id = var_id;
    //    }

    //    _remove_client_io(io_info);
    //    debug(DEBUG_IO, "define var(%s) success", name);
    //}
    return_code = IOFW_IO_ERROR_NONE;

RETURN :
    if(dims != NULL)
    {
	free(dims);
	dims = NULL;
    }
    if(dim_ids != NULL)
    {
	free(dim_ids);
	dim_ids = NULL;
    }
    if(dims_len != NULL)
    {
	free(dims_len);
	dims_len = NULL;
    }
    return return_code;
}

//int iofw_io_def_var_range(int client_id)
//{
//    int i,ret = 0, ndims;
//    iofw_id_nc_t *nc;
//    iofw_id_var_t *var;
//    iofw_io_val_t *io_info;
//    int client_nc_id, client_var_id;
//    size_t *start, *count;
//    size_t data_size;
//
//    int func_code = FUNC_DEF_VAR_RANGE;
//    int return_code;
//
//    //    ret = iofw_unpack_msg_extra_data_size(h_buf, &data_size);
//    ret = iofw_msg_unpack_def_var_range(
//	    &client_nc_id, &client_var_id, &ndims, &start, &count);
//    return 0;
//
//    //float *_data = data;
//    //for(i = 0; i < 4; i ++)
//    //{
//    //    printf("%f, ", _data[i]);
//    //}
//    //printf("\n");
//
//    if( ret < 0 )
//    {
//	error("unpack_def_var_range");
//	return IOFW_IO_ERROR_MSG_UNPACK;
//    }
//
//    _recv_client_io(
//	    client_id, func_code, client_nc_id, 0, client_var_id, &io_info);
//
//    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_var(
//		client_nc_id, client_var_id, &var))
//    {
//	return_code = IOFW_IO_ERROR_INVALID_VAR;
//	debug(DEBUG_IO, "Invalid var.");
//	goto RETURN;
//    }
//
//    for(i = 0; i < ndims; i ++)
//    {
//	/**
//	 *TODO handle exceed
//	 **/
//	//debug(DEBUG_IO, 
//	//	"start : %lu, count : %lu", var->start[i], var->count[i]);
//	if(start[i] < var->start[i])
//	{
//	    var->start[i] = start[i];
//	}
//	if(start[i] + count[i] > var->start[i] + var->count[i])
//	{
//	    var->count[i] = start[i] + count[i] - var->start[i];
//	}
//	//debug(DEBUG_IO, 
//	//	"start : %lu, count : %lu", start[i], count[i]);
//	//debug(DEBUG_IO, 
//	//	"dim %d: start(%lu), count(%lu)", var->start[i], var->count[i]);
//    }
//
//    if(_bitmap_full(io_info->client_bitmap))
//    {
//
//	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_nc_id, &nc) ||
//		IOFW_ID_NC_INVALID == nc->nc_id)
//	{
//	    return_code = IOFW_IO_ERROR_INVALID_NC;
//	    debug(DEBUG_IO, "Invalid nc.");
//	    goto RETURN;
//	}
//	if(IOFW_ID_VAR_INVALID == var->var_id)
//	{
//	    return_code = IOFW_IO_ERROR_INVALID_VAR;
//	    debug(DEBUG_IO, "Invalid var.");
//	    goto RETURN;
//	}
//
//	if(ndims != var->ndims)
//	{
//	    debug(DEBUG_IO, "wrong ndims.");
//	    return_code = IOFW_IO_ERROR_WRONG_NDIMS;
//	    goto RETURN;
//	}
//	
//	data_size = 1;
//	for(i = 0; i < var->ndims; i ++)
//	{
//	    data_size *= var->count[i];
//	    debug(DEBUG_IO, "dim %d: start(%lu), count(%lu)", 
//		    i, var->start[i], var->count[i]);
//	}
//	var->data = malloc(var->ele_size * data_size);
//
//	debug(DEBUG_IO, "malloc for var->data, size = %lu * %lu", 
//		var->ele_size ,  data_size);
//	assert(NULL != var->data);
//	
//	_remove_client_io(io_info);
//    }
//
//    return_code = IOFW_IO_ERROR_NONE;	
//
//RETURN :
//
//    if(NULL != start)
//    {
//	free(start);
//	start = NULL;
//    }
//    if(NULL != count)
//    {
//	free(count);
//	count = NULL;
//    }
//
//    return return_code;
//
//}

int iofw_io_enddef(int client_id)
{
    int client_nc_id, ret;
    iofw_id_nc_t *nc;
    iofw_io_val_t *io_info;
    iofw_id_val_t *iter, *nc_val;

    int func_code = FUNC_NC_ENDDEF;

    ret = iofw_msg_unpack_enddef(&client_nc_id);
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
	    iofw_id_get_val(client_nc_id, 0, 0, &nc_val);
	    qlist_for_each_entry(iter, &(nc_val->link), link)
	    {
		if((ret = _handle_def(iter)) < 0)
		{   
		    return ret;
		}
	    }
	    ret = nc_enddef(nc->nc_id);
	    if(ret < 0)
	    {
		error("enddef error(%s)",nc_strerror(ret));
		return IOFW_IO_ERROR_NC;
	    }

	    nc->nc_status = DATA_MODE;
	}
	_remove_client_io(io_info);
    }
    return IOFW_IO_ERROR_NONE;
}

int iofw_io_put_vara(int client_id)
{
    int i,ret = 0, ndims;
    iofw_id_nc_t *nc;
    iofw_id_var_t *var;
    iofw_io_val_t *io_info;
    int client_nc_id, client_var_id;
    size_t *start, *count;
    size_t data_size;
    char *data;
    int data_len, data_type, client_index;

    int func_code = FUNC_NC_PUT_VARA;
    int return_code;

    //    ret = iofw_unpack_msg_extra_data_size(h_buf, &data_size);
    ret = iofw_msg_unpack_put_vara(
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

    iofw_map_get_client_index_of_server(client_id, &client_index);
    //TODO  check whether data_type is right
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_put_var(
		client_nc_id, client_var_id, client_index, 
		start, count, (char*)data))
    {
	return_code = IOFW_IO_ERROR_INVALID_VAR;
	debug(DEBUG_IO, "Invalid var.");
	goto RETURN;
    }

    if(_bitmap_full(io_info->client_bitmap))
    {
	debug(DEBUG_IO, "bit map full");

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

	if(NULL == var->data)
	{
	    return_code = IOFW_IO_ERROR_INVALID_VAR;
	    debug(DEBUG_IO, "Var data is NULL.");
	    goto RETURN;
	}
	
	iofw_id_merge_var_data(var);

	for(i = 0; i < var->ndims; i ++)
	{
	    debug(DEBUG_IO, "dim %d: start(%lu), count(%lu)", 
		    i, var->start[i], var->count[i]);
	}
	
	for(i = 0; i < var->ndims; i ++)
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
		ret = nc_put_vara_short(nc->nc_id, var->var_id, 
			start, var->count, (short*)var->data);
		break;
	    case IOFW_INT :
		ret = nc_put_vara_int(nc->nc_id, var->var_id, 
			start, var->count, (int*)var->data);
		break;
	    case IOFW_FLOAT :
		ret = nc_put_vara_float(nc->nc_id, var->var_id, 
			start, var->count, (float*)var->data);
		break;
	    case IOFW_DOUBLE :
		ret = nc_put_vara_double(nc->nc_id, var->var_id, 
			start, var->count, (double*)var->data);
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
    return return_code;

}

int iofw_io_close(int client_id)
{
    int client_nc_id, nc_id, ret;
    iofw_id_nc_t *nc;
    iofw_io_val_t *io_info;
    int func_code = FUNC_NC_CLOSE;
    iofw_id_val_t *iter, *nc_val;

    ret = iofw_msg_unpack_close(&client_nc_id);

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

	if( ret != NC_NOERR )
	{
	    error("close nc(%d) file failure,%s\n",nc->nc_id,nc_strerror(ret));
	    return IOFW_IO_ERROR_NC;
	}
	_remove_client_io(io_info);
	
	iofw_id_get_val(client_nc_id, 0, 0, &nc_val);
	qlist_for_each_entry(iter, &(nc_val->link), link)
	{
	    qlist_del(&(iter->link));
	    qhash_del(&(iter->hash_link));
	    iofw_id_val_free(iter);
	}
	qhash_del(&(nc_val->hash_link));
	free(nc_val);
    }
    debug_mark(DEBUG_IO);
    return IOFW_IO_ERROR_NONE;
}


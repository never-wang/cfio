/*
 * =====================================================================================
 *
 *       Filename:  cfio.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/19/2011 06:44:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#include <assert.h>
#include <pthread.h>

#include "mpi.h"

#include "cfio.h"
#include "map.h"
#include "id.h"
#include "buffer.h"
#include "msg.h"
#include "debug.h"
#include "times.h"
#include "cfio_error.h"

/* my real rank in mpi_comm_world */
static int rank;
/*  the num of the app proc*/
static int client_num;
static MPI_Comm inter_comm;

int cfio_init(int x_proc_num, int y_proc_num, int ratio)
{
    int rc, i;
    int size;
    int root = 0;
    int error, ret;
    int server_proc_num;
    int best_server_amount;

    //set_debug_mask(DEBUG_CFIO | DEBUG_MSG | DEBUG_BUF);
    //set_debug_mask(DEBUG_CFIO | DEBUG_IO);// | DEBUG_MSG | DEBUG_SERVER);

    rc = MPI_Initialized(&i); 
    if( !i )
    {
	error("MPI should be initialized before the cfio\n");
	return -1;
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    client_num = x_proc_num * y_proc_num;
    server_proc_num = size - client_num;
    if(server_proc_num < 0)
    {
	server_proc_num = 0;
    }
    
    best_server_amount = (int)((double)client_num / ratio);
    if(best_server_amount <= 0)
    {
	best_server_amount = 1;
    }

    if((ret = cfio_map_init(
		    x_proc_num, y_proc_num, server_proc_num, 
		    best_server_amount, MPI_COMM_WORLD)) < 0)
    {
	error("Map Init Fail.");
	return ret;
    }

    if(cfio_map_proc_type(rank) == CFIO_MAP_TYPE_SERVER)
    {
	if((ret = cfio_server_init()) < 0)
	{
	    error("");
	    return ret;
	}
	if((ret = cfio_server_start()) < 0)
	{
	    error("");
	    return ret;
	}
    }else if(cfio_map_proc_type(rank) == CFIO_MAP_TYPE_CLIENT)
    {
	if((ret = cfio_msg_init(CLIENT_BUF_SIZE)) < 0)
	{
	    error("");
	    return ret;
	}

	if((ret = cfio_id_init(CFIO_ID_INIT_CLIENT)) < 0)
	{
	    error("");
	    return ret;
	}
    }

    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}

int cfio_finalize()
{
    int ret,flag;
    cfio_msg_t *msg;

    ret = MPI_Finalized(&flag);
    if(flag)
    {
	error("***You should not call MPI_Finalize before cfio_Finalized*****\n");
	return CFIO_ERROR_FINAL_AFTER_MPI;
    }
    if(cfio_map_proc_type(rank) == CFIO_MAP_TYPE_CLIENT)
    {
	cfio_msg_pack_io_done(&msg, rank);
	cfio_msg_isend(msg);
    }
    
    if(cfio_map_proc_type(rank) == CFIO_MAP_TYPE_SERVER)
    {
	cfio_server_final();
    }else if(cfio_map_proc_type(rank) == CFIO_MAP_TYPE_CLIENT)
    {
	cfio_id_final();
	cfio_msg_final();
    }

    cfio_map_final();
    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}

int cfio_proc_type(int rank)
{
    if(rank < 0)
    {
	error("rank should be positive.");
	return CFIO_ERROR_RANK_INVALID;
    }

    return cfio_map_proc_type(rank);
}

/**
 * @brief: create
 *
 * @param path: the file anme of the new netCDF dataset
 * @param cmode: the creation mode flag
 * @param ncidp: pointer to location where returned netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int cfio_create(
	const char *path, int cmode, int *ncidp)
{
    if(path == NULL || ncidp == NULL)
    {
	error("args should not be NULL.");
	return CFIO_ERROR_ARG_NULL;
    }

    cfio_msg_t *msg;
    int ret;

    if((ret = cfio_id_assign_nc(ncidp)) < 0)
    {
	error("");
	return ret;
    }

    cfio_msg_pack_create(&msg, rank, path, cmode, *ncidp);
    cfio_msg_isend(msg);

    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}
/**
 * @brief: def_dim
 *
 * @param ncid: NetCDF group ID
 * @param name: Dimension name
 * @param len: Length of dimension
 * @param idp: pointer to location for returned dimension ID
 *
 * @return: 0 if success
 */
int cfio_def_dim(
	int ncid, const char *name, size_t len, int *idp)
{
    if(name == NULL || idp == NULL)
    {
	error("args should not be NULL.");
	return CFIO_ERROR_ARG_NULL;
    }
    
    int ret;

    debug(DEBUG_CFIO, "ncid = %d, name = %s, len = %lu",
	    ncid, name, len);
    
    cfio_msg_t *msg;

    if((ret = cfio_id_assign_dim(ncid, idp)) < 0)
    {
	error("");
	return ret;
    }

    cfio_msg_pack_def_dim(&msg, rank, ncid, name, len, *idp);
    cfio_msg_isend(msg);

    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}

int cfio_def_var(
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, 
	int *varidp)
{
    if(name == NULL || varidp == NULL || start == NULL || count == NULL)
    {
	error("args should not be NULL.");
	return CFIO_ERROR_ARG_NULL;
    }
    
    debug(DEBUG_CFIO, "ndims = %d", ndims);

    cfio_msg_t *msg;
    int ret;

    if((ret = cfio_id_assign_var(ncid, varidp)) < 0)
    {
	error("");
	return ret;
    }
    
    cfio_msg_pack_def_var(&msg, rank, ncid, name, xtype, 
	    ndims, dimids, start, count, *varidp);
    cfio_msg_isend(msg);
    
    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}

int cfio_put_att(
	int ncid, int varid, const char *name, 
	nc_type xtype, size_t len, const void *op)
{
    cfio_msg_t *msg;
    
    //if(cfio_map_get_client_index_of_server(rank) == 0)
    //{
    cfio_msg_pack_put_att(&msg, rank, ncid, varid, name,
	    xtype, len, op);
    cfio_msg_isend(msg);
    //}

    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}

int cfio_enddef(
	int ncid)
{
    cfio_msg_t *msg;

    cfio_msg_pack_enddef(&msg, rank, ncid);
    cfio_msg_isend(msg);

    debug(DEBUG_CFIO, "success return.");
    return CFIO_ERROR_NONE;
}

//TODO Maybe can rewrite so better performance
//Just not use it
//static int _put_vara(
//	int ncid, int varid, int dim,
//	const size_t *start, const size_t *count, 
//	const int  fp_type, const char *fp,
//	size_t head_size, int div_dim)
//{
//    size_t *cur_start, *cur_count;
//    int div, left_dim;
//    const char *cur_fp;
//    size_t desc_data_size, div_data_size;
//    int i;
//    cfio_msg_t *msg;
//
//    //times_start();
//
//    debug(DEBUG_CFIO, "dim = %d; div_dim = %d", dim, div_dim);
//
//    cur_start = malloc(dim * sizeof(size_t));
//    cur_count = malloc(dim *sizeof(size_t));
//    memcpy(cur_start, (void *)start, dim * sizeof(size_t));
//    memcpy(cur_count, (void *)count, dim * sizeof(size_t));
//    cur_fp = fp;
//    
//    desc_data_size = 1;
//    for(i = 0; i < div_dim; i ++)
//    {
//	desc_data_size *= count[i]; 
//    }
//    switch(fp_type)
//    {
//	case CFIO_BYTE :
//	    break;
//	case CFIO_CHAR :
//	    break;
//	case CFIO_SHORT :
//	    desc_data_size *= sizeof(short);
//	    break;
//	case CFIO_INT :
//	    desc_data_size *= sizeof(int);
//	    break;
//	case CFIO_FLOAT :
//	    desc_data_size *= sizeof(float);
//	    break;
//	case CFIO_DOUBLE :
//	    desc_data_size *= sizeof(double);
//	    break;
//    }
//    div_data_size = desc_data_size * count[div_dim];
//    
//    div = count[div_dim];
//    //TODO can use binary search
//    while(div_data_size + head_size > MSG_MAX_SIZE)
//    {
//	div_data_size -= desc_data_size;
//	div --;
//    }
//    debug(DEBUG_CFIO, "desc_data_size = %lu, div_data_size = %lu, div = %d",
//	    desc_data_size, div_data_size, div);
//
//    if(0 == div_data_size)
//    {
//	/**
//	 *even take 1 layer can't be ok, so consider the forward dim
//	 **/
//	cur_count[div_dim] = 1;
//	for(i = 0; i < count[div_dim]; i ++)
//	{
//	    _put_vara(io_proc_id, ncid, varid, dim, cur_start, cur_count,
//		    fp_type, cur_fp, head_size, div_dim - 1);
//	    cur_start[div_dim] += 1;
//	    cur_fp += desc_data_size;
//	}
//    }else
//    {
//	cur_count[div_dim] = div;
//	left_dim = count[div_dim];
//	for(i = 0; i < count[div_dim]; i += div)
//	{
//	    cfio_msg_pack_put_vara(&msg, rank, ncid, varid, dim, 
//		    cur_start, cur_count, fp_type, cur_fp);
//	    cfio_msg_isend(msg);
//	    left_dim -= div;
//	    cur_start[div_dim] += div;
//	    cur_count[div_dim] = left_dim >= div ? div : left_dim; 
//	    cur_fp += div_data_size;
//	}
//    }
//
//    //debug(DEBUG_TIME, "%f ms", times_end());
//    debug(DEBUG_CFIO, "success return.");
//    return CFIO_ERROR_NONE;
//}

int cfio_put_vara_float(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp)
{
    if(start == NULL || count == NULL || fp == NULL)
    {
	error("args should not be NULL.");
	return CFIO_ERROR_ARG_NULL;
    }

    cfio_msg_t *msg;

    //times_start();

    //head_size = 6 * sizeof(int) + 2 * dim * sizeof(size_t);

    //_put_vara(io_proc_id, ncid, varid, dim,
    //  start, count, CFIO_FLOAT, fp, head_size, dim - 1);
    cfio_msg_pack_put_vara(&msg, rank, ncid, varid, dim, 
	    start, count, CFIO_FLOAT, fp);
    cfio_msg_isend(msg);

    debug_mark(DEBUG_CFIO);

	//debug(DEBUG_TIME, "%f ms", times_end());

    return CFIO_ERROR_NONE;
}

int cfio_put_vara_double(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const double *fp)
{
    if(start == NULL || count == NULL || fp == NULL)
    {
	error("args should not be NULL.");
	return CFIO_ERROR_ARG_NULL;
    }

    cfio_msg_t *msg;

	//times_start();
    debug(DEBUG_CFIO, "start :(%lu, %lu), count :(%lu, %lu)", 
	    start[0], start[1], count[0], count[1]);

    //head_size = 6 * sizeof(int) + 2 * dim * sizeof(size_t);

    //_put_vara(io_proc_id, ncid, varid, dim,
    //        start, count, CFIO_DOUBLE, fp, head_size, dim - 1);
    cfio_msg_pack_put_vara(&msg, rank, ncid, varid, dim, 
	    start, count, CFIO_DOUBLE, fp);
    cfio_msg_isend(msg);

    debug_mark(DEBUG_CFIO);

	//debug(DEBUG_TIME, "%f ms", times_end());

    return CFIO_ERROR_NONE;
}

int cfio_close(
	int ncid)
{

    cfio_msg_t *msg;
    int ret;
    //times_start();

    if((ret = cfio_id_remove_nc(ncid)) < 0)
    {
	error("");
	return ret;
    }
    cfio_msg_pack_close(&msg, rank, ncid);
    cfio_msg_isend(msg);

    cfio_msg_test();

    debug(DEBUG_CFIO, "Finish cfio_close");

    return CFIO_ERROR_NONE;
}

/**
 *For Fortran Call
 **/
int cfio_init_(int *x_proc_num, int *y_proc_num, int *ratio)
{
    return cfio_init(*x_proc_num, *y_proc_num, *ratio);
}

int cfio_finalize_()
{
    return cfio_finalize();
}

int cfio_proc_type_(int *rank)
{
    return cfio_proc_type(*rank);
}

int cfio_create_(
	const char *path, int *cmode, int *ncidp)
{
    debug(DEBUG_CFIO, "path = %s, cmode = %d", path, *cmode);

    return cfio_create(path, *cmode, ncidp);
}
int cfio_def_dim_(
	int *ncid, const char *name, int *len, int *idp)
{
    size_t _len;

    _len = (int)(*len);

    return cfio_def_dim(*ncid, name, _len, idp);
}

int cfio_def_var_(
	int *ncid, const char *name, int *xtype,
	int *ndims, const int *dimids, 
	const int *start, const int *count, int *varidp)
{
    size_t *_start, *_count;
    int i, ret;

    _start = malloc((*ndims) * sizeof(size_t));
    if(NULL == _start)
    {
	debug(DEBUG_CFIO, "malloc fail");
	return CFIO_ERROR_MALLOC;
    }
    _count = malloc((*ndims) * sizeof(size_t));
    if(NULL == _count)
    {
	free(_start);
	debug(DEBUG_CFIO, "malloc fail");
	return CFIO_ERROR_MALLOC;
    }
    for(i = 0; i < (*ndims); i ++)
    {
	_start[i] = start[i] - 1;
	_count[i] = count[i];
    }

    ret = cfio_def_var(*ncid, name, *xtype, *ndims, dimids, 
	    _start, _count, varidp);
    
    free(_start);
    free(_count);
    return ret;
}

int cfio_put_vara_double_(
	int *ncid, int *varid, int *dim,
	const int *start, const int *count, const double *fp)
{
    size_t *_start, *_count;
    int i, ret;

    _start = malloc((*dim) * sizeof(size_t));
    if(NULL == _start)
    {
	debug(DEBUG_CFIO, "malloc fail");
	return CFIO_ERROR_MALLOC;
    }
    _count = malloc((*dim) * sizeof(size_t));
    if(NULL == _count)
    {
	free(_start);
	debug(DEBUG_CFIO, "malloc fail");
	return CFIO_ERROR_MALLOC;
    }
    for(i = 0; i < (*dim); i ++)
    {
	_start[i] = start[i] - 1;
	_count[i] = count[i];
    }
    ret = cfio_put_vara_double(
	    *ncid, *varid, *dim, _start, _count, fp);
    
    free(_start);
    free(_count);
    return ret;
}

int cfio_enddef_(
	int *ncid)
{
    return cfio_enddef(*ncid);
}

int cfio_close_(
	int *ncid)
{
    return cfio_close(*ncid);
}


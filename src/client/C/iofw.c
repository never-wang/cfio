/*
 * =====================================================================================
 *
 *       Filename:  iofw.c
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

#include "iofw.h"
#include "map.h"
#include "id.h"
#include "buffer.h"
#include "msg.h"
#include "debug.h"
#include "times.h"
#include "error.h"

/* my real rank in mpi_comm_world */
static int rank;
/*  the num of the app proc*/
static int client_num;
static MPI_Comm inter_comm;

/**
 * @brief: init 
 *
 * @param x_proc_num: client proc num of x axis
 * @param y_proc_num: client proc num of y axis
 *
 * @return: error code
 */
int iofw_init(int x_proc_num, int y_proc_num)
{
    int rc, i;
    int size;
    int root = 0;
    int error, ret;
    int server_proc_num;
    int best_server_amount;

    //set_debug_mask(DEBUG_IOFW | DEBUG_MSG | DEBUG_BUF);

    rc = MPI_Initialized(&i); 
    if( !i )
    {
	error("MPI should be initialized before the iofw\n");
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
    
    best_server_amount = (int)((double)client_num * SERVER_RATIO);
    if(best_server_amount <= 0)
    {
	best_server_amount = 1;
    }

    if((ret = iofw_map_init(
		    x_proc_num, y_proc_num, server_proc_num, 
		    best_server_amount, MPI_COMM_WORLD)) < 0)
    {
	error("Map Init Fail.");
	return ret;
    }

    if(iofw_map_proc_type(rank) == IOFW_MAP_TYPE_SERVER)
    {
	if((ret = iofw_server_init()) < 0)
	{
	    error("");
	    return ret;
	}
	if((ret = iofw_server_start()) < 0)
	{
	    error("");
	    return ret;
	}
    }else if(iofw_map_proc_type(rank) == IOFW_MAP_TYPE_CLIENT)
    {
	if((ret = iofw_msg_init(CLIENT_BUF_SIZE)) < 0)
	{
	    error("");
	    return ret;
	}

	if((ret = iofw_id_init(IOFW_ID_INIT_CLIENT)) < 0)
	{
	    error("");
	    return ret;
	}
    }

    debug(DEBUG_IOFW, "success return.");
    return IOFW_ERROR_NONE;
}

int iofw_finalize()
{
    int ret,flag;
    iofw_msg_t *msg;

    ret = MPI_Finalized(&flag);
    if(flag)
    {
	error("***You should not call MPI_Finalize before iofw_Finalized*****\n");
	return IOFW_ERROR_FINAL_AFTER_MPI;
    }
    if(iofw_map_proc_type(rank) == IOFW_MAP_TYPE_CLIENT)
    {
	iofw_msg_pack_io_done(&msg, rank);
	iofw_msg_isend(msg);
    }
    
    if(iofw_map_proc_type(rank) == IOFW_MAP_TYPE_SERVER)
    {
	iofw_server_final();
    }else if(iofw_map_proc_type(rank) == IOFW_MAP_TYPE_CLIENT)
    {
	iofw_id_final();
	iofw_msg_final();
    }

    iofw_map_final();
    debug(DEBUG_IOFW, "success return.");
    return 0;
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
int iofw_create(
	const char *path, int cmode, int *ncidp)
{
    assert(path != NULL);
    assert(ncidp != NULL);

    iofw_msg_t *msg;
    int ret;

    ret = iofw_id_assign_nc(ncidp);
    if(IOFW_ID_ERROR_TOO_MANY_OPEN == ret)
    {
	return -IOFW_ERROR_TOO_MANY_OPEN;
    }

    iofw_msg_pack_create(&msg, rank, path, cmode, *ncidp);
    iofw_msg_isend(msg);

    debug(DEBUG_IOFW, "success return.");
    return IOFW_ERROR_NONE;
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
int iofw_def_dim(
	int ncid, const char *name, size_t len, int *idp)
{
    assert(name != NULL);
    assert(idp != NULL);

    debug(DEBUG_IOFW, "ncid = %d, name = %s, len = %lu",
	    ncid, name, len);
    
    iofw_msg_t *msg;

    iofw_id_assign_dim(ncid, idp);

    iofw_msg_pack_def_dim(&msg, rank, ncid, name, len, *idp);
    iofw_msg_isend(msg);

    debug(DEBUG_IOFW, "success return.");
    return 0;
}

int iofw_def_var(
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, 
	int *varidp)
{
    debug(DEBUG_IOFW, "ndims = %d", ndims);

    assert(name != NULL);
    assert(varidp != NULL);
    assert(start != NULL);
    assert(count != NULL);

    iofw_msg_t *msg;

    iofw_id_assign_var(ncid, varidp);
    
    iofw_msg_pack_def_var(&msg, rank, ncid, name, xtype, 
	    ndims, dimids, start, count, *varidp);
    iofw_msg_isend(msg);
    
    debug(DEBUG_IOFW, "success return.");
    return 0;
}

int iofw_enddef(
	int ncid)
{
    iofw_msg_t *msg;

    iofw_msg_pack_enddef(&msg, rank, ncid);
    iofw_msg_isend(msg);

    debug(DEBUG_IOFW, "success return.");
    return 0;
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
//    iofw_msg_t *msg;
//
//    //times_start();
//
//    debug(DEBUG_IOFW, "dim = %d; div_dim = %d", dim, div_dim);
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
//	case IOFW_BYTE :
//	    break;
//	case IOFW_CHAR :
//	    break;
//	case IOFW_SHORT :
//	    desc_data_size *= sizeof(short);
//	    break;
//	case IOFW_INT :
//	    desc_data_size *= sizeof(int);
//	    break;
//	case IOFW_FLOAT :
//	    desc_data_size *= sizeof(float);
//	    break;
//	case IOFW_DOUBLE :
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
//    debug(DEBUG_IOFW, "desc_data_size = %lu, div_data_size = %lu, div = %d",
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
//	    iofw_msg_pack_put_vara(&msg, rank, ncid, varid, dim, 
//		    cur_start, cur_count, fp_type, cur_fp);
//	    iofw_msg_isend(msg);
//	    left_dim -= div;
//	    cur_start[div_dim] += div;
//	    cur_count[div_dim] = left_dim >= div ? div : left_dim; 
//	    cur_fp += div_data_size;
//	}
//    }
//
//    //debug(DEBUG_TIME, "%f ms", times_end());
//    debug(DEBUG_IOFW, "success return.");
//    return 0;
//}

int iofw_put_vara_float(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp)
{
    //size_t head_size;
    iofw_msg_t *msg;

    //times_start();

    //head_size = 6 * sizeof(int) + 2 * dim * sizeof(size_t);

    //_put_vara(io_proc_id, ncid, varid, dim,
    //  start, count, IOFW_FLOAT, fp, head_size, dim - 1);
    iofw_msg_pack_put_vara(&msg, rank, ncid, varid, dim, 
	    start, count, IOFW_FLOAT, fp);
    iofw_msg_isend(msg);

    debug_mark(DEBUG_IOFW);

	//debug(DEBUG_TIME, "%f ms", times_end());

    return 0;
}

int iofw_put_vara_double(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const double *fp)
{
    //size_t head_size;
    iofw_msg_t *msg;

	//times_start();
    debug(DEBUG_IOFW, "start :(%lu, %lu), count :(%lu, %lu)", 
	    start[0], start[1], count[0], count[1]);

    //head_size = 6 * sizeof(int) + 2 * dim * sizeof(size_t);

    //_put_vara(io_proc_id, ncid, varid, dim,
    //        start, count, IOFW_DOUBLE, fp, head_size, dim - 1);
    iofw_msg_pack_put_vara(&msg, rank, ncid, varid, dim, 
	    start, count, IOFW_DOUBLE, fp);
    iofw_msg_isend(msg);

    debug_mark(DEBUG_IOFW);

	//debug(DEBUG_TIME, "%f ms", times_end());

    return 0;
}

int iofw_close(
	int ncid)
{

    iofw_msg_t *msg;
    //times_start();

    iofw_msg_pack_close(&msg, rank, ncid);
    iofw_msg_isend(msg);

    debug(DEBUG_IOFW, "Finish iofw_close");

    return 0;
}

/**
 *For Fortran Call
 **/
//int iofw_init_(int *x_proc_num, int *y_proc_num)
//{
//    return iofw_init(*x_proc_num, *y_proc_num);
//}
//int iofw_finalize_()
//{
//    return iofw_finalize();
//}
//int iofw_create_(
//	int *io_proc_id, 
//	const char *path, int *cmode, int *ncidp)
//{
//    debug(DEBUG_IOFW, "io_proc_id = %d, path = %s, cmode = %d",
//	    *io_proc_id, path, *cmode);
//
//    return iofw_create(*io_proc_id, path, *cmode, ncidp);
//}
//int iofw_def_dim_(
//	int *io_proc_id, 
//	int *ncid, const char *name, int *len, int *idp)
//{
//
//    return iofw_def_dim(*io_proc_id, *ncid, name, *len, idp);
//}
//
//int iofw_def_var_(
//	int *io_proc_id, 
//	int *ncid, const char *name, int *xtype,
//	int *ndims, const int *dimids, int *varidp)
//{
//    return iofw_def_var(*io_proc_id, *ncid, name, *xtype, *ndims, dimids, varidp);
//}
//
//int iofw_put_vara_double_(
//	int *io_proc_id,
//	int *ncid, int *varid, int *dim,
//	const int *start, const int *count, const double *fp)
//{
//    size_t *_start, *_count;
//    int i, ret;
//
//    _start = malloc((*dim) * sizeof(size_t));
//    if(NULL == _start)
//    {
//	debug(DEBUG_IOFW, "malloc fail");
//	return IOFW_ERROR_MALLOC;
//    }
//    _count = malloc((*dim) * sizeof(size_t));
//    if(NULL == _count)
//    {
//	free(_start);
//	debug(DEBUG_IOFW, "malloc fail");
//	return IOFW_ERROR_MALLOC;
//    }
//    for(i = 0; i < (*dim); i ++)
//    {
//	_start[i] = start[i] - 1;
//	_count[i] = count[i];
//    }
//    ret = iofw_put_vara_double(
//	    *io_proc_id, *ncid, *varid, *dim, _start, _count, fp);
//    
//    free(_start);
//    free(_count);
//    return ret;
//}
//
//int iofw_enddef_(
//	int *io_proc_id, int *ncid)
//{
//    return iofw_enddef(*io_proc_id, *ncid);
//}
//
//int iofw_close_(
//	int *io_proc_id, int *ncid)
//{
//    return iofw_close(*io_proc_id, *ncid);
//}
//

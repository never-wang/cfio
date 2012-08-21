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

#include "iofw.h"
#include "map.h"
#include "id.h"
#include "buffer.h"
#include "msg.h"
#include "debug.h"
#include "times.h"
#include "mpi.h"

/* my real rank in mpi_comm_world */
static int rank;
/*  the number of the app proc*/
int client_num;

MPI_Comm inter_comm;


int iofw_init(int server_group_num, int *server_group_size)
{
    int rc, i;
    int size;
    int root = 0;
    int error, ret;

    char **argv;

    //set_debug_mask(DEBUG_IOFW | DEBUG_MSG);

    rc = MPI_Initialized(&i); 
    if( !i )
    {
	error("MPI should be initialized before the iofw\n");
	return -1;
    }

    MPI_Comm_size(MPI_COMM_WORLD, &client_num);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);


    argv = malloc(2 * sizeof(char*));
    argv[0] = malloc(128);
    sprintf(argv[0], "%d", client_num);
    argv[1] = NULL;
    ret = MPI_Comm_spawn("iofw_server", argv, 1, 
	    MPI_INFO_NULL, root, MPI_COMM_WORLD, &inter_comm, &error);
    free(argv[0]);
    free(argv);
    if(ret != MPI_SUCCESS)
    {
	error("Spwan iofw server fail.");
	return -IOFW_ERROR_INIT;
    }

    if(iofw_msg_init() < 0)
    {
	error("Msg Init Fail.");
	return -IOFW_ERROR_INIT;
    }

    server_group_num = 1;
    server_group_size = malloc(sizeof(int));
    server_group_size[0] = 1;
    iofw_map_init(client_num, server_group_num, server_group_size, &inter_comm);

    if(iofw_id_init(IOFW_ID_INIT_CLIENT) < 0)
    {
	error("Id Init Fail.");
	return -IOFW_ERROR_INIT;
    }

    return 0;
}
    
int iofw_finalize()
{
    int ret,flag;
    iofw_msg_t *msg;

    ret = MPI_Finalized(&flag);
    if(flag)
    {
	error("***You should not call MPI_Finalize before iofw_Finalized*****\n");
	return -1;
    }

    iofw_msg_pack_io_done(&msg, rank);
    iofw_msg_isend(msg);
    
    debug(DEBUG_IOFW, "Finish iofw_finalize");

    iofw_msg_final();
    iofw_id_final();

    return 0;
}

/**
 * @brief: nc_create
 *
 * @param io_proc_id: id of IO proc
 * @param path: the file anme of the new netCDF dataset
 * @param cmode: the creation mode flag
 * @param ncidp: pointer to location where returned netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int iofw_nc_create(
	int io_proc_id, 
	const char *path, int cmode, int *ncidp)
{
    assert(path != NULL);
    assert(ncidp != NULL);

    iofw_msg_t *msg;
    int ret;

    debug_mark(DEBUG_MSG);
    ret = iofw_id_assign_nc(ncidp);
    debug_mark(DEBUG_MSG);
    if(IOFW_ID_ERROR_TOO_MANY_OPEN == ret)
    {
	return -IOFW_ERROR_TOO_MANY_OPEN;
    }

    iofw_msg_pack_nc_create(&msg, rank, path, cmode, *ncidp);
    iofw_msg_isend(msg);

    return IOFW_ERROR_NONE;
}
/**
 * @brief: nc_def_dim
 *
 * @param io_proc_id: id of IO proc 
 * @param ncid: NetCDF group ID
 * @param name: Dimension name
 * @param len: Length of dimension
 * @param idp: pointer to location for returned dimension ID
 *
 * @return: 0 if success
 */
int iofw_nc_def_dim(
	int io_proc_id,
	int ncid, const char *name, size_t len, int *idp)
{
    assert(name != NULL);
    assert(idp != NULL);

    debug(DEBUG_IOFW, "io_proc_id = %d, ncid = %d, name = %s, len = %lu",
	    io_proc_id, ncid, name, len);
    
    iofw_msg_t *msg;

    iofw_id_assign_dim(ncid, idp);

    iofw_msg_pack_nc_def_dim(&msg, rank, ncid, name, len, *idp);
    iofw_msg_isend(msg);

    return 0;
}
/**
 * @brief: nc_def_var
 *
 * @param io_proc_id: id of proc id 
 * @param ncid: netCDF ID
 * @param name: variable name
 * @param xtype: one of the set of predefined netCDF external data types
 * @param ndims: number of dimensions for the variable
 * @param dimids[]: vector of ndims dimension IDs corresponding to the 
 *	variable dimensions
 * @param varidp: pointer to location for the returned variable ID
 *
 * @return: 0 if success
 */
int iofw_nc_def_var(
	int io_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, int *varidp)
{
    debug(DEBUG_MSG, "ndims = %d", ndims);

    assert(name != NULL);
    assert(varidp != NULL);

    iofw_msg_t *msg;

    iofw_id_assign_var(ncid, varidp);
    
    iofw_msg_pack_nc_def_var(&msg, rank, 
	    ncid, name, xtype, ndims, dimids, *varidp);
    iofw_msg_isend(msg);
    
    return 0;
}

/**
 * @brief: nc_enddef
 *
 * @param io_proc_id: id of io proc
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_nc_enddef(
	int io_proc_id,
	int ncid)
{
    iofw_msg_t *msg;

    iofw_msg_pack_nc_enddef(&msg, rank, ncid);
    iofw_msg_isend(msg);

    return 0;
}

//TODO Maybe can rewrite so better performance
static int _nc_put_vara(
	int io_proc_id, int ncid, int varid, int dim,
	const size_t *start, const size_t *count, 
	const int  fp_type, const char *fp,
	size_t head_size, int div_dim)
{
    size_t *cur_start, *cur_count;
    int div, left_dim;
    const char *cur_fp;
    size_t desc_data_size, div_data_size;
    int i;
    iofw_msg_t *msg;

    //times_start();

    debug(DEBUG_IOFW, "dim = %d; div_dim = %d", dim, div_dim);

    cur_start = malloc(dim * sizeof(size_t));
    cur_count = malloc(dim *sizeof(size_t));
    memcpy(cur_start, (void *)start, dim * sizeof(size_t));
    memcpy(cur_count, (void *)count, dim * sizeof(size_t));
    cur_fp = fp;
    
    desc_data_size = 1;
    for(i = 0; i < div_dim; i ++)
    {
	desc_data_size *= count[i]; 
    }
    switch(fp_type)
    {
	case IOFW_BYTE :
	    break;
	case IOFW_CHAR :
	    break;
	case IOFW_SHORT :
	    desc_data_size *= sizeof(short);
	    break;
	case IOFW_INT :
	    desc_data_size *= sizeof(int);
	    break;
	case IOFW_FLOAT :
	    desc_data_size *= sizeof(float);
	    break;
	case IOFW_DOUBLE :
	    desc_data_size *= sizeof(double);
	    break;
    }
    div_data_size = desc_data_size * count[div_dim];
    
    div = count[div_dim];
    //TODO can use binary search
    while(div_data_size + head_size > MSG_MAX_SIZE)
    {
	div_data_size -= desc_data_size;
	div --;
    }
    debug(DEBUG_IOFW, "desc_data_size = %lu, div_data_size = %lu, div = %d",
	    desc_data_size, div_data_size, div);

    if(0 == div_data_size)
    {
	/**
	 *even take 1 layer can't be ok, so consider the forward dim
	 **/
	cur_count[div_dim] = 1;
	for(i = 0; i < count[div_dim]; i ++)
	{
	    _nc_put_vara(io_proc_id, ncid, varid, dim, cur_start, cur_count,
		    fp_type, cur_fp, head_size, div_dim - 1);
	    cur_start[div_dim] += 1;
	    cur_fp += desc_data_size;
	}
    }else
    {
	cur_count[div_dim] = div;
	left_dim = count[div_dim];
	for(i = 0; i < count[div_dim]; i += div)
	{
	    iofw_msg_pack_nc_put_vara(&msg, rank, ncid, varid, dim, 
		    cur_start, cur_count, fp_type, cur_fp);
	    iofw_msg_isend(msg);
	    left_dim -= div;
	    cur_start[div_dim] += div;
	    cur_count[div_dim] = left_dim >= div ? div : left_dim; 
	    cur_fp += div_data_size;
	}
    }

    //debug(DEBUG_TIME, "%f ms", times_end());

    return 0;
}

int iofw_nc_put_vara_float(
	int io_proc_id,
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp)
{
    size_t head_size;

	//times_start();

    head_size = 6 * sizeof(int) + 2 * dim * sizeof(size_t);
    
    _nc_put_vara(io_proc_id, ncid, varid, dim,
	    start, count, IOFW_FLOAT, fp, head_size, dim - 1);

    debug_mark(DEBUG_IOFW);

	//debug(DEBUG_TIME, "%f ms", times_end());

    return 0;
}

int iofw_nc_put_vara_double(
	int io_proc_id,
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const double *fp)
{
    size_t head_size;

	//times_start();

    head_size = 6 * sizeof(int) + 2 * dim * sizeof(size_t);

    debug(DEBUG_IOFW, "start :(%lu, %lu), count :(%lu, %lu)", 
	    start[0], start[1], count[0], count[1]);
    
    _nc_put_vara(io_proc_id, ncid, varid, dim,
	    start, count, IOFW_DOUBLE, fp, head_size, dim - 1);

    debug_mark(DEBUG_IOFW);

	//debug(DEBUG_TIME, "%f ms", times_end());

    return 0;
}
/**
 * @brief: nc_close
 *
 * @param io_proc_id: 
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_nc_close(
	int io_proc_id,
	int ncid)
{

    iofw_msg_t *msg;
    //times_start();

    iofw_msg_pack_nc_close(&msg, rank, ncid);
    iofw_msg_isend(msg);

    debug(DEBUG_IOFW, "Finish iofw_nc_close");

    return 0;
}

/**
 *For Fortran Call
 **/
int iofw_init_(int *server_group_num, int *server_group_size)
{
    return iofw_init(*server_group_num, server_group_size);
}
int iofw_finalize_()
{
    return iofw_finalize();
}
int iofw_nc_create_(
	int *io_proc_id, 
	const char *path, int *cmode, int *ncidp)
{
    debug(DEBUG_IOFW, "io_proc_id = %d, path = %s, cmode = %d",
	    *io_proc_id, path, *cmode);

    return iofw_nc_create(*io_proc_id, path, *cmode, ncidp);
}
int iofw_nc_def_dim_(
	int *io_proc_id, 
	int *ncid, const char *name, int *len, int *idp)
{

    return iofw_nc_def_dim(*io_proc_id, *ncid, name, *len, idp);
}

int iofw_nc_def_var_(
	int *io_proc_id, 
	int *ncid, const char *name, int *xtype,
	int *ndims, const int *dimids, int *varidp)
{
    return iofw_nc_def_var(*io_proc_id, *ncid, name, *xtype, *ndims, dimids, varidp);
}

int iofw_nc_put_vara_double_(
	int *io_proc_id,
	int *ncid, int *varid, int *dim,
	const int *start, const int *count, const double *fp)
{
    size_t *_start, *_count;
    int i, ret;

    _start = malloc((*dim) * sizeof(size_t));
    if(NULL == _start)
    {
	debug(DEBUG_IOFW, "malloc fail");
	return IOFW_ERROR_MALLOC;
    }
    _count = malloc((*dim) * sizeof(size_t));
    if(NULL == _count)
    {
	free(_start);
	debug(DEBUG_IOFW, "malloc fail");
	return IOFW_ERROR_MALLOC;
    }
    for(i = 0; i < (*dim); i ++)
    {
	_start[i] = start[i] - 1;
	_count[i] = count[i];
    }
    ret = iofw_nc_put_vara_double(
	    *io_proc_id, *ncid, *varid, *dim, _start, _count, fp);
    
    free(_start);
    free(_count);
    return ret;
}

int iofw_nc_enddef_(
	int *io_proc_id, int *ncid)
{
    return iofw_nc_enddef(*io_proc_id, *ncid);
}

int iofw_nc_close_(
	int *io_proc_id, int *ncid)
{
    return iofw_nc_close(*io_proc_id, *ncid);
}


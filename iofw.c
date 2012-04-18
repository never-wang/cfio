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
#include "pomme_buffer.h"
#include "pomme_queue.h"
#include "mpi.h"
#include "pack.h"
#include "msg.h"
#include "debug.h"
#include "times.h"

/* my real rank in mpi_comm_world */
static int rank;
/*  the number of the app proc*/
int app_proc_num;
/* the number of the server proc */
int server_proc_num;

MPI_Comm inter_comm;

int iofw_init(int iofw_servers)
{
    int rc, i;
    int size;
    int root = 0;
    int error, ret;

    char **argv;

    rc = MPI_Initialized(&i); 
    if( !i )
    {
	error("MPI should be initialized before the iofw\n");
	return -1;
    }

    MPI_Comm_size(MPI_COMM_WORLD, &app_proc_num);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    server_proc_num = iofw_servers;

    /* now with "./", TODO delete */
    debug(DEBUG_IOFW, "server_proc_num = %d", server_proc_num);
    
    iofw_map_init(app_proc_num, server_proc_num);

    argv = malloc(2 * sizeof(char*));
    argv[0] = malloc(128);
    sprintf(argv[0], "%d", app_proc_num);
    argv[1] = NULL;
    ret = MPI_Comm_spawn("./iofw_server", argv, server_proc_num, MPI_INFO_NULL, 
	    root, MPI_COMM_WORLD, &inter_comm, &error);
    free(argv[0]);
    free(argv);
    if(ret != MPI_SUCCESS)
    {
	error("Spwan iofw server fail.");
	return -1;
    }

    if(iofw_msg_init() < 0)
    {
	return -1;
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
    iofw_msg_isend(msg, inter_comm);
    
    debug(DEBUG_IOFW, "Finish iofw_finalize");

    iofw_msg_final();

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
	const char *path, int cmode, int *ncidp)
{
    assert(path != NULL);
    assert(ncidp != NULL);

    iofw_msg_t *msg;

    iofw_msg_pack_nc_create(&msg, rank, path, cmode);
    iofw_msg_isend(msg, inter_comm);

    return 0;
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

    iofw_msg_t *msg;

    iofw_msg_pack_nc_def_dim(&msg, rank, ncid, name, len);
    iofw_msg_isend(msg, inter_comm);

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
    assert(name != NULL);
    assert(varidp != NULL);

    iofw_msg_t *msg;

    iofw_msg_pack_nc_def_var(&msg, rank, 
	    ncid, name, xtype, ndims, dimids);
    iofw_msg_isend(msg, inter_comm);
    
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
    iofw_msg_isend(msg, inter_comm);

    return 0;
}

int iofw_nc_put_var1_float(
	int io_proc_id,
	int ncid, int varid, int dim,  
	const size_t *index, const float *fp)
{
    iofw_msg_t *msg;

    iofw_msg_pack_nc_put_var1_float(&msg, rank, 
	    ncid, varid, dim, index, fp);
    iofw_msg_isend(msg, inter_comm);

    free_buf(buf);
    return 0;
}

static int _nc_put_vara_float(
	int io_proc_id, int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp,
	size_t head_size, int div_dim)
{
    size_t *cur_start, *cur_count;
    int div, left_dim;
    const float *cur_fp;
    size_t desc_data_size, div_data_size;
    int i;
    iofw_msg_t *msg;

    //times_start();

    debug(DEBUG_IOFW, "dim = %d; div_dim = %d", dim, div_dim);

    cur_start = malloc(dim * sizeof(size_t));
    cur_count = malloc(dim *sizeof(size_t));
    memcpy(cur_start, start, dim * sizeof(size_t));
    memcpy(cur_count, count, dim * sizeof(size_t));
    cur_fp = fp;
    
    desc_data_size = 1;
    for(i = 0; i < div_dim; i ++)
    {
	desc_data_size *= count[i]; 
    }
    desc_data_size *= sizeof(float);
    div_data_size = desc_data_size * count[div_dim];
    
    div = count[div_dim];
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
	    _nc_put_vara_float(io_proc_id, ncid, varid, dim, cur_start, cur_count,
		    cur_fp, head_size, div_dim - 1);
	    cur_start[div_dim] += 1;
	    cur_fp += desc_data_size / sizeof(float);
	}
    }else
    {
	cur_count[div_dim] = div;
	left_dim = count[div_dim];
	for(i = 0; i < count[div_dim]; i += div)
	{
	    iofw_msg_pack_nc_put_vara_float(&msg, rank, ncid, varid, dim, 
		    cur_start, cur_count, cur_fp);
	    debug_mark(DEBUG_IOFW);
	    iofw_isend_msg(msg, inter_comm);
	    left_dim -= div;
	    cur_start[div_dim] += div;
	    cur_count[div_dim] = left_dim >= div ? div : left_dim; 
	    cur_fp += div_data_size / sizeof(float);
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
    
    _nc_put_vara_float(io_proc_id, ncid, varid, dim,
	    start, count, fp, head_size, dim - 1);

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
    iofw_isend_msg(msg, inter_comm);

    debug(DEBUG_IOFW, "Finish iofw_nc_close");

    return 0;
}


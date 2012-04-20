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

#include "io.h"
#include "msg.h"
#include "buffer.h"
#include "debug.h"
#include "mpi.h"

int iofw_io_client_done(int *client_done, int *server_done,
	int client_to_serve)
{
    
    (*client_done)++;

    debug(DEBUG_IO, "client_done = %d; client_to_serve = %d", 
	    *client_done, client_to_serve);
    if((*client_done) == client_to_serve)
    {
	*server_done = 1;
    }

    return 0;	
}

int iofw_io_nc_create(int client_proc)
{
    int ret, cmode;
    char *path;
    int ncid = 0,dimid;
    
    iofw_msg_unpack_nc_create(&path,&cmode);
    
    //ret = nc_create(path,cmode,&ncid);	
    //if( ret != NC_NOERR )
    //{
    //    error("Error happened when open %s error(%s) \n",path,nc_strerror(ret));
    //    return -IOFW_IO_ERROR_NC;
    //}

    return IOFW_IO_ERROR_NONE;
}

int iofw_io_nc_def_dim(int client_proc)
{
    int ret = 0;
    int ncid,dimid;
    size_t len;
    char *name;
    iofw_msg_unpack_nc_def_dim(&ncid, &name, &len);

   // ret = nc_def_dim(ncid,name,len,&dimid);
   // if( ret != NC_NOERR )
   // {
   //     error("def dim(%s) error(%s)",name,nc_strerror(ret));
   //     return -IOFW_IO_ERROR_NC;
   // }

    return IOFW_IO_ERROR_NONE;
}

int iofw_io_nc_def_var(int client_proc)
{
    int ret = 0;
    int ncid, ndims;
    int *dimids;
    char *name;
    nc_type xtype;
    ret = iofw_msg_unpack_nc_def_var(&ncid, &name, &xtype, &ndims, &dimids);
    //if( ret < 0 )
    //{
    //    error("unpack_msg_def_var failed");
    //    return ret;
    //}

    //int varid;
    //ret = nc_def_var(ncid,name,xtype,ndims,dimids,&varid);
    //if( ret != NC_NOERR )
    //{
    //    error("nc_def_var failed error");
    //    return ret;
    //}

    return 0;
}
int iofw_io_nc_enddef(int client_proc)
{
    int ncid, ret ;
    ret = iofw_msg_unpack_nc_enddef(&ncid);
    //if( ret < 0 )
    //{
    //    error("unapck msg error");
    //    return ret;
    //}
    //ret = nc_enddef(ncid);
    //if( ret != NC_NOERR )
    //{
    //    error("nc_enddef error(%s)",nc_strerror(ret));
    //}
    return IOFW_IO_ERROR_NONE;
}
int iofw_io_nc_put_vara_float(int client_proc)
{
    int i,ret = 0,ncid, varid, dim;
    size_t *start, *count;
    size_t data_size;
    float *data;
    int data_len;

//    ret = iofw_unpack_msg_extra_data_size(h_buf, &data_size);
    ret = iofw_msg_unpack_nc_put_vara_float(
	    &ncid, &varid, &dim, &start, &count,
	    &data_len, &data);	

    if( ret < 0 )
    {
	error("unpack_msg_put_vara_float failure");
	return -1;
    }

    //size_t _start[dim] ,_count[dim];

    //memcpy(_start,start,sizeof(_start));
    //memcpy(_count,count,sizeof(_count));

    //ret = nc_put_vara_float( ncid, varid, _start, _count, (float *)(data));
    //if( ret != NC_NOERR )
    //{
    //    error("write nc(%d) var (%d) failure(%s)",ncid,varid,nc_strerror(ret));
    //    return -1;
    //}
    //

    return IOFW_IO_ERROR_NONE;	

}
/**
 * @brief iofw_io_nc_close 
 *
 * @param src
 * @param my_rank
 * @param buf
 *
 * @return 
 */
int iofw_io_nc_close(int client_proc)
{

    int ncid, ret;
    ret = iofw_msg_unpack_nc_close(&ncid);

    if( ret < 0 )
    {
	error("unpack close error\n");
	return ret;
    }

    //ret = nc_close(ncid);

    //if( ret != NC_NOERR )
    //{
    //    error("close nc %d file failure,%s\n",ncid,nc_strerror(ret));
    //    ret = -1;
    //}
    return IOFW_IO_ERROR_NONE;
}

/**
 * @brief iofw_nc_put_var1_float : 
 *
 * @param src: the rank of the client
 * @param my_rank: 
 * @param buf
 *
 * @return 
 */
int iofw_io_nc_put_var1_float(int client_proc)
{
    return 0;	
}

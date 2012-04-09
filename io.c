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
#include "pack.h"
#include "pomme_queue.h"
#include "pomme_buffer.h"
#include "debug.h"
#include "mpi.h"

static MPI_Comm inter_comm;

int iofw_io_init(MPI_Comm init_inter_comm)
{
    inter_comm = init_inter_comm;
    return 0;
}

int iofw_io_dequeue(int source, int my_rank, io_op_t *op)
{
    int ret = 0;

    if( NULL == op )
    {
	error("nil operation");
	return -1;
    }

    iofw_buf_t * h_buf = create_buf(op->head, op->head_len);
    iofw_buf_t * d_buf = NULL; 

    int code = iofw_unpack_msg_func_code(h_buf);
    switch(code)
    {
	case FUNC_NC_PUT_VARA_FLOAT:

	    ret = iofw_io_nc_put_vara_float(source, my_rank, h_buf);
		//debug(DEBUG_UNMAP,"put vara");
	    break;
	case FUNC_NC_CLOSE:
	    ret = iofw_io_nc_close( source, my_rank, h_buf); 
		//debug(DEBUG_UNMAP,"nc close");
	    break;

	default:
	    error("unknow operation code is %d",code);
	    ret = -1;
	    break;
    }
    free(h_buf);
    return ret;
}

int iofw_io_client_done(int *client_done, int *server_done,
	int client_to_serve)
{
    
    (*client_done)++;

    debug(DEBUG_UNMAP, "client_done = %d; client_to_serve = %d", 
	    *client_done, client_to_serve);
    if((*client_done) == client_to_serve)
    {
	*server_done = 1;
    }

    return 0;	
}

int iofw_io_nc_create(int source, int my_rank, iofw_buf_t * buf)
{
    int ret, cmode;
    char *path;
    int ncid = 0,dimid;
    
    ret = iofw_unpack_msg_create(buf, &path,&cmode);
    if( ret < 0 )
    {
	error("unpack mesg create error");
	ncid = ret;
	goto ack;
    }
    ret = nc_create(path,cmode,&ncid);	
    if( ret != NC_NOERR )
    {
	error("Error happened when open %s error(%s) \n",path,nc_strerror(ret));
	ncid = ret;
    }
ack:
    debug_mark(DEBUG_UNMAP);
    if( (ret = iofw_send_int1(source,my_rank,ncid, inter_comm) ) < 0 )
    {
	error("send ncid to client error");
	return ret;
    }
    debug_mark(DEBUG_UNMAP);
    return 0;
}
int iofw_io_nc_def_dim(int source, int my_rank,iofw_buf_t * buf)
{
    int ret = 0;
    int ncid,dimid;
    size_t len;
    char *name;
    ret = iofw_unpack_msg_def_dim(buf, &ncid, &name, &len);
    if( ret < 0 )
    {
	error("upack for nc_def_dim error");
	dimid = ret;
	goto ack;
    }

    ret = nc_def_dim(ncid,name,len,&dimid);
    if( ret != NC_NOERR )
    {
	error("def dim(%s) error(%s)",name,nc_strerror(ret));
	dimid = ret;
	goto ack;
    }
    //   free(name);
ack:
    ret = iofw_send_int1(source,my_rank, dimid, inter_comm);
    if( ret < 0 )
    {
	error("Send dim id back to client error");
	return ret;
    }
    return 0;
}

int iofw_io_nc_def_var(int src, int my_rank, iofw_buf_t * buf)
{
    int ret = 0;
    int ncid, ndims;
    int *dimids;
    char *name;
    nc_type xtype;
    ret = iofw_unpack_msg_def_var(buf, &ncid, &name,
	    &xtype, &ndims, &dimids);
    if( ret < 0 )
    {
	error("unpack_msg_def_var failed");
	return ret;
    }

    int varid;
    ret = nc_def_var(ncid,name,xtype,ndims,dimids,&varid);
    if( ret != NC_NOERR )
    {
	error("nc_def_var failed error");
	return ret;
    }

    ret = iofw_send_int1(src,my_rank, varid, inter_comm);
    if( ret < 0 )
    {
	error("send varible id to client failed");
	return ret;
    }

    return 0;
}
int iofw_io_nc_end_def(int src, int my_rank, iofw_buf_t *buf)
{
    int ncid, ret ;
    ret = iofw_unpack_msg_enddef(buf, &ncid);
    if( ret < 0 )
    {
	error("unapck msg error");
	return ret;
    }
    ret = nc_enddef(ncid);
    if( ret != NC_NOERR )
    {
	error("nc_enddef error(%s)",nc_strerror(ret));
    }
    return ret;
}
int iofw_io_nc_put_vara_float(int src, int my_rank, iofw_buf_t *h_buf)
{
    int i,ret = 0,ncid, varid, dim;
    size_t *start, *count;
    size_t data_size;
    float *data;
    int data_len;

//    ret = iofw_unpack_msg_extra_data_size(h_buf, &data_size);
    ret = iofw_unpack_msg_put_vara_float(h_buf, 
	    &ncid, &varid, &dim, &start, &count,
	    &data_len, &data);	

    if( ret < 0 )
    {
	error("unpack_msg_put_vara_float failure");
	return -1;
    }

    size_t _start[dim] ,_count[dim];

    memcpy(_start,start,sizeof(_start));
    memcpy(_count,count,sizeof(_count));

    ret = nc_put_vara_float( ncid, varid, _start, _count, (float *)(data));
    if( ret != NC_NOERR )
    {
	error("write nc(%d) var (%d) failure(%s)",ncid,varid,nc_strerror(ret));
	return -1;
    }

    return 0;	

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
int iofw_io_nc_close(int src, int my_rank,iofw_buf_t * buf)
{

    int ncid, ret;
    ret = iofw_unpack_msg_close(buf, &ncid);

    if( ret < 0 )
    {
	error("unpack close error\n");
	return ret;
    }

    ret = nc_close(ncid);

    if( ret != NC_NOERR )
    {
	error("%d close nc %d file failure,%s\n",src,ncid,nc_strerror(ret));
	ret = -1;
    }
    return 0;
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
int iofw_io_nc_put_var1_float(int src, int my_rank,iofw_buf_t * buf)
{
    return 0;	
}
int io_op_queue_init(ioop_queue_t *io_queue,int buffer_size,
	int chunk_size, int max_qlength, char *queue_name)
{
    int ret = 0;
    if( io_queue == NULL )
    {
	error("Trying to init an nil io_op_queue");
	return -1;
    }
    if( queue_name == NULL )
    {
	queue_name = "default io_op_queue";
    }
    memset(io_queue,0,sizeof(ioop_queue_t));

    ret = 	init_queue(&io_queue->opq, queue_name, max_qlength);
    if( ret < 0 )
    {
	error("Init queue(%s) failed",queue_name);
	return -1;
    }

    ret = pomme_buffer_init(&io_queue->buffer, buffer_size,
	    chunk_size);
    if( ret < 0 )
    {
	error("Init pomme buffer failed");
	goto error_1;
    }
    return 0;
error_1:
    distroy_queue(io_queue->opq);
    return ret;
}

int io_op_queue_distroy(ioop_queue_t *io_queue)
{
    if( io_queue == NULL )
    {
	error("trying to distroy an nil io_op_queue");
	return -1;
    }
    int ret = distroy_queue(io_queue->opq);
    if( ret < 0 )
    {
	error("distroy queue fail %s",io_queue->opq->name);
	return ret;
    }
    pomme_buffer_distroy(&io_queue->buffer);
    memset(io_queue,0,sizeof(ioop_queue_t));
    return 0;

}

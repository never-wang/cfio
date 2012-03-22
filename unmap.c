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

#include "msg.h"
#include "unmap.h"
#include "pack.h"
#include "pomme_queue.h"
#include "pomme_buffer.h"
#include "debug.h"

int done = 0;

int client_to_serve;

extern MPI_Comm inter_comm;

static int client_served = 0;

static int iofw_do_nc_create(int source, int tag, int my_rank,Buf buf); 
static int iofw_do_nc_end_def(int, int ,int, Buf);

static int iofw_do_nc_def_dim(int source, int tag, int my_rank,Buf buf);
static int iofw_do_nc_def_var(int src, int tag, int my_rank,Buf buf);

static int iofw_do_nc_put_var1_float(int src, int tag, int my_rank,Buf buf);
static inline int iofw_client_done(int client_rank);

static int iofw_do_nc_close(int src, int tag, int my_rank,Buf buf);
static int iofw_do_nc_put_vara_float(int src, int tag, int my_rank, 
	iofw_buf_t *buf);

int unmap(int source, int tag ,int my_rank,void *buffer,int size)
{	
    int ret = 0;
    Buf buf = create_buf(buffer, size);
    if( buf == NULL )
    {
	error("create buf failure");
	return -1;
    }
    int code = iofw_unpack_msg_func_code(buf);
    switch(code)
    {
	case FUNC_NC_CREATE: 
	    iofw_do_nc_create(source, tag, my_rank, buf);
	    debug(DEBUG_UNMAP, "server %d done nc_create for client %d\n",my_rank,source);
	    break;
	case FUNC_NC_ENDDEF:
	    iofw_do_nc_end_def(source, tag, my_rank, buf);
	    debug(DEBUG_UNMAP, "server %d done nc_end_def for client %d\n",my_rank,source);
	    break;
	case FUNC_NC_DEF_DIM:
	    iofw_do_nc_def_dim(source, tag, my_rank, buf);
	    debug(DEBUG_UNMAP, "server %d done nc_def_dim for client %d\n",my_rank,source);
	    break;
	case FUNC_NC_DEF_VAR:
	    iofw_do_nc_def_var(source, tag, my_rank, buf);
	    debug(DEBUG_UNMAP, "server %d done nc_def_var for client %d\n",my_rank,source);
	    break;
	case FUNC_NC_PUT_VAR1_FLOAT:
	    break;
	case CLIENT_END_IO:
	    iofw_client_done(source);
	    debug(DEBUG_UNMAP, "server %d done client_end_io for client %d\n",my_rank,source);
	    break;
//	case FUNC_NC_PUT_VARA_FLOAT:
//	    iofw_unpack_msg_extra_data_size(buf,data_len);
//	    debug(DEBUG_UNMAP, "server %d done nc_put_vara_float for client %d\n",my_rank,source);
//	    break;
	default:
	    ret = ENQUEUE_MSG;
	    debug(DEBUG_UNMAP, "server %d recv ENQUEUE_MSG from client\n",my_rank,source);
	    break;
    }	
    free(buf);
    return ret;
}

int iofw_do_io(int source,int tag, int my_rank, io_op_t *op)
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
	   // d_buf = create_buf(op->body, op->body_len);

//	    ret = iofw_do_nc_put_vara_float(source, tag,
//		    my_rank, h_buf, d_buf);

	    ret = iofw_do_nc_put_vara_float(source, tag,
		    my_rank, h_buf);
	    break;
	case FUNC_NC_CLOSE:

	    ret = iofw_do_nc_close( source, tag, my_rank, h_buf); 
	    break;

	default:
	    error("unknow operation code is %d",code);
	    ret = -1;
	    break;
    }
    free(h_buf);
    return ret;

}


/**
 * @brief iofw_client_done : report the finish of
 * 			     client
 * @param client_rank
 *
 * @return 
 */
static inline int iofw_client_done(int client_rank)
{
    client_served++;

    debug(DEBUG_UNMAP, "client_served = %d; client_to_serve = %d", 
	    client_served, client_to_serve);
    if( client_served == client_to_serve)
    {
	done = 1;
    }

    return 0;	
}
/*
 * @brief: do the real nc create, 
 * @param source : where the msg is from
 * @param tag: the tag of the msg , unused
 * @param my_rank: self rank
 * @Buf buf: pack bufer
 */
static int iofw_do_nc_create(int source, int tag, 
	int my_rank, Buf buf)
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
/*@brief: nc define dim 
 *@param source: where the msg is from 
 *@param tag: the tag of the msg
 *@param my_rank: self rank
 *@param buf: msg content 
 * */
static int iofw_do_nc_def_dim(int source, int tag, int my_rank,Buf buf)
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

/**
 * @brief iofw_nc_def_var : define a varible in an ncfile
 *
 * @param src: the rank of the client
 * @param tag: the tag of the msg
 * @param my_rank: self rank
 * @param buf: data content
 *
 * @return : o for success ,< for error
 */
static int iofw_do_nc_def_var(int src, int tag, int my_rank, Buf buf)
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

/**
 * @brief iofw_do_nc_end_def 
 *
 * @param src
 * @param tag
 * @param my_rank
 * @param buf
 *
 * @return 
 */
static int iofw_do_nc_end_def( int src, int tag, int my_rank, iofw_buf_t *buf)
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

/**
 * @brief iofw_nc_put_vara_float : write an array to an varible
 *
 * @param src: where the msg com from 
 * @param tag: the tag of the msg
 * @param my_rank: the rank of the server
 * @param h_buf: the buf head to header  
 * @param d_buf: the buf to the data
 *
 * @return 
 */
static int iofw_do_nc_put_vara_float(int src, int tag, int my_rank, 
	iofw_buf_t *h_buf)
{
    int i,ret = 0,ncid, varid, dim;
    size_t *start, *count;
    size_t data_size;
    float *data;
    uint32_t data_len;

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
 * @brief iofw_do_nc_close 
 *
 * @param src
 * @param tag
 * @param my_rank
 * @param buf
 *
 * @return 
 */
static int iofw_do_nc_close(int src, int tag, int my_rank,Buf buf)
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
	error("close nc file failure\n");
	ret = -1;
    }
    return 0;
}

/**
 * @brief iofw_nc_put_var1_float : 
 *
 * @param src: the rank of the client
 * @param tag: the tag of the msg
 * @param my_rank: 
 * @param buf
 *
 * @return 
 */
static int iofw_do_nc_put_var1_float(int src, int tag, int my_rank,Buf buf)
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

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
#include "msg.h"
#include "unmap.h"
#include "pack.h"
#include "netcdf.h"
#include "pomme_queue.h"
#include "pomme_buffer.h"
static int iofw_nc_create(int source, int tag, int my_rank,Buf buf); 
static int iofw_nc_def_dim(int source, int tag, int my_rank,Buf buf);
static int iofw_nc_def_var(int src, int tag, int my_rank,Buf buf);
int unmap(int source, int tag ,int my_rank,void *buffer,int size)
{	
    int ret = 0;
    Buf buf = create_buf(buffer, size);
    int code = iofw_unpack_msg_func_code(buf);
    switch(code)
    {
	case FUNC_NC_CREATE: 
	    iofw_nc_create(source, tag, my_rank, buf);
	    break;
	case FUNC_NC_ENDDEF:
	    break;
	case FUNC_NC_CLOSE:
	    break;
	case FUNC_NC_DEF_DIM:
	    iofw_nc_def_dim(source, tag, my_rank, buf);
	    break;
	case FUNC_NC_DEF_VAR:
	    iofw_nc_def_var(source, tag, my_rank, buf);
	    break;
	case FUNC_NC_PUT_VAR1_FLOAT:
	    break;
	default:
	    debug("unknow operation code is %d @%s %s %d",code,FFL);
	    ret = -1;
	    break;
    }	

    return ret;
}
/*
 * @brief: do the real nc create, 
 * @param source : where the msg is from
 * @param tag: the tag of the msg , unused
 * @param my_rank: self rank
 * @Buf buf: pack bufer
 */
static int iofw_nc_create(int source, int tag, 
	int my_rank, Buf buf)
{
    int ret, cmode;
    char *path;
    ret = iofw_unpack_msg_create(buf, &path,&cmode);
    if( ret < 0 )
    {
	debug("unpack mesg create error@%s %s %d\n",FFL);
	return ret;
    }
    int ncid = 0;
    ret = nc_create(path,cmode,&ncid);
    if( ret != NC_NOERR )
    {
	debug("Error happened when open %s",path);
	return ret;
    }
    free(path);
    if( (ret = iofw_send_int1(source,my_rank,ncid) ) < 0 )
    {
	debug("send ncid to client error%s %s %d\n",FFL);
	return ret;
    }
    free_buf(buf);
    return 0;
}
/*@brief: nc define dim 
 *@param source: where the msg is from 
 *@param tag: the tag of the msg
 *@param my_rank: self rank
 *@param buf: msg content 
 * */
static int iofw_nc_def_dim(int source, int tag, int my_rank,Buf buf)
{
    int ret = 0;
    int ncid,dimid;
    size_t len;
    char *name;
    ret = iofw_unpack_msg_def_dim(buf, &ncid, &name, &len);
    if( ret < 0 )
    {
	debug("upack for nc_def_dim error@%s %s %d\n",FFL);
	return ret;
    }
    ret = nc_def_dim(ncid,name,len,&dimid);
    if( ret != NC_NOERR )
    {
	debug("def dim error@%s %s %d\n",FFL);
	return ret;
    }
    free(name);
    ret = iofw_send_int1(source,my_rank, dimid);
    if( ret < 0 )
    {
	debug("Send dim id back to client error@%s %s %d\n",FLL);
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
static int iofw_nc_def_var(int src, int tag, int my_rank, Buf buf)
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
	    debug("unpack_msg_def_var failed@%s %s %d\n",FFL);
	    return ret;
	}
	int varid;
	ret = nc_def_var(ncid,name,xtype,ndims,dimids,&varid);
	if( ret != NC_NOERR )
	{
	    debug("nc_def_var failed ret is %d@%s %s %d\n",ret, FFL);
	    return ret;
	}
	ret = iofw_send_int1(src,my_rank, varid);
	if( ret < 0 )
	{
	    debug("send varible id to client failed@%s %s %d\n",FFL);
	    return ret;
	}
	free(name);
	free(dimids);
	return 0;
}






int io_op_queue_init(ioop_queue_t *io_queue,int buffer_size,
	int chunk_size, int max_qlength, char *queue_name)
{
    int ret = 0;
    if( io_queue == NULL )
    {
	debug("Trying to init an nil io_op_queue %s %s %d\n",FFL);
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
	debug("Init queue(%s) failed %s %s %d\n",queue_name,FFL);
	return -1;
    }

    ret = pomme_buffer_init(&io_queue->buffer, buffer_size,
	    chunk_size);
    if( ret < 0 )
    {
	debug("Init pomme buffer failed@%s %s %d\n",FFL);
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
	debug("trying to distroy an nil io_op_queue");
	return -1;
    }
    int ret = distroy_queue(io_queue->opq);
    if( ret < 0 )
    {
	debug("distroy queue fail %s",io_queue->opq->name);
	return ret;
    }
    pomme_buffer_distroy(io_queue->buffer);
    memset(io_queue,0,sizeof(ioop_queue_t));
    return 0;

}

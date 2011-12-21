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
static int iofw_nc_create(BUF); 
int unmap(int source, int tag ,int my_rank,void *buffer,int size)
{	
	int ret = 0;
	Buf buf = create_buf(buffer, size);
	int code = iofw_unpack_msg_func_code(buf);
       	swtich(code)
	{
		case FUNC_NC_CREATE: 
			break;
		case FUNC_NC_ENDDEF:
			break;
		case FUNC_NC_CLOSE:
			break;
		case FUNC_NC_DEF_DIM:
			break;
		case FUNC_NC_DEF_VAR:
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
 * des: do the real nc create, 
 */
static int iofw_nc_create(int source, int tag, 
		int my_rank, BUF buf)
{
	int ret, cmode;
	char *path;
	ret = iofw_unpack_msg_create(buf, &path,&cmode);
	if( ret < 0 )
	{
		debug("unpack mesg create error@%s %s %d\n",FFL);
		return ret;
	}
	int ncid = nccreate(path,mode);
	

	
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

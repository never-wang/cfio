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
#include "iofw.h"
#include "unmap.h"
#include "map.h"
#include "pomme_buffer.h"
#include "pomme_queue.h"
#include <pthread.h>
#include <mpi.h>
/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* global head queue */
ioop_queue_t server_queue;
/* the server who i should send io to */
static int my_server_rank = -1;
/* my real rank in mpi_comm_world */
static int rank = -1;
/* the app_rank is the rank of self in the world*/
static int app_rank = -1;
/* the number of the app proc*/
static int app_proc_num = -1;
/* the number of the server proc */
static int server_proc_num = -1;
/* the number of the client i serve */
int client_to_serve = 0;
/* the number of the client i served */
int client_served = 0;
/* all the client report done */
int done = 0;
/* if i am server */
int i_am_server = 0;

/*
 * get the rank of the server of me 
 */

static void * iofw_writer(void *argv)
{
    pomme_queue_t *queue = server_queue.opq;
	pomme_buffer_t *buffer = server_queue.buffer;
    queue_body_t *pos = NULL;
	iofw_buf_t *h_buf = NULL;
	iofw_buf_t *d_buf = NULL;
	int ret = 0;
    while(1)
    {
		do
		{
			pos = queue_get_front(queue);
		}while(pos == NULL);
		io_op_t *entry = queue_entry(pos, io_op_t,next_head);  

		ret = iofw_do_io(entry->src, entry->tag, rank, entry);
		if( ret < 0 )
		{
			debug("iofw_writer write fail@%s %s %d\n",FFL);
		}
		int h_start = (char *)entry->head - (char *)server_queue.buffer;
		int b_start = (char *)entry->body - (char *)server_queue.buffer;

		pomme_buffer_release(buffer, h_start, entry->head_len);
		pomme_buffer_release(buffer, b_start, entry->body_len);

		/* here need optimaze*/
		free(entry);

    }

    return NULL;	
}
int iofw_server()
{
    int ret = 0;

    /* main thread is used to do read msg from client */
    reader = pthread_self();
    if( (ret = pthread_create(&writer,NULL,&iofw_writer,NULL))<0  )
    {
	debug("Tread create error(%s) @%s %s %d\n",stderror(ret),FFL);
	return ret;
    }
    static int done = 0;
    void *p_buffer = NULL;
    pomme_buffer_t *buffer = server_queue.buffer;
    pomme_queue_t *queue = server_queue.opq;
    while( done == 0 )
    {
	int offset = 0;
	/*
	 * wait for any mesg header
	 */
	do{
	    offset = pomme_buffer_next(buffer,buffer->chunk_size);
	}while(offset<0);	
	p_buffer = buffer->buffer+offset;
	/*
	 * chunk size is set to the max length of the header
	 */
	MPI_Status status;
	ret = MPI_Recv(p_buffer,buffer->chunk_size,
		MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	int count = 0;
	MPI_Get_count(&status,MPI_BYTE,&count);
	/*the length should include the data length */
	ret = unmap(status.MPI_SOURCE, status.MPI_TAG, rank,p_buffer,count);
	if( ret < 0 )
	{
	    debug("unmap data failed\n");
	    continue;
	}
	if( ret > 0 )
	{
	    io_op_t * op = malloc(sizeof(io_op_t));
	    memset(op, 0, sizeof(io_op_t));
	    op->head = p_buffer;
	    op->head_len = count;
		op->src = status.MPI_SOURCE;
		op->tag = status.MPI_TAG;

	    queue_push_back(queue,&op->next_head);
	    int len = ret, recv_len = 0;

	    do
	    {
		offset = pomme_buffer_next(buffer,ret);
	    }while( offset < 0 );

	    p_buffer = buffer->buffer + offset;
	    op->body = p_buffer;
	    int src = status.MPI_SOURCE;
	    int tag = status.MPI_TAG;
	    int tlen = 0;
	    do{
		ret = MPI_Recv(p_buffer, len, MPI_BYTE, src, tag, MPI_COMM_WORLD,&status);
		MPI_Get_count(&status, MPI_BYTE, &tlen);
		recv_len += tlen;
		p_buffer += tlen;
	    }while(recv_len < len );
	    op->body_len = len;
	}  
    }
    return 0;
}
int iofw_init(int iofw_servers, int *is_server)
{
    int rc, i;
    int size;

    rc = MPI_Initialized(&i); 
    if( !i )
    {
	debug("MPI should be initialized before the iofw\n");
	return -1;
    }

    MPI_Comm_size(MPI_COMM_WORLD,&size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    app_proc_num = size - iofw_servers; 
    server_proc_num = iofw_servers;
    /* *
     * the higher iofw_server is treated as the server
     * thus the app will not need to change there rank
     * */
    if( rank >= app_proc_num)
    {
	*is_server = 1;
	i_am_server = 1;
    }else{
	iofw_map_forwarding_proc(rank, &my_server_rank);
	app_rank = rank;
    }
    if( *is_server )
    {
	int ret = 0;
	char name[10];
	sprintf(name,"buffer@%d",rank);
	memset(&server_queue, 0, sizeof(ioop_queue_t));
	ret = io_op_queue_init(&server_queue, BUFFER_SIZE, 
		CHUNK_SIZE, MAX_QUEUE_SIZE, name);
	if( ret < 0 )
	{
	    debug("rank %d init io op queue failure@%s %s %d\n",rank,FFL);
	    return -1;
	}
	rc = iofw_server();
    }
    return 0;

}
int iofw_Finalize()
{
    int ret,flag;

    ret = MPI_Finalized(&flag);
    if(flag)
    {
	debug("***You should not call MPI_Finalize before iofw_Finalized");
    }
    if( i_am_server )
    {
	debug("server %d exit\n",rank);
    }else{
	ret = iofw_io_stop(rank);
    }
    return 0;
}

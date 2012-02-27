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
#include "io.h"
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
int app_proc_num = -1;
/* the number of the server proc */
int server_proc_num = -1;
/* the number of the client i serve */
int client_to_serve = 0;
/* the number of the client i served */
int client_served = 0;
/* all the client report done */
int done = 0;
int write_done  = 0;
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
    int ret = 0;
    int count = 0;
    while(1)
    {
	do
	{
	    pos = queue_get_front(queue);
	    if( pos == NULL){
		if( done == 1 )
		{
		    goto msg_over;
		}
		//				fprintf(stderr,"sleep %d\n",done);
		//				sleep(1);

	    }
	}while(pos == NULL);

	io_op_t *entry = queue_entry(pos, io_op_t,next_head);  
	ret = iofw_do_io(entry->src, entry->tag, rank, entry);

	if( ret < 0 )
	{
	    debug("iofw_writer write fail@%s %s %d\n",FFL);
	}
	int h_start = entry->head_start;
	int b_start = entry->body_start;


	pomme_buffer_release(buffer, h_start, entry->head_len);
	if( entry->body != NULL)
	{
	    pomme_buffer_release(buffer, b_start, entry->body_len);
	}

	/* here need optimaze*/
	free(entry);

    }
msg_over:
    write_done = 1;
    return NULL;	
}
int iofw_server()
{
    int ret = 0;

    /* main thread is used to do read msg from client */
    reader = pthread_self();
    if( (ret = pthread_create(&writer,NULL,&iofw_writer,NULL))<0  )
    {
	debug("Tread create error() @%s %s %d\n",FFL);
	return ret;
    }
    void *p_buffer = NULL;
    pomme_buffer_t *buffer = server_queue.buffer;
    pomme_queue_t *queue = server_queue.opq;
    while( done == 0 )
    {
	int offset = 0;
	/*
	 * wait for any msg header
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
	size_t len,recv_len;
	ret = unmap(status.MPI_SOURCE, status.MPI_TAG, rank,p_buffer,count, &len);

	if( ret < 0 )
	{
	    switch(ret)
	    {
		case ENQUEUE_MSG:
		    pomme_buffer_take(buffer, count);

		    io_op_t * op = malloc(sizeof(io_op_t));
		    memset(op, 0, sizeof(io_op_t));

		    op->head = p_buffer;
		    op->head_start = offset;
		    op->head_len = count;

		    op->src = status.MPI_SOURCE;
		    op->tag = status.MPI_TAG;

		    op->body = NULL;
		    queue_push_back(queue,&op->next_head);

		    break;
		default:
		    debug("[%s %s %d]:Error",FFL);
	    }
	    continue;
	}
	else if( len > 0 )
	{
	    pomme_buffer_take(buffer, count);

	    io_op_t * op = malloc(sizeof(io_op_t));
	    memset(op, 0, sizeof(io_op_t));

	    op->head = p_buffer;
	    op->head_start = offset;
	    op->head_len = count;

	    op->src = status.MPI_SOURCE;
	    op->tag = status.MPI_TAG;

	    recv_len = 0;

	    do
	    {
			offset = pomme_buffer_next(buffer,ret);
	    }while( offset < 0 );

	    p_buffer = buffer->buffer + offset;

	    op->body = p_buffer;
	    op->body_start = offset;


	    int src = status.MPI_SOURCE;
	    int tag = status.MPI_TAG;
	    int tlen = 0;

	    do{
		ret = MPI_Recv(p_buffer, len - recv_len , MPI_BYTE, src, tag, MPI_COMM_WORLD,&status);
		MPI_Get_count(&status, MPI_BYTE, &tlen);

		recv_len += tlen;
		p_buffer += tlen;

	    }while(recv_len < len );

	    op->body_len = len;
	    pomme_buffer_take(buffer,len);
	    //ret = iofw_do_io(op->src, op->tag, rank, op);

	    queue_push_back(queue,&op->next_head);
	}  
    }
    pthread_join(writer,NULL);
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
	*is_server = 0;
	iofw_map_forwarding_proc(rank, &my_server_rank);
	app_rank = rank;
    }

    if( *is_server )
    {
	int ret = 0;
	char name[10];
	sprintf(name,"buffer@%d",rank);
	memset(&server_queue, 0, sizeof(ioop_queue_t));

	ret = iofw_map_client_num(rank, &client_to_serve);

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
int iofw_finalize()
{
    int ret,flag;

    ret = MPI_Finalized(&flag);
    if(flag)
    {
	debug("***You should not call MPI_Finalize before iofw_Finalized*****\n");
    }
    if( i_am_server )
    {
	//	io_op_queue_distroy(server_queue);
    }else{
	ret = iofw_io_stop(rank);
    }
    return 0;
}

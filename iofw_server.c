/****************************************************************************
 *       Filename:  iofw_server.c
 *
 *    Description:  main program for iofw_server
 *
 *        Version:  1.0
 *        Created:  03/13/2012 02:42:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <pthread.h>

#include "unmap.h"
#include "map.h"
#include "pomme_queue.h"
#include "mpi.h"
#include "debug.h"

#define BUFFER_SIZE  1024*1024*1024
#define CHUNK_SIZE  32*1024
#define MAX_QUEUE_SIZE  1024*1024*1024

/*  the number of the client i serve */
int client_to_serve = 0;
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
/* all the client report done */
extern int done;
int write_done  = 0;
/* Communicator between IO process adn compute process*/   
MPI_Comm inter_comm;

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
	    error("iofw_writer write fail");
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
	error("Tread create error()");
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
	p_buffer = buffer->buffer + offset;
	/*
	 * chunk size is set to the max length of the header
	 */
	MPI_Status status;
	debug_mark(DEBUG_USER);
	ret = MPI_Recv(p_buffer,buffer->chunk_size,
		MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,inter_comm,&status);
	debug_mark(DEBUG_USER);

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
		    error("Error");
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
			offset = pomme_buffer_next(buffer,len);
	    }while( offset < 0 );

	    p_buffer = buffer->buffer + offset;

	    op->body = p_buffer;
	    op->body_start = offset;


	    int src = status.MPI_SOURCE;
	    int tag = status.MPI_TAG;
	    int tlen = 0;

	    do{
		ret = MPI_Recv(p_buffer, len - recv_len , MPI_BYTE, 
			src, tag, inter_comm,&status);
		MPI_Get_count(&status, MPI_BYTE, &tlen);

		recv_len += tlen;
		p_buffer += tlen;
		debug(DEBUG_USER, "recv_len = %d", recv_len);

	    }while(recv_len < len );

	    op->body_len = len;
	    pomme_buffer_take(buffer,len);
	    //ret = iofw_do_io(op->src, op->tag, rank, op);

	    queue_push_back(queue,&op->next_head);
	}
	debug(DEBUG_USER, "done = %d", done);
    }
    debug_mark(DEBUG_USER);
    pthread_join(writer,NULL);
    debug_mark(DEBUG_USER);
    return 0;
}

int main(int argc, char** argv)
{
    int size;

    MPI_Init(&argc, &argv);
   // set_debug_mask(DEBUG_USER | DEBUG_UNMAP);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    debug(DEBUG_USER, "rank = %d; size = %d", rank, size);

    int ret = 0;
    char name[10];
    sprintf(name,"buffer@%d",rank);
    memset(&server_queue, 0, sizeof(ioop_queue_t));

    iofw_map_client_num(rank, &client_to_serve);

    ret = io_op_queue_init(&server_queue, BUFFER_SIZE, 
	    CHUNK_SIZE, MAX_QUEUE_SIZE, name);
    if( ret < 0 )
    {
	error("rank %d init io op queue failure",rank);
	return -1;
    }
    
    ret = MPI_Comm_get_parent(&inter_comm);

    if(ret != MPI_SUCCESS)
    {
	error("MPI_Comm_get_parent fail.");
	return -1;
    }

    if(inter_comm == MPI_COMM_WORLD)
    {
	debug(DEBUG_USER, "inter_comm == MPI_COMM_WORLD");
    }

    iofw_server();

    MPI_Finalize();
    return 0;
}

/****************************************************************************
 *       Filename:  iofw_io.c
 *
 *    Description:  main program for iofw_io
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

#include "map.h"
#include "io.h"
#include "pomme_queue.h"
#include "mpi.h"
#include "debug.h"
#include "msg.h"
#include "times.h"

#define BUFFER_SIZE  1024*1024*1024
#define CHUNK_SIZE  32*1024
#define MAX_QUEUE_SIZE  1024*1024*1024

/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* global head queue */
ioop_queue_t server_queue;
/* my real rank in mpi_comm_world */
static int rank = -1;
/* all the client report done */
int server_done;
int write_done  = 0;
/* Communicator between IO process adn compute process*/   
MPI_Comm inter_comm;

/* Num of clients whose io is done*/
int client_done = 0;
/* Num of clients which need to be served by the server */
int client_to_serve = 0;

static int _decode(int source, int tag, void *buffer,int size)
{	
    int ret = 0;
    /* TODO the buf control here may have some bad thing */
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
	    iofw_io_nc_create(source, rank, buf);
	    debug(DEBUG_USER, "server %d done nc_create for client %d",
		    rank,source);
	    ret = IMM_MSG;
	    break;
	case FUNC_NC_ENDDEF:
	    iofw_io_nc_end_def(source, rank, buf);
	    debug(DEBUG_USER, "server %d done nc_end_def for client %d",
		    rank,source);
	    ret = IMM_MSG;
	    break;
	case FUNC_NC_DEF_DIM:
	    iofw_io_nc_def_dim(source, rank, buf);
	    debug(DEBUG_USER, "server %d done nc_def_dim for client %d",
		    rank,source);
	    ret = IMM_MSG;
	    break;
	case FUNC_NC_DEF_VAR:
	    iofw_io_nc_def_var(source, rank, buf);
	    debug(DEBUG_USER, "server %d done nc_def_var for client %d",
		    rank,source);
	    ret = IMM_MSG;
	    break;
	case FUNC_NC_PUT_VAR1_FLOAT:
	    ret = IMM_MSG;
	    break;
	case CLIENT_END_IO:
	    iofw_io_client_done(&client_done, &server_done,
		    client_to_serve);
	    debug(DEBUG_USER, "server %d done client_end_io for client %d",
		    rank,source);
	    ret = IMM_MSG;
	    break;
	case FUNC_NC_CLOSE:
	   debug(DEBUG_USER,"server %d received nc_close from client %d",
		   rank, source);
	   ret = ENQUEUE_MSG;
	   break;
	case FUNC_NC_PUT_VARA_FLOAT:
	   debug(DEBUG_USER, 
		   "server %d received nc_put_vara_float from client %d", 
		   rank, source);
	   ret = ENQUEUE_MSG;
	   break;
	default:
	   error("server %d received unexpected msg from client %d",
		   rank, source);
	   ret = -1;
	   break;
    }	
    free(buf);
    return ret;
}

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
		if( server_done == 1 )
		{
		    goto msg_over;
		}
	    }
	}while(pos == NULL);

	io_op_t *entry = queue_entry(pos, io_op_t,next_head);  
	ret = iofw_io_dequeue(entry->src, rank, entry);

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

	pos = NULL;
    }
msg_over:
    write_done = 1;
    return NULL;	
}
int iofw_server()
{
    int ret = 0;
    io_op_t *op;

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
    while(server_done == 0 )
    {
	int offset = 0;
	/*
	 * wait for any msg header
	 */
	do{
	    offset = pomme_buffer_next(buffer,buffer->chunk_size);
	}while(offset<0);	
	p_buffer = (char *)buffer->buffer + offset;
	/*
	 * chunk size is set to the max length of the header
	 */
	MPI_Status status;
	ret = MPI_Recv(p_buffer,buffer->chunk_size,
		MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,inter_comm,&status);

	int count = 0;
	MPI_Get_count(&status,MPI_BYTE,&count);
	ret = _decode(status.MPI_SOURCE, status.MPI_TAG, 
		p_buffer,count);

	switch(ret)
	{
	    case ENQUEUE_MSG:
		pomme_buffer_take(buffer, count);

		op = malloc(sizeof(io_op_t));
		memset(op, 0, sizeof(io_op_t));

		op->head = p_buffer;
		op->head_start = offset;
		op->head_len = count;

		op->src = status.MPI_SOURCE;
		op->tag = status.MPI_TAG;

		op->body = NULL;
		queue_push_back(queue,&op->next_head);

		break;
	    case IMM_MSG :
		break;
	}
	continue;
    }
    pthread_join(writer,NULL);
    return 0;
}

int main(int argc, char** argv)
{
    int size;
    int client_num;

    MPI_Init(&argc, &argv);
    times_init();
    times_start();
    set_debug_mask(DEBUG_USER | DEBUG_UNMAP);
    //set_debug_mask(DEBUG_TIME);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    debug(DEBUG_USER, "rank = %d; size = %d", rank, size);

    int ret = 0;
    char name[10];
    sprintf(name,"buffer@%d",rank);
    memset(&server_queue, 0, sizeof(ioop_queue_t));

    ret = io_op_queue_init(&server_queue, BUFFER_SIZE, 
	    MSG_MAX_SIZE, MAX_QUEUE_SIZE, name);
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

    /**
     * init for io's inter_comm
     **/
    iofw_io_init(inter_comm);
    /**
     * init for client and server num
     **/
    client_num = atoi(argv[1]);
    iofw_map_init(client_num, size);

    iofw_map_client_num(rank, &client_to_serve);
    client_done = 0;
    debug(DEBUG_USER, "client_to_serve = %d", client_to_serve);

    iofw_server();

    debug(DEBUG_TIME, "ifow_server total time : %f ms", times_end());
    times_final();

    MPI_Finalize();
    return 0;
}

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
int client_to_served = 0;






/*
 * get the rank of the server of me 
 */
static inline int get_server_rank(int my_rank);

static void * iofw_writer(void *argv)
{
	
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
    while( done == 0 )
    {
	/* buffer */
	pomme_buffer_t *buffer = server_queue.buffer;
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
    }else{
	my_server_rank = get_server_num(rank); 
	app_rank = rank;
    }
    if( *is_server )
    {
	int ret = 0;
	char name[10];
	sprintf(name,"buffer@%d",rank);
	ret = io_op_queue_init(&server_queue, BUFFER_SIZE, 
		CHUNK_SIZE, MAX_QUEUE_SIZE, name);
	if( ret < 0 )
	{
	    debug("rank %d init io op queue failure@%s %s %d\n",rank,FFL);
	    return -1;
	}
	rc = iofw_server();
    }

}
static inline int get_server_rank(int my_rank)
{
    return (app_proc_num + my_rank%server_proc_num);
}

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

/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* my real rank in mpi_comm_world */
static int rank;
/* all the client report done */
int server_done;
/* Communicator between IO process adn compute process*/   
MPI_Comm inter_comm;

/* Num of clients whose io is done*/
int client_done = 0;
/* Num of clients which need to be served by the server */
int client_to_serve = 0;

static int decode()
{	
    int client_proc = 0;
    int ret = 0;
    int code;
    /* TODO the buf control here may have some bad thing */
    iofw_msg_unpack_func_code(&code);

    switch(code)
    {
	case FUNC_NC_CREATE: 
	    iofw_io_nc_create(client_proc);
	    debug(DEBUG_USER, "server %d done nc_create for client %d",
		    rank,source);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_DEF_DIM:
	    iofw_io_nc_def_dim(client_proc);
	    debug(DEBUG_USER, "server %d done nc_def_dim for client %d",
		    rank,source);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_DEF_VAR:
	    iofw_io_nc_def_var(client_proc);
	    debug(DEBUG_USER, "server %d done nc_def_var for client %d",
		    rank,source);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_ENDDEF:
	    iofw_io_nc_end_def(client_proc);
	    debug(DEBUG_USER, "server %d done nc_end_def for client %d",
		    rank,source);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_PUT_VAR1_FLOAT:
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_PUT_VARA_FLOAT:
	    debug(DEBUG_USER, 
		    "server %d received nc_put_vara_float from client %d", 
		    rank, source);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_CLOSE:
	    debug(DEBUG_USER,"server %d received nc_close from client %d",
		    rank, source);
	    return IOFW_SERVER_ERROR_NONE;
	case CLIENT_END_IO:
	    iofw_io_client_done(&client_done, &server_done,
		    client_to_serve);
	    debug(DEBUG_USER, "server %d done client_end_io for client %d",
		    rank,source);
	    return IOFW_SERVER_ERROR_NONE;
	default:
	    error("server %d received unexpected msg from client %d",
		    rank, source);
	    return -IOFW_SERVER_ERROR_UNEXPECTED_MSG;
    }	
}

static void * iofw_writer(void *argv)
{
    int ret = 0;
    int count = 0;
    while(!server_done)
    {
	decode();
    }
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
    while(!server_done)
    {
	iofw_msg_recv(inter_comm);
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

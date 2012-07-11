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

#include "iofw_server.h"
#include "map.h"
#include "io.h"
#include "id.h"
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
/* Communicator between IO process adn compute process*/   
static MPI_Comm inter_comm;

/* Num of clients whose io is done*/
static int client_done = 0;
/* Num of clients which need to be served by the server */
static int client_to_serve = 0;
/* all the client report done */
static int server_done = 0;

static inline void _client_end_io()
{
   if((++client_done) == client_to_serve)
   {
       server_done = 1;
   }

   debug(DEBUG_USER, "client_done = %d, client_to_serve = %d", 
	   client_done, client_to_serve);
}

static int decode(iofw_msg_t *msg)
{	
    int client_proc = 0;
    int ret = 0;
    uint32_t code;
    /* TODO the buf control here may have some bad thing */
    iofw_msg_unpack_func_code(msg, &code);
    client_proc = msg->src;

    switch(code)
    {
	case FUNC_NC_CREATE: 
	    debug(DEBUG_USER,"server %d recv nc_create from client %d",
		    rank, client_proc);
	    iofw_io_nc_create(client_proc);
	    debug(DEBUG_USER, "server %d done nc_create for client %d\n",
		    rank,client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_DEF_DIM:
	    debug(DEBUG_USER,"server %d recv nc_def_dim from client %d",
		    rank, client_proc);
	    iofw_io_nc_def_dim(client_proc);
	    debug(DEBUG_USER, "server %d done nc_def_dim for client %d\n",
		    rank,client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_DEF_VAR:
	    debug(DEBUG_USER,"server %d recv nc_def_var from client %d",
		    rank, client_proc);
	    iofw_io_nc_def_var(client_proc);
	    debug(DEBUG_USER, "server %d done nc_def_var for client %d\n",
		    rank,client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_ENDDEF:
	    debug(DEBUG_USER,"server %d recv nc_enddef from client %d",
		    rank, client_proc);
	    iofw_io_nc_enddef(client_proc);
	    debug(DEBUG_USER, "server %d done nc_enddef for client %d\n",
		    rank,client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_PUT_VAR1_FLOAT:
	    debug(DEBUG_USER,"server %d recv nc_put_vara1_float from client %d",
		    rank, client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_PUT_VARA_FLOAT:
	    debug(DEBUG_USER,"server %d recv nc_put_vara_float from client %d",
		    rank, client_proc);
	    iofw_io_nc_put_vara_float(client_proc);
	    debug(DEBUG_USER, 
		    "server %d done nc_put_vara_float from client %d\n", 
		    rank, client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_CLOSE:
	    debug(DEBUG_USER,"server %d recv nc_close from client %d",
		    rank, client_proc);
	    iofw_io_nc_close(client_proc);
	    debug(DEBUG_USER,"server %d received nc_close from client %d\n",
		    rank, client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	case CLIENT_END_IO:
	    debug(DEBUG_USER,"server %d recv client_end_io from client %d",
		    rank, client_proc);
	    _client_end_io();
	    debug(DEBUG_USER, "server %d done client_end_io for client %d\n",
		    rank,client_proc);
	    return IOFW_SERVER_ERROR_NONE;
	default:
	    error("server %d received unexpected msg from client %d",
		    rank, client_proc);
	    return -IOFW_SERVER_ERROR_UNEXPECTED_MSG;
    }	
}

static void * iofw_writer(void *argv)
{
    int ret = 0;
    iofw_msg_t *msg;
    int count = 0;
    while(!server_done)
    {
	msg = iofw_msg_get_first();
	if(NULL != msg)
	{
	    decode(msg);
	}
    }

    pthread_cancel(reader);

    debug(DEBUG_USER, "\nWriter done");
    return ((void *)0);
}
static void* iofw_reader(void *argv)
{
    while(1)
    {
	iofw_msg_recv(rank, inter_comm);
    }
}

int iofw_server()
{
    int ret = 0;

    if( (ret = pthread_create(&writer,NULL,iofw_writer,NULL))<0  )
    {
	error("Tread Writer create error()");
	return ret;
    }
    if( (ret = pthread_create(&reader,NULL,iofw_reader,NULL))<0  )
    {
	error("Tread Reader create error()");
	return ret;
    }

    pthread_join(writer, NULL);

    return 0;
}

int main(int argc, char** argv)
{
    int size;
    int client_num;

    MPI_Init(&argc, &argv);
    times_init();
    times_start();
    //set_debug_mask(DEBUG_USER | DEBUG_MSG);
    //set_debug_mask(DEBUG_ID);
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

    iofw_msg_init();
    /**
     * init for client and server num
     **/
    client_num = atoi(argv[1]);
    if(iofw_id_init(IOFW_ID_INIT_SERVER) < 0)
    {
	error("ID Init Fail.");
	return -1;
    }

    iofw_map_client_num(rank, size, &client_to_serve);
    client_done = 0;
    debug(DEBUG_USER, "client_to_serve = %d", client_to_serve);

    server_done = 0;

    iofw_server();

    debug(DEBUG_TIME, "ifow_server total time : %f ms", times_end());
    times_final();

    iofw_msg_final();
    iofw_id_final();

    MPI_Finalize();
    return 0;
}

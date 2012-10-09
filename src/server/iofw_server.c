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
#include "define.h"

/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* my real rank in mpi_comm_world */
static int rank;
static int server_proc_num;	    /* server group size */
/* Communicator between IO process adn compute process*/   
static MPI_Comm inter_comm;

/* Num of clients whose io is done*/
static int client_done = 0;
/* all the client report done */
static int server_done = 0;

static int decode(iofw_msg_t *msg)
{	
    int client_id = 0;
    int ret = 0;
    uint32_t code;
    /* TODO the buf control here may have some bad thing */
    iofw_msg_unpack_func_code(msg, &code);
    client_id = msg->src;

    switch(code)
    {
	case FUNC_NC_CREATE: 
	    debug(DEBUG_USER,"server %d recv nc_create from client %d",
		    rank, client_id);
	    iofw_io_create(client_id);
	    debug(DEBUG_USER, "server %d done nc_create for client %d\n",
		    rank,client_id);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_DEF_DIM:
	    debug(DEBUG_USER,"server %d recv nc_def_dim from client %d",
		    rank, client_id);
	    iofw_io_def_dim(client_id);
	    debug(DEBUG_USER, "server %d done nc_def_dim for client %d\n",
		    rank,client_id);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_DEF_VAR:
	    debug(DEBUG_USER,"server %d recv nc_def_var from client %d",
		    rank, client_id);
	    iofw_io_def_var(client_id);
	    debug(DEBUG_USER, "server %d done nc_def_var for client %d\n",
		    rank,client_id);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_ENDDEF:
	    debug(DEBUG_USER,"server %d recv nc_enddef from client %d",
		    rank, client_id);
	    iofw_io_enddef(client_id);
	    debug(DEBUG_USER, "server %d done nc_enddef for client %d\n",
		    rank,client_id);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_PUT_VARA:
	    debug(DEBUG_USER,"server %d recv nc_put_vara from client %d",
		    rank, client_id);
	    iofw_io_put_vara(client_id);
	    debug(DEBUG_USER, 
		    "server %d done nc_put_vara_float from client %d\n", 
		    rank, client_id);
	    return IOFW_SERVER_ERROR_NONE;
	case FUNC_NC_CLOSE:
	    debug(DEBUG_USER,"server %d recv nc_close from client %d",
		    rank, client_id);
	    iofw_io_close(client_id);
	    debug(DEBUG_USER,"server %d received nc_close from client %d\n",
		    rank, client_id);
	    return IOFW_SERVER_ERROR_NONE;
	case CLIENT_END_IO:
	    debug(DEBUG_USER,"server %d recv client_end_io from client %d",
		    rank, client_id);
	    iofw_io_client_done(client_id, &server_done);
	    debug(DEBUG_USER, "server %d done client_end_io for client %d\n",
		    rank,client_id);
	    return IOFW_SERVER_ERROR_NONE;
	default:
	    error("server %d received unexpected msg from client %d",
		    rank, client_id);
	    return IOFW_SERVER_ERROR_UNEXPECTED_MSG;
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

    debug(DEBUG_USER, "Server(%d) Writer done", rank);
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

#ifndef SVR_RECV_ONLY
    if( (ret = pthread_create(&writer,NULL,iofw_writer,NULL))<0  )
    {
        error("Tread Writer create error()");
        return ret;
    }
#endif

    if( (ret = pthread_create(&reader,NULL,iofw_reader,NULL))<0  )
    {
        error("Tread Reader create error()");
        return ret;
    }

#ifndef SVR_RECV_ONLY
    pthread_join(writer, NULL);
#else
    pthread_join(reader, NULL);
#endif

    return 0;
}

static int _server_init(int argc, char** argv)
{
    int ret = 0;
    int x_proc_num, y_proc_num;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &server_proc_num);
    debug(DEBUG_USER, "rank = %d; size = %d", rank, server_proc_num);

    if(rank == 0)
    {
	set_debug_mask(DEBUG_MSG | DEBUG_USER | DEBUG_IO | DEBUG_ID | DEBUG_MAP);
	//set_debug_mask(DEBUG_ID);
	//set_debug_mask(DEBUG_TIME);
    }

    ret = MPI_Comm_get_parent(&inter_comm);

    if(ret != MPI_SUCCESS)
    {
	error("MPI_Comm_get_parent fail.");
	return IOFW_SERVER_ERROR_INIT_FAIL;
    }

    if(iofw_msg_init(SERVER_BUF_SIZE) < 0)
    {
	return IOFW_SERVER_ERROR_INIT_FAIL;
    }
    /**
     * init for client and server num
     **/
    x_proc_num = atoi(argv[1]);
    y_proc_num = atoi(argv[2]);
    debug(DEBUG_USER, "x_proc_num = %s; y_proc_num = %s", 
	    argv[1], argv[2]);
    //if(iofw_id_init(IOFW_ID_INIT_SERVER) < 0)
    //{
    //    error("ID Init Fail.");
    //    return IOFW_SERVER_ERROR_INIT_FAIL;
    //}

    //iofw_map_client_num(rank, size, &client_num);
    client_done = 0;
    server_done = 0;
    
    if(iofw_map_init(x_proc_num, y_proc_num, server_proc_num, inter_comm) < 0) 
    {
	error("Map Init fial.");
	return IOFW_SERVER_ERROR_INIT_FAIL;
    }
    if(iofw_id_init(IOFW_ID_INIT_SERVER) < 0)
    {
	error("ID init fail.");
	return IOFW_SERVER_ERROR_INIT_FAIL;
    }

    if(iofw_io_init(rank) < 0)
    {
	error("IO init fail.");
	return IOFW_SERVER_ERROR_INIT_FAIL;
    }

    return IOFW_SERVER_ERROR_NONE;

}

static void server_final()
{
    iofw_io_final();
    iofw_msg_final();
    iofw_id_final();

    MPI_Finalize();
}

int main(int argc, char** argv)
{

    times_init();
    times_start();

    _server_init(argc, argv);

    iofw_server();

    debug(DEBUG_TIME, "Proc %d : iofw_server total time : %f", rank, times_end());
    times_final();

    debug(DEBUG_USER, "Proc %d : wait for final", rank);
    MPI_Barrier(MPI_COMM_WORLD);
    
    debug(DEBUG_USER, "Proc %d : begin final", rank);
    server_final();
    return 0;
}

/****************************************************************************
 *       Filename:  server.c
 *
 *    Description:  main program for server
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
#include <string.h>

#include "server.h"
#include "map.h"
#include "io.h"
#include "id.h"
#include "mpi.h"
#include "debug.h"
#include "msg.h"
#include "times.h"
#include "define.h"
#include "error.h"

/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* my real rank in mpi_comm_world */
static int rank;
static int server_proc_num;	    /* server group size */

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
	    return IOFW_ERROR_NONE;
	case FUNC_NC_DEF_DIM:
	    debug(DEBUG_USER,"server %d recv nc_def_dim from client %d",
		    rank, client_id);
	    iofw_io_def_dim(client_id);
	    debug(DEBUG_USER, "server %d done nc_def_dim for client %d\n",
		    rank,client_id);
	    return IOFW_ERROR_NONE;
	case FUNC_NC_DEF_VAR:
	    debug(DEBUG_USER,"server %d recv nc_def_var from client %d",
		    rank, client_id);
	    iofw_io_def_var(client_id);
	    debug(DEBUG_USER, "server %d done nc_def_var for client %d\n",
		    rank,client_id);
	    return IOFW_ERROR_NONE;
	case FUNC_NC_ENDDEF:
	    debug(DEBUG_USER,"server %d recv nc_enddef from client %d",
		    rank, client_id);
	    iofw_io_enddef(client_id);
	    debug(DEBUG_USER, "server %d done nc_enddef for client %d\n",
		    rank,client_id);
	    return IOFW_ERROR_NONE;
	case FUNC_NC_PUT_VARA:
	    debug(DEBUG_USER,"server %d recv nc_put_vara from client %d",
		    rank, client_id);
	    iofw_io_put_vara(client_id);
	    debug(DEBUG_USER, 
		    "server %d done nc_put_vara_float from client %d\n", 
		    rank, client_id);
	    return IOFW_ERROR_NONE;
	case FUNC_NC_CLOSE:
	    debug(DEBUG_USER,"server %d recv nc_close from client %d",
		    rank, client_id);
	    iofw_io_close(client_id);
	    debug(DEBUG_USER,"server %d received nc_close from client %d\n",
		    rank, client_id);
	    return IOFW_ERROR_NONE;
	case CLIENT_END_IO:
	    debug(DEBUG_USER,"server %d recv client_end_io from client %d",
		    rank, client_id);
	    iofw_io_client_done(client_id, &server_done);
	    debug(DEBUG_USER, "server %d done client_end_io for client %d\n",
		    rank,client_id);
	    return IOFW_ERROR_NONE;
	default:
	    error("server %d received unexpected msg from client %d",
		    rank, client_id);
	    return IOFW_ERROR_UNEXPECTED_MSG;
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

    ret = pthread_cancel(reader);
    if(ret < 0)
    {
	printf("cancel : %s\n", strerror(ret));
    }else
    {
	printf("you are right\n");
    }
    debug(DEBUG_USER, "Server(%d) Writer done", rank);
    return ((void *)0);
}
static void* iofw_reader(void *argv)
{
    while(1)
    {
	iofw_msg_recv(rank, iofw_map_get_comm());
    }
    return ((void *)0);
}

int iofw_server_start()
{
    int ret = 0;

#ifndef SVR_RECV_ONLY
    if( (ret = pthread_create(&writer,NULL,iofw_writer,NULL))<0  )
    {
        error("Tread Writer create error()");
        return IOFW_ERROR_PTHREAD_CREATE;
    }
#endif

    if( (ret = pthread_create(&reader,NULL,iofw_reader,NULL))<0  )
    {
        error("Tread Reader create error()");
        return IOFW_ERROR_PTHREAD_CREATE;
    }

#ifndef SVR_RECV_ONLY
    pthread_join(writer, NULL);
#else
    pthread_join(reader, NULL);
#endif
    return IOFW_ERROR_NONE;
}

int iofw_server_init()
{
    int ret = 0;
    int x_proc_num, y_proc_num;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0)
    {
	//set_debug_mask(DEBUG_MSG | DEBUG_USER | DEBUG_IO | DEBUG_ID | DEBUG_MAP);
	//set_debug_mask(DEBUG_ID);
	//set_debug_mask(DEBUG_TIME);
    }

    if((ret = iofw_msg_init(SERVER_BUF_SIZE)) < 0)
    {
	error("");
	return ret;
    }
    /**
     * init for client and server num
     **/
    server_done = 0;
    
    if((ret = iofw_id_init(IOFW_ID_INIT_SERVER)) < 0)
    {
	error("");
	return ret;
    }

    if((ret = iofw_io_init(rank)) < 0)
    {
	error("");
	return ret;
    }

    return IOFW_ERROR_NONE;
}

int iofw_server_final()
{
    iofw_io_final();
    iofw_id_final();
    iofw_msg_final();

    return IOFW_ERROR_NONE;
}

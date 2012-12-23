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
#include "recv.h"
#include "io.h"
#include "id.h"
#include "mpi.h"
#include "debug.h"
#include "times.h"
#include "define.h"
#include "cfio_error.h"

/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* my real rank in mpi_comm_world */
static int rank;
static int server_proc_num;	    /* server group size */

static int reader_done, writer_done;

static int decode(cfio_msg_t *msg)
{	
    int client_id = 0;
    int ret = 0;
    uint32_t code;
    /* TODO the buf control here may have some bad thing */
    cfio_recv_unpack_func_code(msg, &code);
    client_id = msg->src;

    switch(code)
    {
	case FUNC_NC_CREATE: 
	    debug(DEBUG_SERVER,"server %d recv nc_create from client %d",
		    rank, client_id);
	    cfio_io_create(msg);
	    debug(DEBUG_SERVER, "server %d done nc_create for client %d\n",
		    rank,client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_NC_DEF_DIM:
	    debug(DEBUG_SERVER,"server %d recv nc_def_dim from client %d",
		    rank, client_id);
	    cfio_io_def_dim(msg);
	    debug(DEBUG_SERVER, "server %d done nc_def_dim for client %d\n",
		    rank,client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_NC_DEF_VAR:
	    debug(DEBUG_SERVER,"server %d recv nc_def_var from client %d",
		    rank, client_id);
	    cfio_io_def_var(msg);
	    debug(DEBUG_SERVER, "server %d done nc_def_var for client %d\n",
		    rank,client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_PUT_ATT:
	    debug(DEBUG_SERVER, "server %d recv nc_put_att from client %d",
		    rank, client_id);
	    cfio_io_put_att(msg);
	    debug(DEBUG_SERVER, "server %d done nc_put_att from client %d",
		    rank, client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_NC_ENDDEF:
	    debug(DEBUG_SERVER,"server %d recv nc_enddef from client %d",
		    rank, client_id);
	    cfio_io_enddef(msg);
	    debug(DEBUG_SERVER, "server %d done nc_enddef for client %d\n",
		    rank,client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_NC_PUT_VARA:
	    debug(DEBUG_SERVER,"server %d recv nc_put_vara from client %d",
		    rank, client_id);
	    cfio_io_put_vara(msg);
	    debug(DEBUG_SERVER, 
		    "server %d done nc_put_vara_float from client %d\n", 
		    rank, client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_NC_CLOSE:
	    debug(DEBUG_SERVER,"server %d recv nc_close from client %d",
		    rank, client_id);
	    cfio_io_close(msg);
	    debug(DEBUG_SERVER,"server %d received nc_close from client %d\n",
		    rank, client_id);
	    return CFIO_ERROR_NONE;
	case FUNC_END_IO:
	    debug(DEBUG_SERVER,"server %d recv client_end_io from client %d",
		    rank, msg->src);
	    cfio_io_reader_done(msg->src, &reader_done);
	    debug(DEBUG_SERVER, "server %d done client_end_io for client %d\n",
		    rank,msg->src);
	    return CFIO_ERROR_NONE;
	default:
	    error("server %d received unexpected msg from client %d",
		    rank, client_id);
	    return CFIO_ERROR_UNEXPECTED_MSG;
    }	
}

static void * cfio_reader(void *argv)
{
    int ret = 0;
    cfio_msg_t *msg;
    
    while(!reader_done)
    {
        msg = cfio_recv_get_first();
        if(NULL != msg)
        {
            decode(msg);
        }
	free(msg);
    }
    
    debug(DEBUG_SERVER, "Server(%d) Reader done", rank);
    return ((void *)0);
}
static void* cfio_writer(void *argv)
{
    cfio_msg_t *msg;
    int i, server_index, client_num;
    uint32_t func_code;
    int *client_id;

    server_index = cfio_map_get_server_index(rank);
    client_num = cfio_map_get_client_num_of_server(rank);
    client_id = malloc(sizeof(int) * client_num);
    if(client_id == NULL)
    {
	error("malloc fail.");
	return (void*)0;
    }
    cfio_map_get_clients(rank, client_id);

    while(!writer_done)
    {
	/*  recv from client one by one, to make sure that data recv and output in time */
	for(i = 0; i < client_num; i ++)
	{
	    cfio_recv(client_id[i], rank, cfio_map_get_comm(), &func_code);
	    if(func_code == FUNC_END_IO)
	    {
		debug(DEBUG_SERVER,"server(writer) %d recv client_end_io from client %d",
			rank, client_id[i]);
		cfio_io_writer_done(client_id[i], &writer_done);
		debug(DEBUG_SERVER, "server(writer) %d done client_end_io for client %d\n",
			rank,client_id[i]);
	    }
	}
    }
    debug(DEBUG_SERVER, "Server(%d) Writer done", rank);
    return ((void *)0);
}

int cfio_server_start()
{
    int ret = 0;

#ifndef SVR_RECV_ONLY
    if( (ret = pthread_create(&reader,NULL,cfio_reader,NULL))<0  )
    {
        error("Thread Writer create error()");
        return CFIO_ERROR_PTHREAD_CREATE;
    }
#endif

    if( (ret = pthread_create(&writer,NULL,cfio_writer,NULL))<0  )
    {
        error("Thread Reader create error()");
        return CFIO_ERROR_PTHREAD_CREATE;
    }

#ifndef SVR_RECV_ONLY
    pthread_join(reader, NULL);
#endif

    pthread_join(writer, NULL);
    return CFIO_ERROR_NONE;
}

int cfio_server_init()
{
    int ret = 0;
    int x_proc_num, y_proc_num;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if((ret = cfio_recv_init()) < 0)
    {
	error("");
	return ret;
    }
    
    reader_done = 0;
    writer_done = 0;
    
    if((ret = cfio_id_init(CFIO_ID_INIT_SERVER)) < 0)
    {
	error("");
	return ret;
    }

    if((ret = cfio_io_init(rank)) < 0)
    {
	error("");
	return ret;
    }

    return CFIO_ERROR_NONE;
}

int cfio_server_final()
{
    cfio_io_final();
    cfio_id_final();
    cfio_recv_final();

    return CFIO_ERROR_NONE;
}

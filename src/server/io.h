/*
 * =====================================================================================
 *
 *       Filename:  unmap.h
 *
 *    Description: unmap the io_forward op into a real operation 
 *
 *        Version:  1.0
 *        Created:  12/20/2011 04:54:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#ifndef _UNMAP_H
#define _UNMAP_H
#include "buffer.h"
#include "mpi.h"

/* return codes */
#define	ALL_CLINET_REPORT_DONE 1
/* the msg is delt, the buffer could be reused inmmediately */
#define DEALT_MSG 2
/* the msg is put into the queue, the buffer should be keep util
 * the write to free it */
#define ENQUEUE_MSG 3
#define IMM_MSG 4

#define IOFW_IO_ERROR_NONE	     0
#define IOFW_IO_ERROR_NC	    -1	/* nc operation error */
#define IOFW_IO_ERROR_INVALID_NC    -2	/* invalid nc id */
#define IOFW_IO_ERROR_INVALID_DIM   -3	/* invalid dimension id */
#define IOFW_IO_ERROR_INVALID_VAR   -4	/* invalid variable id */
#define IOFW_IO_ERROR_MSG_UNPACK    -5
#define IOFW_IO_ERROR_PUT_VAR	    -6

/**
 * @brief: initialize
 *
 * @param _server_group_size: size of the server group
 *
 * @return: error code
 */
int iofw_io_init(int _server_group_size)
/**
 * @brief: finalize
 *
 * @return: error code
 */
int iofw_io_final();
int iofw_io_client_done(int *client_done, int *server_done,
	int client_to_serve);
int iofw_io_nc_create(int client_proc);
int iofw_io_nc_def_dim(int client_proc);
int iofw_io_nc_def_var(int client_proc);
int iofw_io_nc_enddef(int client_proc);
int iofw_io_nc_put_var1_float(int client_proc);
int iofw_io_nc_put_vara_float(int client_proc);
int iofw_io_nc_close(int client_proc);

#endif

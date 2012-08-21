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
#include "quicklist.h"

#define IO_HASH_TABLE_SIZE 32

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
#define IOFW_IO_ERROR_WRONG_NDIMS   -7  /* Wrong ndims in put var */
#define IOFW_IO_ERROR_NC_NOT_DEFINE -8  /* nc file is not in DEFINE_MODE, some IO
					   function only can be called in 
					   DEFINE_MODE*/

typedef struct
{
    int func_code;
    int client_nc_id;
    int client_dim_id;
    int client_var_id;
}iofw_io_key_t;

typedef struct
{
    int func_code;
    int client_nc_id;
    int client_dim_id;
    int client_var_id;

    uint8_t *client_bitmap;

    qlist_head_t hash_link;
    //qlist_head_t queue_link;
}iofw_io_val_t;

/**
 * @brief: initialize
 *
 * @param _server_group_size: size of the server group
 *
 * @return: error code
 */
int iofw_io_init();
/**
 * @brief: finalize
 *
 * @return: error code
 */
int iofw_io_final();
int iofw_io_client_done(int client_id, int *server_done);
int iofw_io_nc_create(int client_proc);
int iofw_io_nc_def_dim(int client_proc);
int iofw_io_nc_def_var(int client_proc);
int iofw_io_nc_enddef(int client_proc);
int iofw_io_nc_put_vara(int client_proc);
int iofw_io_nc_close(int client_proc);

#endif

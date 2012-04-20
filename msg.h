/****************************************************************************
 *       Filename:  msg.h
 *
 *    Description:  define for msg between IO process and IO forwarding process
 *
 *        Version:  1.0
 *        Created:  12/13/2011 10:39:50 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _MSG_H
#define _MSG_H
#include <stdlib.h>

#include "mpi.h"
#include "netcdf.h"
#include "buffer.h"
#include "quicklist.h"

#define MSG_MAX_SIZE 1048576
/* define for control messge */
#define CLIENT_END_IO 201

#define CLIENT_BUF_SIZE 1024*1024*1024

/**
 *define for nc function code
 **/
#define FUNC_NC_CREATE		((uint32_t)1)
#define FUNC_NC_ENDDEF		((uint32_t)2)
#define FUNC_NC_CLOSE  ((uint32_t) 303)
#define FUNC_NC_DEF_DIM ((uint32_t)10)
#define FUNC_NC_DEF_VAR ((uint32_t)20)
#define FUNC_NC_PUT_VAR1_FLOAT ((uint32_t)100)
#define FUNC_NC_PUT_VARA_FLOAT ((uint32_t)110)

#define IOFW_MSG_ERROR_NONE	0
#define IOFW_MSG_ERROR_BUFFER	1   /* buffer error */
#define IOFW_MSG_ERROR_MALLOC	2   /* malloc error */
#define IOFW_MSG_ERROR_MPI	3   /* mpi recv error */

typedef struct
{
    size_t size;	/* size of the msg */
    char *addr;		/* pointer to the data buffer of the msg */   
    int src;		/* id of sending msg proc */
    int dst;		/* id of dst porc */  
    MPI_Request req;	/* MPI request of the send msg */
    qlist_head_t link;	/* quicklist head */
}iofw_msg_t;

/**
 * @brief: init the buffer and msg queue
 *
 * @return: error code
 */
int iofw_msg_init();
/**
 * @brief: finalize , free the buffer and msg queue
 *
 * @return: error code
 */
int iofw_msg_final();
/**
 * @brief: async send msg to other proc by mpi
 *
 * @param msg: point to the msg which is to be send
 * @param comm: MPI communicator
 *
 * @return: error code
 */
int iofw_msg_isend(iofw_msg_t *msg,  MPI_Comm comm);
/**
 * @brief: recv msg from client
 *
 * @param rank: the rank of server who recv the msg
 * @param comm: MPI communicator
 *
 * @return: error code
 */
int iofw_msg_recv(int rank, MPI_Comm comm);

/**
 * @brief: get the first msg in msg queue
 *
 * @return: pointer to the first msg
 */
iofw_msg_t *iofw_msg_get_first();
/**
 * @brief: pack iofw_nc_create function into struct iofw_msg_t
 *
 * @param _msg: pointer to the struct iofw_msg_t
 * @param client_proc_id: id of client proc who call the function
 * @param path: the file name of the new netCDF dataset, iofw_nc_create's
 *	arg
 * @param cmode: the creation mode flag , arg of iofw_nc_create
 *
 * @return: error code
 */
int iofw_msg_pack_nc_create(
	iofw_msg_t **_msg, int client_proc_id, 
	const char *path, int cmode);
/**
 * @brief: pack iofw_nc_def_dim into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: NetCDF group ID, arg of iofw_nc_def_dim
 * @param name: Dimension name, arg of iofw_nc_def_dim
 * @param len: Length of dimension, arg of iofw_nc_def_dim
 *
 * @return: error_code
 */
int iofw_msg_pack_nc_def_dim(
	iofw_msg_t **_msg, int client_proc_id, 
	int ncid, const char *name, size_t len);
/**
 * @brief: pack iofw_nc_def_var into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_nc_def_var
 * @param name: variable name, arg of iofw_nc_def_var
 * @param xtype: predefined netCDF external data type, arg of 
 *	iofw_nc_def_var
 * @param ndims: number of dimensions for the variable, arg of 
 *	iofw_nc_def_var
 * @param dimids: vector of ndims dimension IDs corresponding to the
 *	variable dimensions, arg of iofw_nc_def_var
 *
 * @return: error code
 */
int iofw_msg_pack_nc_def_var(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids);
/**
 * @brief: pack iofw_nc_enddef into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_nc_enddef
 *
 * @return: error code
 */
int iofw_msg_pack_nc_enddef(
	iofw_msg_t **_msg, int client_proc_id, 
	int ncid);
/**
 * @brief: pack iofw_nc_put_var1_float into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of nc_put_var1_float
 * @param varid: variable ID, arg of nc_put_var1_float
 * @param dim: the dimensionality fo variable, arg of nc_put_var1_float
 * @param index: the index of the data value to be written, arg of 
 *	nc_put_var1_float
 * @param fp: pinter to the data value to be written, arg of 
 *	nc_put_var1_float
 *
 * @return: error code
 */
int iofw_msg_pack_nc_put_var1_float(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int dim,
	const size_t *index, const float *fp);
/**
 * @brief: pack iofw_nc_put_vara_float into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_nc_put_vara_float
 * @param varid: variable ID, arg of iofw_nc_put_vara_float
 * @param dim: the dimensionality fo variable
 * @param start: a vector of size_t intergers specifying the index in 
 *	the variable where the first of the data values will be written,
 *	arg of iofw_nc_put_vara_float
 * @param count: a vector of size_t intergers specifying the edge lengths
 *	along each dimension of the block of data values to be written, 
 *	arg of iofw_nc_put_vara_float
 * @param fp : pointer to where data is stored, arg of 
 *	iofw_nc_put_vara_float
 *
 * @return: 
 */
int iofw_msg_pack_nc_put_vara_float(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp);
/**
 * @brief: pack iofw_nc_close into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_nc_close
 *
 * @return: error code
 */
int iofw_msg_pack_nc_close(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid);

/**
 * @brief: pack a special msg, the msg will inform the server that one
 *	client's IO is over
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 *
 * @return: error code
 */
int iofw_msg_pack_io_done(
	iofw_msg_t **_msg, int client_proc_id);
/**
 * @brief: unpack funciton code from the buffer
 *
 * @param msg: pointer to the recv msg
 * @param func_code: the code of fucntion
 *
 * @return: error code
 */
int iofw_msg_unpack_func_code(
	iofw_msg_t *msg,
	uint32_t *func_code);
/**
 * @brief: unpack arguments for ifow_nc_create function
 *
 * @param path: poiter to where the file anme of the new netCDF dataset is to be 
 *	stored
 * @param cmode: pointer to where the creation mode flag is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_create(
	char **path, int *cmode);
/**
 * @brief: unpack arguments for ifow_nc_def_dim function
 *
 * @param ncid: pointer to where NetCDF group ID is to be stored
 * @param name: pointer to where dimension name is to be stored
 * @param len: pointer to where length of dimension is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_def_dim(
	int *ncid, char **name, size_t *len);
/**
 * @brief: unpack arguments for the iofw_nc_def_var function
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param name: pointer to where variable name is to be stored, need to be freed by
 *	the caller
 * @param xtype: pointer to where netCDF external data types is to be stored
 * @param ndims: pointer to where number of dimensions for the variable is to be 
 *	stored
 * @param dimids: pointer to where vector of ndims dimension IDs corresponding to
 *      the variable dimensions is to be stored, need to be freed by the caller
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_def_var(
	int *ncid, char **name, nc_type *xtype,
	unsigned int *ndims, int **dimids);
/**
 * @brief: unpack arguments for the iofw_nc_enddef function 
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_enddef(
	int *ncid);
/**
 * @brief: unpack arguments for the iofw_nc_put_var1_float function
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param varid: pointer to where variable ID is to be stored 
 * @param indexdim: pointer to where the dimensionality of the to be written 
 *	variable is to be stored 
 * @param index: pointer to where the index of the to be written data value to be 
 *	stored
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_put_var1_float(
	int *ncid, int *varid, unsigned int *indexdim, size_t **index);
/**
 * @brief: unpack arguments for function : iofw_msg_unpack_put_vara_float
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param varid: pointer to where variable ID is to be stored 
 * @param dim: pointer to where the dimensionality of to be written variable
 *	 is to be stored 
 * @param start: pointer to where the start index of to be written data value 
 *	to be stored
 * @param count: pointer to where the size of to be written data dimension len
 * value to be stored
 * @param data_len: pointer to the size of data 
 * @param fp: where the data is stored
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_put_vara_float(
	int *ncid, int *varid, unsigned int *dim, 
	size_t **start, size_t **count,
	unsigned int *data_len, float **fp);
/**
 * @brief: unpack arguments for the iofw_nc_close function
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_nc_close(int *ncid);

#endif

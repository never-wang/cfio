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

#define MSG_MAX_SIZE 1024*1024*256
/* define for control messge */

/**
 *define for nc function code
 **/
#define FUNC_NC_CREATE		((uint32_t)1)
#define FUNC_NC_ENDDEF		((uint32_t)2)
#define FUNC_NC_CLOSE		((uint32_t)3)
#define FUNC_NC_DEF_DIM		((uint32_t)11)
#define FUNC_NC_DEF_VAR		((uint32_t)12)
#define FUNC_PUT_ATT		((uint32_t)13)
#define FUNC_NC_PUT_VARA	((uint32_t)20)
#define FUNC_END_IO		((uint32_t)40)
/* below two are only used in io.c */
#define FUNC_READER_END_IO		((uint32_t)41)
#define FUNC_WRITER_END_IO		((uint32_t)42)

typedef struct
{
    uint32_t func_code;	/* function code , like FUNC_NC_CREATE */
    size_t size;	/* size of the msg */
    char *addr;		/* pointer to the data buffer of the msg */   
    int src;		/* id of sending msg proc */
    int dst;		/* id of dst porc */  
    MPI_Comm comm;	/* communication  */
    MPI_Request req;	/* MPI request of the send msg */
    qlist_head_t link;	/* quicklist head */
}iofw_msg_t;

/**
 * @brief: init the buffer and msg queue
 *
 * @param buffer_size: msg buffer size
 *
 * @return: error code
 */
int iofw_msg_init(int buffer_size);
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
 *
 * @return: error code
 */
int iofw_msg_isend(iofw_msg_t *msg);
/**
 * @brief: recv msg from client
 *
 * @param rank: the rank of server who recv the msg
 * @param comm: MPI communicator
 * @param _msg: point to the msg which is recieved
 *
 * @return: error code
 */
int iofw_msg_recv(int rank, MPI_Comm comm, iofw_msg_t **_msg);

/**
 * @brief: get the first msg in msg queue
 *
 * @return: pointer to the first msg
 */
iofw_msg_t *iofw_msg_get_first();
/**
 * @brief: pack iofw_create function into struct iofw_msg_t
 *
 * @param _msg: pointer to the struct iofw_msg_t
 * @param client_proc_id: id of client proc who call the function
 * @param path: the file name of the new netCDF dataset, iofw_create's
 *	arg
 * @param cmode: the creation mode flag , arg of iofw_create
 * @param ncid: ncid assigned by client
 *
 * @return: error code
 */
int iofw_msg_pack_create(
	iofw_msg_t **_msg, int client_proc_id, 
	const char *path, int cmode, int ncid);
/**
 * @brief: pack iofw_def_dim into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: NetCDF group ID, arg of iofw_def_dim
 * @param name: Dimension name, arg of iofw_def_dim
 * @param len: Length of dimension, arg of iofw_def_dim
 * @param dimid: dim id assigned by client
 *
 * @return: error_code
 */
int iofw_msg_pack_def_dim(
	iofw_msg_t **_msg, int client_proc_id, 
	int ncid, const char *name, size_t len, int dimid);
/**
 * @brief: pack iofw_def_var into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_def_var
 * @param name: variable name, arg of iofw_def_var
 * @param xtype: predefined netCDF external data type, arg of 
 *	iofw_def_var
 * @param ndims: number of dimensions for the variable, arg of 
 *	iofw_def_var
 * @param dimids: vector of ndims dimension IDs corresponding to the
 *	variable dimensions, arg of iofw_def_var
 * @param start: a vector of size_t intergers specifying the index in 
 *	the variable where the first of the data values will be written,
 *	arg of iofw_put_vara_float
 * @param count: a vector of size_t intergers specifying the edge lengths
 *	along each dimension of the block of data values to be written, 
 *	arg of iofw_put_vara_float
 * @param varid: var id assigned by client
 *
 * @return: error code
 */
int iofw_msg_pack_def_var(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, int varid);
/**
 * @brief: pack iofw_put_att into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_put_att
 * @param varid: variable ID, arg of iofw_put_att
 * @param name: variable name, arg of iofw_put_att
 * @param xtype: predefined netCDF external data type, arg of iofw_put_att
 * @param len: number of values provided for the attribute
 * @param op: Pointer to values
 *
 * @return: error code
 */
int iofw_msg_pack_put_att(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, const char *name, 
	nc_type xtype, size_t len, const void *op);
/**
 * @brief: pack iofw_enddef into msg
 
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_enddef
 *
 * @return: error code
 */
int iofw_msg_pack_enddef(
	iofw_msg_t **_msg, int client_proc_id, 
	int ncid);
/**
 * @brief: pack iofw_put_vara_float into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_put_vara_float
 * @param varid: variable ID, arg of iofw_put_vara_float
 * @param ndims: the dimensionality fo variable
 * @param start: a vector of size_t intergers specifying the index in 
 *	the variable where the first of the data values will be written,
 *	arg of iofw_put_vara_float
 * @param count: a vector of size_t intergers specifying the edge lengths
 *	along each dimension of the block of data values to be written, 
 *	arg of iofw_put_vara_float
 * @param fp_type: type of data, can be IOFW_BYTE, IOFW_CHAR, IOFW_SHROT, 
 *	IOFW_INT, IOFW_FLOAT, IOFW_DOUBLE
 * @param fp : pointer to where data is stored, arg of 
 *	iofw_put_vara_float
 *
 * @return: 
 */
int iofw_msg_pack_put_vara(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int ndims,
	const size_t *start, const size_t *count, 
	const int fp_type, const void *fp);
/**
 * @brief: pack iofw_close into msg
 *
 * @param _msg: pointer to the msg
 * @param client_proc_id: id of client proc who call the function
 * @param ncid: netCDF ID, arg of iofw_close
 *
 * @return: error code
 */
int iofw_msg_pack_close(
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
 * @brief: unpack arguments for ifow_create function
 *
 * @param path: poiter to where the file anme of the new netCDF dataset is to be 
 *	stored
 * @param cmode: pointer to where the creation mode flag is to be stored
 * @param ncid: pointer to where the ncid assigned by client is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_create(
	char **path, int *cmode, int *ncid);
/**
 * @brief: unpack arguments for ifow_def_dim function
 *
 * @param ncid: pointer to where NetCDF group ID is to be stored
 * @param name: pointer to where dimension name is to be stored
 * @param len: pointer to where length of dimension is to be stored
 * @param dimid: pointer to where the dimid assigned by client is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_def_dim(
	int *ncid, char **name, size_t *len,int *dimid);
/**
 * @brief: unpack arguments for the iofw_def_var function
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param name: pointer to where variable name is to be stored, need to be freed by
 *	the caller
 * @param xtype: pointer to where netCDF external data types is to be stored
 * @param ndims: pointer to where number of dimensions for the variable is to be 
 *	stored
 * @param dimids: pointer to where vector of ndims dimension IDs corresponding to
 *      the variable dimensions is to be stored, need to be freed by the caller
 * @param start: pointer to where the start index of to be written data value 
 *	to be stored, need to be freed by the caller
 * @param count: pointer to where the size of to be written data dimension len
 *	value to be stored, need to be freed by the caller
 * @param varid: pointer to where the varid assigned by client is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_def_var(
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids, 
	size_t **start, size_t **count, int *varid);
/**
 * @brief: unpack arguments for iofw_put_att
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param varid: pointer to where variable ID is to be stored 
 * @param name: pointer to where variable name is to be stored, need to be freed by
 *	the caller
 * @param xtype: pointer to where netCDF external data types is to be stored
 * @param len: pointer to where number of values provided for the attribute is to be
 *	stored
 * @param op: Pointer to values
 *
 * @return: 
 */
int iofw_msg_unpack_put_att(
	int *ncid, int *varid, char **name, 
	nc_type *xtype, int *len, void **op);
/**
 * @brief: unpack arguments for the iofw_enddef function 
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_enddef(
	int *ncid);
/**
 * @brief: unpack arguments for function : iofw_msg_unpack_put_vara_**
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param varid: pointer to where variable ID is to be stored 
 * @param ndims: pointer to where the dimensionality of to be written variable
 *	 is to be stored 
 * @param start: pointer to where the start index of to be written data value 
 *	to be stored
 * @param count: pointer to where the size of to be written data dimension len
 *	value to be stored
 * @param data_len: pointer to the size of data 
 * @param fp_type: pointer to type of data, can be IOFW_BYTE, IOFW_CHAR, 
 *	IOFW_SHROT, IOFW_INT, IOFW_FLOAT, IOFW_DOUBLE
 * @param fp: where the data is stored
 *
 * @return: error code
 */
int iofw_msg_unpack_put_vara(
	int *ncid, int *varid, int *ndims, 
	size_t **start, size_t **count,
	int *data_len, int *fp_type, char **fp);
/**
 * @brief: unpack arguments for the iofw_close function
 *
 * @param ncid: pointer to where netCDF ID is to be stored
 *
 * @return: error code
 */
int iofw_msg_unpack_close(int *ncid);

#endif

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
#include "pack.h"

/**
 *define for nc function code
 **/
#define FUNC_NC_CREATE 1
#define FUNC_NC_ENDDEF 2
#define FUNC_NC_CLOSE 3
#define FUNC_NC_DEF_DIM 10
#define FUNC_NC_DEF_VAR 20
#define FUNC_NC_PUT_VAR1_FLOAT 100

/**
 * @brief: send msg to othre proc by mpi
 *
 * @param dst_proc_id: the destination proc id 
 * @param buf: pointer to the struct buf which store the data of send msg
 *
 * @return: 0 if success
 */
int iofw_send_msg(int dst_proc_id, iofw_buf_t *buf);
/**
 * @brief: recieve an interger from other proc
 *
 * @param src_por_id: source proc id
 * @param data: pointer to location where received data is to be stored
 *
 * @return: 0 if success
 */
int iofw_recv_int1(int src_por_id, int *data);
/**
 * @brief: send an interter to other proc
 * @param des_por_id: destination
 * @param my_rank: self rank
 * @param data: the data to send
 * @return: 0 if success
 */
int iofw_send_int1(int des_por_id, int my_rank, int data);

/**
 * @brief: pack for ifow_nc_create function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param path: the file name of the new netCDF dataset
 * @param cmode: the creation mode flag
 *
 * @return: 0 if success
 */
int iofw_pack_msg_create(
	iofw_buf_t *buf,
	const char *path, int cmode);
/**
 * @brief: pack for the iofw_nc_def_dim function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: NetCDF group ID
 * @param name: Dimension name
 * @param len: Length of dimension
 *
 * @return: 0 if success
 */
int iofw_pack_msg_def_dim(
	iofw_buf_t *buf,
	int ncid, const char *name, size_t len);
/**
 * @brief: pack for the iofw_nc_def_var function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: netCDF ID
 * @param name: variable name
 * @param xtype: one of the set of predefined netCDF external data types
 * @param ndims: number of dimensions for the variable
 * @param dimids[]: vector of ndims dimension IDs corresponding to the 
 *	variable dimensions
 *
 * @return: 0 if success
 */
int iofw_pack_msg_def_var(
	iofw_buf_t *buf,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int dimids[]);
/**
 * @brief: pack for the iofw_nc_enddef function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_pack_msg_enddef(
	iofw_buf_t *buf,
	int ncid);
/**
 * @brief: pack for the iofw_nc_put_var1_float
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: netCDF ID
 * @param varid: variable ID
 * @param index[]: the index of the data value to be written
 * @param fp: pinter to the data value to be written
 *
 * @return: 0 if success
 */
int iofw_pack_msg_put_var1_float(
	iofw_buf_t *buf,
	int ncid, int varid, const size_t index[],
	const float *fp);
/**
 * @brief: pack for the iofw_nc_close function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_pack_msg_close(
	iofw_buf_t *buf,
	int ncid);

/**
 * @brief iofw_pack_msg_io_stop : tell the server the client
 * is finish
 *
 * @param buf: where the packed function is stored
 *
 * @return 0 if success , < 0 for failure 
 */
int iofw_pack_msg_io_stop(
	iofw_buf_t *buf);
/**
 * @brief: unpack funciton code from the msg 
 *
 * @param buf: pointer the struct buf where the packed function is stored
 *
 * @return: the code of function
 */
int iofw_unpack_msg_func_code(
	iofw_buf_t *buf);
/**
 * @brief: unpack arguments for ifow_nc_create function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param path: poiter to where the file anme of the new netCDF dataset is to be 
 *	stored
 * @param cmode: pointer to where the creation mode flag is to be stored
 *
 * @return: 0 if success
 */
int iofw_unpack_msg_create(
	iofw_buf_t *buf,
	char **path, int *cmode);
/**
 * @brief: unpack arguments for ifow_nc_def_dim function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: pointer to where NetCDF group ID is to be stored
 * @param name: pointer to where dimension name is to be stored
 * @param len: pointer to where length of dimension is to be stored
 *
 * @return: 0 if success
 */
int iofw_unpack_msg_def_dim(
	iofw_buf_t *buf,
	int *ncid, char **name, size_t *len);
/**
 * @brief: unpack arguments for the iofw_nc_def_var function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored, the name, and dimids need to be freed by the caller
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param name: pointer to where variable name is to be stored, need to be freed by
 *	the caller
 * @param xtype: pointer to where netCDF external data types is to be stored
 * @param ndims: pointer to where number of dimensions for the variable is to be 
 *	stored
 * @param dimids: pointer to where vector of ndims dimension IDs corresponding to
 *      the variable dimensions is to be stored, need to be freed by the caller
 *
 * @return: 0 if success
 */
int iofw_unpack_msg_def_var(
	iofw_buf_t *buf,
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids);
/**
 * @brief: unpack arguments for the iofw_nc_enddef function 
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: pointer to where netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int iofw_unpack_msg_enddef(
	iofw_buf_t *buf,
	int *ncid);
/**
 * @brief: unpack arguments for the iofw_nc_put_var1_float function
 *
 * @param buf: pointer to the struct buf where the packed function is 
 *	stored
 * @param ncid: pointer to where netCDF ID is to be stored
 * @param varid: pointer to where variable ID is to be stored 
 * @param indexdim: pointer to where the dimensionality of the to be written data 
 *	value's index is to be stored 
 * @param index: pointer to where the index of the to be written data value to be 
 *	stored
 *
 * @return: 0 if success
 */
int iofw_unpack_msg_put_var1_float(
	iofw_buf_t *buf,
	int *ncid, int *varid, int *indexdim, size_t *index);
/**
 * @brief: unpack arguments for the iofw_nc_close function
 *
 * @param buf: pointer to the struct buf where the packed function is stored
 * @param ncid: pointer to where netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int iofw_unpack_msg_close(
	    iofw_buf_t *buf,
	    int *ncid);
#endif

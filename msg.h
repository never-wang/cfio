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
 * @brief: pack for ifow_nc_create function
 *
 * @param buf: pointer to the struct buf where the packed function information is 
 *	stored
 * @param path: the file anme of the new netCDF dataset
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
 * @param buf: pointer to the struct buf where the packed function information is 
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
 * @param buf: pointer to the struct buf where the packed function information is 
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
 * @param buf: pointer to the struct buf where the packed function information is 
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
 * @param buf: pointer to the struct buf where the packed function information is 
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
 * @param buf: pointer to the struct buf where the packed function information is 
 *	stored
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_pack_msg_close(
	iofw_buf_t *buf,
	int ncid);

#endif

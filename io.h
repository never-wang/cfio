/****************************************************************************
 *       Filename:  io.h
 *
 *    Description:  IO interface
 *
 *        Version:  1.0
 *        Created:  12/13/2011 03:53:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _IO_H
#define _IO_H
#include <stdlib.h>

#include <netcdf.h>
/**
 * @brief: nc_create
 *
 * @param io_proc_id: id of IO proc
 * @param path: the file anme of the new netCDF dataset
 * @param cmode: the creation mode flag
 * @param ncidp: pointer to location where returned netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int iofw_nc_create(
	const int io_proc_id,
	const char *path, int cmode, int *ncidp);
/**
 * @brief: nc_def_dim
 *
 * @param io_proc_id: id of IO proc 
 * @param ncid: NetCDF group ID
 * @param name: Dimension name
 * @param len: Length of dimension
 * @param idp: pointer to location for returned dimension ID
 *
 * @return: 0 if success
 */
int iofw_nc_def_dim(
	const int io_proc_id,
	int ncid, const char *name, size_t len, int *idp);
/**
 * @brief: nc_def_var
 *
 * @param io_proc_id: id of proc id 
 * @param ncid: netCDF ID
 * @param name: variable name
 * @param xtype: one of the set of predefined netCDF external data types
 * @param ndims: number of dimensions for the variable
 * @param dimids[]: vector of ndims dimension IDs corresponding to the 
 *	variable dimensions
 * @param varidp: pointer to location for the returned variable ID
 *
 * @return: 0 if success
 */
int iofw_nc_def_var(
	int io_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int dimids[], int *varidp);

/**
 * @brief: nc_enddef
 *
 * @param io_proc_id: id of io proc
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_nc_enddef(
	int io_proc_id,
	int ncid);
/**
 * @brief: nc_put_var_float
 *
 * @param io_proc_id: id of IO proc
 * @param ncid: netCDF ID
 * @param varid: variable ID
 * @param index: the index of the data value to be written
 * @param fp: pinter to the data value to be written
 *
 * @return: 0 if success
 */
int iofw_nc_put_var1_float(
	int io_proc_id,
	int ncid, int varid, int dim,
	const size_t index, const float *fp);
/**
 * @brief: nc_put_vara_float
 *
 * @param io_proc_id: id of IO proc
 * @param ncid: netCDF ID
 * @param varid: variable ID
 * @param dim: the dimensionality fo variable
 * @param start: a vector of size_t intergers specifying the index in the variable
 *	where the first of the data values will be written
 * @param count: a vector of size_t intergers specifying the edge lengths along 
 *	each dimension of the block of data values to be written
 * @param fp: pinter to the data value to be written
 *
 * @return: 
 */
int iofw_nc_put_vara_float(
	int io_proc_id,
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp);
/**
 * @brief: nc_close
 *
 * @param io_proc_id: 
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_nc_close(
	int io_proc_id,
	int ncid);

/**
 * @brief iofw_io_stop : tell the server the client is over
 *
 * @param s_rank: the rank of the server
 *
 * @return : 0 for success , < for failure
 */
int iofw_io_stop(int s_rank);
#endif

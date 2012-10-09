/*
 * =====================================================================================
 *
 *       Filename:  iofw.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/19/2011 06:29:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#ifndef _IO_FW_H
#define	_IO_FW_H

#include <stdlib.h>
#include <netcdf.h>

#include "mpi.h"
#include "iofw_types.h"

#define IOFW_ERROR_NONE		    0
#define IOFW_ERROR_TOO_MANY_OPEN    1 /* Too Many Open NC File */
#define IOFW_ERROR_INIT		    2 /* error when init */
#define IOFW_ERROR_NULL_DATA	    3 /* pointer to data is NULL */
#define IOFW_ERROR_MALLOC	    4 /* malloc fail */
#define IOFW_ERROR_ARGV		    5 /* wrong argv */

#define SERVER_RATIO 0.125
#define CLIENT_BUF_SIZE 1024*1024*1024
/**
 * @brief: init
 *
 * @param x_proc_num: client proc number of x axis
 * @param y_proc_num: client proc number of y axis
 * @param server_ratio: server ratio, which means server_amount/client_amount = 
 *	server_ratio
 *
 * @return: error code
 */
int iofw_init(int x_proc_num, int y_proc_num, int server_ratio);

/**
 * @brief iofw_Finalize : stop the iofw services, the function 
 * should be called before the mpi_Finalize
 *
 * @return 
 */
int iofw_finalize();
/**
 * @brief: iofw_create
 *
 * @param path: the file anme of the new netCDF dataset
 * @param cmode: the creation mode flag
 * @param ncidp: pointer to location where returned netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int iofw_create(
	const char *path, int cmode, int *ncidp);
/**
 * @brief: iofw_def_dim
 *
 * @param ncid: NetCDF group ID
 * @param name: Dimension name
 * @param len: Length of dimension
 * @param idp: pointer to location for returned dimension ID
 *
 * @return: 0 if success
 */
int iofw_def_dim(
	int ncid, const char *name, size_t len, int *idp);
/**
 * @brief: iofw_def_var
 *
 * @param io_proc_id: id of proc id 
 * @param ncid: netCDF ID
 * @param name: variable name
 * @param xtype: one of the set of predefined netCDF external data types
 * @param ndims: number of dimensions for the variable
 * @param dimids[]: vector of ndims dimension IDs corresponding to the 
 *	variable dimensions
 * @param start: a vector of size_t intergers specifying the index in the variable
 *	where the first of the data values will be written
 * @param count: a vector of size_t intergers specifying the edge lengths along 
 *	each dimension of the block of data values to be written
 * @param varidp: pointer to location for the returned variable ID
 *
 * @return: 0 if success
 */
int iofw_def_var(
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, 
	int *varidp);
/**
 * @brief:iofw_ enddef
 *
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_enddef(
	int ncid);
/**
 * @brief: iofw_put_vara_float
 *
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
int iofw_put_vara_float(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp);
/**
 * @brief: iofw_put_vara_double
 *
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
int iofw_put_vara_double(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const double *fp);
/**
 * @brief: iofw_close
 *
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_close(
	int ncid);

#endif

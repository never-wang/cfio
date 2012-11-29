/*
 * =====================================================================================
 *
 *       Filename:  cfio.h
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
#include "map.h"
#include "cfio_types.h"

#define CLIENT_BUF_SIZE 1024*1024*1024

#define CFIO_PROC_CLIENT CFIO_MAP_TYPE_CLIENT
#define CFIO_PROC_SERVER CFIO_MAP_TYPE_SERVER
#define CFIO_PROC_BLANK	 CFIO_MAP_TYPE_BLANK

#define CFIO_START(rank) \
    if(cfio_map_proc_type(rank) == CFIO_MAP_TYPE_CLIENT) {

#define CFIO_END()	\
    }			
	
/**
 * @brief: init, the x and y is	------>x(dim 0)
 *			       	|[0 1 2]
 *			       	|[3 4 5]
 *			 (dim 1)|y[6 7 8]
 *
 * @param x_proc_num: client proc number of x axis
 * @param y_proc_num: client proc number of y axis
 * @param ratio: client : server = ratio
 *
 * @return: error code
 */
int cfio_init(int x_proc_num, int y_proc_num, int ratio);

/**
 * @brief cfio_Finalize : stop the cfio services, the function 
 * should be called before the mpi_Finalize
 *
 * @return 
 */
int cfio_finalize();
/**
 * @brief: get the process type, client, server or blank
 *
 * @param rank: rank of the proccess
 *
 * @return: the proccess type
 */
int cfio_proc_type(int rank);
/**
 * @brief: cfio_create
 *
 * @param path: the file anme of the new netCDF dataset
 * @param cmode: the creation mode flag
 * @param ncidp: pointer to location where returned netCDF ID is to be stored
 *
 * @return: 0 if success
 */
int cfio_create(
	const char *path, int cmode, int *ncidp);
/**
 * @brief: cfio_def_dim
 *
 * @param ncid: NetCDF group ID
 * @param name: Dimension name
 * @param len: Length of dimension
 * @param idp: pointer to location for returned dimension ID
 *
 * @return: 0 if success
 */
int cfio_def_dim(
	int ncid, const char *name, size_t len, int *idp);
/**
 * @brief: cfio_def_var
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
int cfio_def_var(
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, 
	int *varidp);
/**
 * @brief: cfio_put_att
 *
 * @param ncid: NetCDF ID
 * @param varid: Variable ID of the variable to which the attribute will be 
 *	assigned or NC_GLOBAL for a global attribute
 * @param name: Attribute name
 * @param xtype: one fo the set of prdefined netCDF external data types
 * @param len: number of values provided for the attribute
 * @param op: Pointer to values
 *
 * @return: error code
 */
int cfio_put_att(
	int ncid, int varid, const char *name, 
	nc_type xtype, size_t len, const void *op);
/**
 * @brief:cfio_ enddef
 *
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int cfio_enddef(
	int ncid);
/**
 * @brief: cfio_put_vara_float
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
int cfio_put_vara_float(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp);
/**
 * @brief: cfio_put_vara_double
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
int cfio_put_vara_double(
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const double *fp);
/**
 * @brief: cfio_close
 *
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int cfio_close(
	int ncid);

#endif

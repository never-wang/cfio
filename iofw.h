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

#include "pomme_buffer.h"
#include "pomme_queue.h"
#include "unmap.h"
#include "pack.h"
#include "mpi.h"

/**********************************************************
 *  des: init the io forwrding invironment
 *  @param iofw_servers: the number of iofw_servers;
 *********************************************************/
int iofw_init(int iofw_servers);

/**
 * @brief iofw_Finalize : stop the iofw services, the function 
 * should be called before the mpi_Finalize
 *
 * @return 
 */
int iofw_finalize();
/**
 * @brief: iofw_nc_create
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
 * @brief: iofw_nc_def_dim
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
 * @brief: iofw_nc_def_var
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
 * @brief:iofw_ nc_enddef
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
 * @brief: iofw_nc_put_var_float
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
	const size_t *index, const float *fp);
/**
 * @brief: iofw_nc_put_vara_float
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
 * @brief: iofw_nc_close
 *
 * @param io_proc_id: 
 * @param ncid: netCDF ID
 *
 * @return: 0 if success
 */
int iofw_nc_close(
	int io_proc_id,
	int ncid);

#endif

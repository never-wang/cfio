/****************************************************************************
 *       Filename:  io.c
 *
 *    Description:  IO interface
 *
 *        Version:  1.0
 *        Created:  12/13/2011 04:25:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>
#include <stdio.h>

#include "io.h"
#include "pack.h"
#include "map.h"
#include "msg.h"

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
	int io_proc_id,
	const char *path, int cmode, int *ncidp)
{
    assert(path != NULL);
    assert(ncidp != NULL);

    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_create(buf, path, cmode);
    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    iofw_recv_int1(dst_proc_id, ncidp);

    free_buf(buf);
    return 0;
}

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
	int io_proc_id,
	int ncid, const char *name, size_t len, int *idp)
{
    assert(name != NULL);
    assert(idp != NULL);

    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_def_dim(buf, ncid, name, len);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    iofw_recv_int1(dst_proc_id, idp);

    free_buf(buf);
    return 0;
}
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
	int ndims, const int dimids[], int *varidp)
{
    assert(name != NULL);
    assert(dimids != NULL);
    assert(varidp != NULL);

    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_def_var(buf, ncid, name, xtype, ndims, dimids);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    iofw_recv_int1(dst_proc_id, varidp);

    free_buf(buf);
    return 0;
}

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
	int ncid)
{
    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_enddef(buf, ncid);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    free_buf(buf);
    return 0;
}

int iofw_nc_put_var1_float(
	int io_proc_id,
	int ncid, int varid, const size_t index[],
	const float *fp)
{
    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_put_var1_float(buf, ncid, varid, index, fp);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    free_buf(buf);
    return 0;
}

int iofw_nc_put_vara_float(
	int io_proc_id,
	int ncid, int varid, const size_t start[],
	const size_t count[], const float *fp)
{
    iofw_buf_t *head_buf, *data_buf;
    int dst_proc_id,i;
    int data_len,  dim;

    head_buf = init_buf(BUF_SIZE);
    iofw_pack_msg_put_vara_float(head_buf, ncid, varid, start, count);
    
    data_buf = init_buf(BUF_SIZE);

    dim = sizeof(start) / sizeof(size_t);
    data_len = 1;
    for(i = 0; i < dim; i ++)
    {
	data_len *= count[i]; 
    }

	fprintf(stderr,"sizeof(start) : %d,data_len %d\n",sizeof(start),data_len);
    pack32_array((uint32_t*)fp, data_len, data_buf);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, head_buf);
    iofw_send_msg(dst_proc_id, data_buf);

    free_buf(head_buf);
    free_buf(data_buf);
    return 0;
}

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
	int ncid)
{
    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_close(buf, ncid);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    free_buf(buf);
    return 0;
}

/**
 * @brief iofw_io_stop : tell the server the client is over
 *
 *
 * @return : 0 for success , < for failure
 */
int iofw_io_stop(int io_proc_id)
{

    iofw_buf_t *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_io_stop(buf);

    iofw_map_forwarding_proc(io_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    free_buf(buf);
    return 0;

}


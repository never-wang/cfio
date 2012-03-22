/****************************************************************************
 *       Filename:  msg.c
 *
 *    Description:  implement for msg between IO process adn IO forwarding process
 *
 *        Version:  1.0
 *        Created:  12/13/2011 03:11:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include "msg.h"
#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "debug.h"

int iofw_send_msg(
	int dst_proc_id, int src_proc_id, iofw_buf_t *buf, MPI_Comm comm)
{
    int tag = src_proc_id;

    debug(DEBUG_MSG, "send: src=%d; dst=%d; comm = %d; size = %d", 
	    src_proc_id, dst_proc_id, comm, buf->processed);
    MPI_Send(buf->head, buf->processed, MPI_BYTE, dst_proc_id, tag, comm);
    
    return 0;
}

int iofw_isend_msg(
	int dst_proc_id, int src_proc_id, iofw_buf_t *buf, MPI_Comm comm)
{
    int tag = src_proc_id;
    MPI_Request request;
    
    debug(DEBUG_MSG, "isend: src=%d; dst=%d; comm = %d; size = %d", 
	    src_proc_id, dst_proc_id, comm, buf->processed);

    MPI_Isend(buf->head, buf->processed, MPI_BYTE, 
	    dst_proc_id, tag, comm, &request);
    
    return 0;
}

int iofw_send_int1(int dst_proc_id, int src_proc_id, int data, MPI_Comm comm)
{
    debug(DEBUG_MSG, "send: src=%d; dst=%d; comm = %d", 
	    src_proc_id, dst_proc_id, comm);
    
    MPI_Send(&data, 1 , MPI_INT, dst_proc_id, src_proc_id, comm);
    return 0;	
}

int iofw_recv_int1(int src_proc_id, int *data, MPI_Comm comm)
{
    MPI_Status status;

    MPI_Recv(data, 1, MPI_INT, src_proc_id, src_proc_id, comm, &status);

    return 0;
}

int iofw_pack_msg_create(
	iofw_buf_t *buf,
	const char *path, int cmode)
{
    uint32_t code = FUNC_NC_CREATE;

    packdata(&code, sizeof(uint32_t), buf);
    packstr(path, buf);
    packdata(&cmode, sizeof(int), buf);

    return 0;
}

/**
 *pack msg function
 **/
int iofw_pack_msg_def_dim(
	iofw_buf_t *buf,
	int ncid, const char *name, size_t len)
{
    uint32_t code = FUNC_NC_DEF_DIM;

    packdata(&code, sizeof(uint32_t), buf);
    packdata(&ncid, sizeof(int), buf);
    packstr(name, buf);
    
    packdata(&len, sizeof(size_t), buf);

    return 0;
}

int iofw_pack_msg_def_var(
	iofw_buf_t *buf,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids)
{
    uint32_t code = FUNC_NC_DEF_VAR;
    
    packdata(&code , sizeof(uint32_t), buf);
    packdata(&ncid, sizeof(int), buf);

    packstr(name, buf);
    packdata(&xtype, sizeof(nc_type), buf);
	
    packdata_array(dimids, ndims, sizeof(int), buf);


    return 0;
}

int iofw_pack_msg_enddef(
	iofw_buf_t *buf,
	int ncid)
{
    uint32_t code = FUNC_NC_ENDDEF;

    packdata(&code, sizeof(uint32_t), buf);
    packdata(&ncid, sizeof(int), buf);

    return 0;
}

int iofw_pack_msg_put_var1_float(
	iofw_buf_t *buf,
	int ncid, int varid, int dim,
	const size_t *index, const float *fp)
{
    uint32_t code = FUNC_NC_PUT_VAR1_FLOAT;
    
    packdata(&code, sizeof(uint32_t), buf);
    packdata(&ncid, sizeof(int), buf);
    packdata(&varid, sizeof(int), buf);
    packdata_array(index, dim, sizeof(size_t), buf);
    packdata((void*)fp, sizeof(float), buf);

    return 0;
}

int iofw_pack_msg_put_vara_float(
	iofw_buf_t *buf,
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp)
{
    int i;
    size_t data_len;
    uint32_t code = FUNC_NC_PUT_VARA_FLOAT;
    float *ffp;
    
    debug(DEBUG_MSG, "pack_msg_put_vara_float");
    for(i = 0; i < dim; i ++)
    {
	debug(DEBUG_MSG, "start[%d] = %d", i, start[i]);
    }
    for(i = 0; i < dim; i ++)
    {
	debug(DEBUG_MSG, "count[%d] = %d", i, count[i]);
    }
    
    data_len = 1;
    for(i = 0; i < dim; i ++)
    {
	data_len *= count[i]; 
    }

    packdata(&code, sizeof(uint32_t), buf);

    packdata(&ncid, sizeof(int), buf);
    packdata(&varid, sizeof(int), buf);
    
    packdata_array(start, dim, sizeof(size_t), buf);
    packdata_array(count, dim, sizeof(size_t), buf);

    packdata_array(fp, data_len, sizeof(float), buf);

    buf->processed -= data_len * sizeof(float);
    buf->processed -= sizeof(int);
    unpackdata_array(&ffp, &data_len, sizeof(float), buf);

    return 0;
}

int iofw_pack_msg_close(
	iofw_buf_t *buf,
	int ncid)
{
    uint32_t code = FUNC_NC_CLOSE;
    
    packdata(&code, sizeof(uint32_t), buf);
    packdata(&ncid, sizeof(int), buf);

    return 0;
}

int iofw_pack_msg_io_stop(
	iofw_buf_t *buf)
{
    uint32_t code = CLIENT_END_IO;
    
    packdata(&code, sizeof(uint32_t), buf);
    return 0;
}
/**
 *unpack msg function
 **/
uint32_t iofw_unpack_msg_func_code(
	iofw_buf_t *buf)
{
    uint32_t func_code;

    unpackdata(&func_code, sizeof(uint32_t), buf);

    return func_code;
}

int iofw_unpack_msg_create(
	iofw_buf_t *buf,
	char **path, int *cmode)
{
    uint32_t len;

    unpackstr_ptr(path, &len, buf);
    unpackdata(cmode, sizeof(int), buf);

    return 0;
}

int iofw_unpack_msg_def_dim(
	iofw_buf_t *buf,
	int *ncid, char **name, size_t *len)
{
    uint32_t len_t;

    unpackdata(ncid, sizeof(int), buf);
    unpackstr_ptr(name, &len_t, buf);
    unpackdata(len, sizeof(size_t), buf);

    return 0;
}

int iofw_unpack_msg_def_var(
	iofw_buf_t *buf,
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids)
{
    uint32_t len;

    unpackdata(ncid, sizeof(int), buf);
    unpackstr_malloc(name, &len, buf);
    unpackdata(xtype, sizeof(nc_type), buf);
    
    unpackdata_array((void **)dimids, ndims, sizeof(int), buf);

    return 0;
}

int iofw_unpack_msg_enddef(
	iofw_buf_t *buf,
	int *ncid)
{
    unpackdata(ncid, sizeof(int), buf);

    return 0;
}

int iofw_unpack_msg_put_var1_float(
	iofw_buf_t *buf,
	int *ncid, int *varid, int *indexdim, size_t **index)
{
    unpackdata(ncid, sizeof(int), buf);
    unpackdata(varid, sizeof(int), buf);
    unpackdata_array((void **)index, indexdim, sizeof(int), buf);

    return 0;
}

int iofw_unpack_msg_put_vara_float(
	iofw_buf_t *buf,
	int *ncid, int *varid, int *dim, size_t **start, size_t **count,
	int *data_len, float **fp)
{
    int i;

    unpackdata(ncid, sizeof(int), buf);
    unpackdata(varid, sizeof(int), buf);
    
    unpackdata_array((void**)start, dim, sizeof(size_t), buf);
    unpackdata_array((void**)count, dim, sizeof(size_t), buf);

    unpackdata_array((void**)fp, data_len, sizeof(float), buf);
    
    return 0;
}

int iofw_unpack_msg_close(
	    iofw_buf_t *buf,
	    int *ncid)
{
    unpackdata(ncid, sizeof(int), buf);

    debug_mark(DEBUG_MSG);
    return 0;

}

int iofw_unpack_msg_extra_data_size(
	iofw_buf_t  *buf,
	size_t *data_size)
{
    unpackdata(data_size, sizeof(size_t), buf);
    return 0;
}


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

int iofw_send_msg(int dst_proc_id, iofw_buf_t *buf)
{
    int tag = 1;

    MPI_Send(buf->head, buf->processed, MPI_BYTE, dst_proc_id, tag, MPI_COMM_WORLD);
 
    return 0;
}

int iofw_recv_int1(int src_proc_id, int *data)
{
    int tag = 1;
    MPI_Status status;

    MPI_Recv(data, 1, MPI_INT, src_proc_id, tag, MPI_COMM_WORLD, &status);

    return 0;
}

int iofw_pack_msg_create(
	iofw_buf_t *buf,
	const char *path, int cmode)
{
    pack32(FUNC_NC_CREATE, buf);
    packstr(path, buf);
    pack32(cmode, buf);

    return 0;
}

/**
 *pack msg function
 **/
int iofw_pack_msg_def_dim(
	iofw_buf_t *buf,
	int ncid, const char *name, size_t len)
{
    pack32(FUNC_NC_DEF_DIM, buf);
    pack32(ncid, buf);
    packstr(name, buf);
    
    //TODO when pack size_t , how
    pack32(len, buf);

    return 0;
}

int iofw_pack_msg_def_var(
	iofw_buf_t *buf,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int dimids[])
{
    pack32(FUNC_NC_DEF_VAR, buf);
    pack32(ncid, buf);
    packstr(name, buf);
    //TODO xtype is 32-byte?
    pack32(xtype, buf);
    pack32_array((uint32_t*)dimids, ndims, buf);

    return 0;
}

int iofw_pack_msg_enddef(
	iofw_buf_t *buf,
	int ncid)
{
    pack32(FUNC_NC_ENDDEF, buf);
    pack32(ncid, buf);

    return 0;
}

int iofw_pack_msg_put_var1_float(
	iofw_buf_t *buf,
	int ncid, int varid, const size_t index[],
	const float *fp)
{
    pack32(FUNC_NC_PUT_VAR1_FLOAT, buf);
    pack32(ncid, buf);
    pack32(varid, buf);
    //TODO pack size_t
    pack32_array((uint32_t*)index, sizeof(index)/sizeof(size_t), buf);

    return 0;
}

int iofw_pack_msg_close(
	iofw_buf_t *buf,
	int ncid)
{
    pack32(FUNC_NC_CLOSE, buf);
    pack32(ncid, buf);

    return 0;
}
/**
 *unpack msg function
 **/
int iofw_unpack_msg_func_code(
	iofw_buf_t *buf)
{
    int func_code;

    unpack32(&func_code, buf);

    return func_code;
}

int iofw_unpack_msg_create(
	iofw_buf_t *buf,
	char **path, int *cmode)
{
    int len;

    unpackstr_malloc(path, &len, buf);
    unpack32(cmode, buf);

    return 0;
}

int iofw_unpack_msg_def_dim(
	iofw_buf_t *buf,
	int *ncid, char **name, size_t *len)
{
    int len;

    unpack32(ncid, buf);
    unpackstr_malloc(name, &len, buf);
    unpack32(len, buf);

    return 0;
}

int iofw_unpack_msg_def_var(
	iofw_buf_t *buf,
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids)
{
    int len;

    unpack32(ncid, buf);
    unpackstr_malloc(name, &len, buf);
    unpack32(xtype, buf);
    unpack32(ndims, buf);
    
    unpack32_array(dimids, ndims, buf);

    return 0;
}

int iofw_unpack_msg_enddef(
	iofw_buf_t *buf,
	int *ncid)
{
    unpack32(ncid, buf);

    return 0;
}

int iofw_unpack_msg_put_var1_float(
	iofw_buf_t *buf,
	int *ncid, int *varid, int *indexdim, size_t *index)
{
    unpack32(ncid, buf);
    unpack32(varid, buf);
    unpack32_array(index, indexdim, buf);

    return 0;
}

int iofw_unpack_msg_close(
	    iofw_buf_t *buf,
	    int *ncid)
{
    unpack32(ncid, buf);

    return 0;

}


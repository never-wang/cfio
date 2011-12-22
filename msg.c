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
/*
 * why set tag == 1 ?
 */
//    int tag = 1;
    MPI_Status status;

    MPI_Recv(data, 1, MPI_INT, src_proc_id, src_proc_id, MPI_COMM_WORLD, &status);

    return 0;
}
int iofw_send_int1(int des_por_id, int my_rank,int data)
{
    MPI_Send(&data, 1 , MPI_INT, des_por_id, my_rank, MPI_COMM_WORLD);
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
    pack32((uint32_t*)fp, buf);

    return 0;
}

int iofw_pack_msg_put_vara_float(
	iofw_buf_t *buf,
	int ncid, int varid, const size_t start[],
	const size_t count[])
{
    int dim;
    size_t data_size;

    pack32(FUNC_NC_PUT_VARA_FLOAT, buf);
    pack32(ncid, buf);
    pack32(varid, buf);
    
    if((dim = sizeof(start) / sizeof(size_t)) != 
	    sizeof(count) / sizeof(size_t))
    {
	//TODO DEBUG
	return IOFW_UNEQUAL_DIM;
    }
    pack32_array((uint32_t*)start, dim, buf);
    pack32_array((uint32_t*)count, dim, buf);

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

int iofw_pack_msg_io_stop(
	iofw_buf_t *buf)
{
    pack32(CLIENT_END_IO, buf);
    return 0;
}
/**
 *unpack msg function
 **/
int iofw_unpack_msg_func_code(
	iofw_buf_t *buf)
{
    int func_code;

    unpack32((uint32_t*)&func_code, buf);

    return func_code;
}

int iofw_unpack_msg_create(
	iofw_buf_t *buf,
	char **path, int *cmode)
{
    uint32_t len;

    unpackstr_malloc(path, &len, buf);
    unpack32((uint32_t*)cmode, buf);

    return 0;
}

int iofw_unpack_msg_def_dim(
	iofw_buf_t *buf,
	int *ncid, char **name, size_t *len)
{
    uint32_t len_t;

    unpack32((uint32_t*)ncid, buf);
    unpackstr_malloc(name, &len_t, buf);
    unpack32((uint32_t*)len, buf);

    return 0;
}

int iofw_unpack_msg_def_var(
	iofw_buf_t *buf,
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids)
{
    uint32_t len;

    unpack32((uint32_t*)ncid, buf);
    unpackstr_malloc(name, &len, buf);
    unpack32((uint32_t*)xtype, buf);
    unpack32((uint32_t*)ndims, buf);
    
    unpack32_array((uint32_t**)dimids, (uint32_t*)ndims, buf);

    return 0;
}

int iofw_unpack_msg_enddef(
	iofw_buf_t *buf,
	int *ncid)
{
    unpack32((uint32_t*)ncid, buf);

    return 0;
}

int iofw_unpack_msg_put_var1_float(
	iofw_buf_t *buf,
	int *ncid, int *varid, int *indexdim, size_t *index)
{
    unpack32((uint32_t*)ncid, buf);
    unpack32((uint32_t*)varid, buf);
    unpack32_array((uint32_t**)index, (uint32_t*)indexdim, buf);

    return 0;
}

int iofw_unpack_msg_close(
	    iofw_buf_t *buf,
	    int *ncid)
{
    unpack32((uint32_t*)ncid, buf);

    return 0;

}


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
#include "times.h"

static iofw_msg_t *msg_head;
iofw_buf_t *buffer;

static inline iofw_msg_t *create_msg(int client_proc_id)
{
    iofw_msg_t *msg;
    msg = malloc(sizeof(iofw_msg_t));
    if(NULL == msg)
    {
	return NULL;
    }
    msg->addr = buffer->free_addr;
    msg->size = 0;
    msg->src = client_proc_id;

    return msg;
}

int iofw_msg_init()
{
    msg_head = malloc(sizeof(iofw_msg_t));
    INIT_QLIST_HEAD(msg_head);

    buffer = iofw_buf_open(CLIENT_BUF_SIZE, iofw_msg_buf_free, NULL);

    if(NULL == buffer)
    {
	return IOFW_MSG_ERROR_BUFFER;
    }

    return IOFW_MSG_ERROR_NONE;
}

int ifow_msg_final()
{
    iofw_msg_t *msg, *next;
    qlist_for_each_entry_safe(msg, next, msg_head->link, link)
    {
	free(msg);
    }
    free(msg_head);

    return IOFW_MSG_ERROR_NONE;
}

int ifow_msg_buf_free()
{
    iofw_msg_t *msg; *next;
    qlist_for_each_entry_safe(msg, next, msg_head->link, link)
    {
	if(MPI_test(&msg->req, NULL))
	{
	    assert(msg->addr == buffer->used_addr);
	    inc_buf_addr(buffrer, &(buffer->used_addr), msg->size);
	    qlist_del(&(msg->link));
	    free(msg);
	}else
	{
	    break;
	}
    }
}

int iofw_msg_isend(
	iofw_msg_t *msg, MPI_Comm comm)
{
    int tag = msg->src;
    MPI_Request request;
    MPI_Status status;
 
	//times_start();

    debug(DEBUG_MSG, "isend: src=%d; dst=%d; comm = %d; size = %d", 
	    msg->src, msg->dst, comm, msg->size);

    MPI_Isend(msg->addr, msg->size, MPI_BYTE, 
	    msg->dst, tag, comm, &(msg->req));

    qlist_add_tail(&(msg->link), &(msg_head->link));

    //debug(DEBUG_TIME, "%f ms", times_end());

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_recv(MPI_comm comm)
{
    MPI_Status status;

    while(free_buf_size(buffer) < MSG_MAX_SIZE) {};

    MPI_Recv(buffer->free_addr, MSG_MAX_SIZE, MPI_BYTE, MPI_SOURCE_ANY, 
	    MPI_TAG_ANY, comm, &status);

    return IOFW_MSG_ERROR_NON;
}

int iofw_msg_pack_nc_create(
	iofw_msg_t **_msg, int client_proc_id, 
	const char *path, int cmode)
{
    uint32_t code = FUNC_NC_CREATE;
    iofw_msg_t *msg;

    msg = create_msg(client_proc_id);

    iofw_buf_pack_data(&code, sizeof(uint32_t) , buffer);
    iofw_buf_pack_str(path, buffer);
    iofw_buf_pack_data(&cmode, sizeof(int), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;

    return IOFW_MSG_ERROR_NONE;
}

/**
 *pack msg function
 **/
int iofw_msg_pack_nc_def_dim(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, size_t len)
{
    uint32_t code = FUNC_NC_DEF_DIM;
    iofw_msg_t *msg;

    msg = create_msg(client_proc_id);

    iofw_buf_pack_data(&code, sizeof(uint32_t), &(msg->size), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), &(msg->size), buffer);
    iofw_buf_pack_str(name, buffer);
    iofw_buf_pack_data(&len, sizeof(size_t), &(msg->size), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;
    
    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_def_var(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids)
{
    uint32_t code = FUNC_NC_DEF_VAR;
    iofw_msg_t *msg;

    msg = create_msg(client_proc_id);
    
    iofw_buf_pack_data(&code , sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);
    iofw_buf_pack_str(name, buffer);
    iofw_buf_pack_data(&xtype, sizeof(nc_type), buffer);
    iofw_pack_data_array(dimids, ndims, sizeof(int), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_enddef(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid)
{
    uint32_t code = FUNC_NC_ENDDEF;
    iofw_msg_t *msg;

    msg = create_msg(client_proc_id);
    
    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;
    
    return 0;
    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_put_var1_float(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int dim,
	const size_t *index, const float *fp)
{
    uint32_t code = FUNC_NC_PUT_VAR1_FLOAT;
    iofw_msg_t *msg;
    
    msg = create_msg(client_proc_id);
    
    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);
    iofw_buf_pack_data(&varid, sizeof(int), buffer);
    iofw_buf_pack_data_array(index, dim, sizeof(size_t), buffer);
    iofw_buf_pack_data((void*)fp, sizeof(float), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;
    
    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_put_vara_float(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int dim,
	const size_t *start, const size_t *count, const float *fp)
{
    int i;
    size_t data_len;
    uint32_t code = FUNC_NC_PUT_VARA_FLOAT;
    iofw_msg_t *msg;
    
    //times_start();

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
    
    msg = create_msg(client_proc_id);

    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);
    iofw_buf_pack_data(&varid, sizeof(int), buffer);
    iofw_buf_pack_data_array(start, dim, sizeof(size_t), buffer);
    iofw_buf_pack_data_array(count, dim, sizeof(size_t), buffer);
    iofw_buf_pack_data_array(fp, data_len, sizeof(float), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;
    
    //debug(DEBUG_TIME, "%f ms", times_end());

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_close(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid)
{
    uint32_t code = FUNC_NC_CLOSE;
    iofw_msg_t *msg;
    
    //times_start();
    msg = create_msg(client_proc_id);

    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);

    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;
    //debug(DEBUG_TIME, "%f", times_end());

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_io_done(
	iofw_msg_t **_msg, int client_proc_id)
{
    uint32_t code = CLIENT_END_IO;
    iofw_msg_t *msg;
    
    msg = create_msg(client_proc_id);
    
    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    
    msg->size = buffer->free_addr - msg->addr;
    iofw_map_forward_proc(msg->src, &(msg->dst));
    *_msg = msg;
    
    return IOFW_MSG_ERROR_NONE;
}
/**
 *unpack msg function
 **/
int iofw_msg_unpack_func_code(uint32_t *func_code)
{
    iofw_buf_unpack_data(func_code, sizeof(uint32_t), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_create(
	char **path, int *cmode)
{
    iofw_buf_unpack_str(path, buffer);
    iofw_buf_unpack_data(cmode, sizeof(int), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_def_dim(
	int *ncid, char **name, size_t *len)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_str(name, buffer);
    iofw_buf_unpack_data(len, sizeof(size_t), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_def_var(
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_str(name, buffer);
    iofw_buf_unpack_data(xtype, sizeof(nc_type), buffer);
    
    iofw_buf_unpack_data_array((void **)dimids, ndims, 
	    sizeof(int), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_enddef(
	int *ncid)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_put_var1_float(
	int *ncid, int *varid, int *indexdim, size_t **index)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_data(varid, sizeof(int), buffer);
    iofw_buf_unpack_data_array((void **)index, indexdim, 
	    sizeof(int), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_put_vara_float(
	int *ncid, int *varid, int *dim, 
	size_t **start, size_t **count,
	int *data_len, float **fp)
{
    int i;

    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_data(varid, sizeof(int), buffer);
    
    iofw_buf_unpack_data_array((void**)start, dim, sizeof(size_t), buffer);
    iofw_buf_unpack_data_array((void**)count, dim, sizeof(size_t), buffer);

    iofw_buf_unpack_data_array_ptr((void**)fp, data_len, 
	    sizeof(float), buffer);
    
    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_close(int *ncid)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);

    return IOFW_MSG_ERROR_NONE;
}


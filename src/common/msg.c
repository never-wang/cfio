/****************************************************************************
 *       Filename:  msg.c
 *
 *    Description:  implement for msg between IO process adn IO forwardinging process
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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "msg.h"
#include "debug.h"
#include "times.h"
#include "map.h"
#include "pthread.h"
#include "id.h"
#include "iofw_types.h"

static iofw_msg_t *msg_head;
iofw_buf_t *buffer;
pthread_mutex_t *mutex;

static inline iofw_msg_t *create_msg()
{
    iofw_msg_t *msg;
    msg = malloc(sizeof(iofw_msg_t));
    if(NULL == msg)
    {
	return NULL;
    }
    memset(msg, 0, sizeof(iofw_msg_t));
    msg->addr = buffer->free_addr;

    return msg;
}

int iofw_msg_init()
{
    msg_head = malloc(sizeof(iofw_msg_t));
    if(NULL == msg_head)
    {
	return -IOFW_MSG_ERROR_MALLOC;
    }
    INIT_QLIST_HEAD(&(msg_head->link));

    buffer = iofw_buf_open(CLIENT_BUF_SIZE, NULL);

    if(NULL == buffer)
    {
	return -IOFW_MSG_ERROR_BUFFER;
    }

    mutex = malloc(sizeof(pthread_mutex_t));
    if(NULL == mutex)
    {
	return -IOFW_MSG_ERROR_MALLOC;
    }
    pthread_mutex_init(mutex, NULL);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_final()
{
    iofw_msg_t *msg, *next;

    if(NULL != msg_head)
    {
        qlist_for_each_entry_safe(msg, next, &(msg_head->link), link)
        {
            free(msg);
        }
        free(msg_head);
        msg_head = NULL;
    }

    if(NULL != mutex)
    {
        pthread_mutex_destroy(mutex);
        //free(mutex);
        mutex = NULL;
    }

    if(NULL != buffer)
    {
        buffer = NULL;
    }

    return IOFW_MSG_ERROR_NONE;
}

/**
 * @brief: free unsed space in client buffer
 *
 * @return: 
 */
static void iofw_msg_client_buf_free()
{
    iofw_msg_t *msg, *next;
    int done;
    MPI_Status status;

    qlist_for_each_entry_safe(msg, next, &(msg_head->link), link)
    {
	MPI_Test(&msg->req, &done, &status);
    
	if(done)
	{
	    //assert(msg->addr == buffer->used_addr);
	    assert(check_used_addr(msg->addr, buffer));
	    buffer->used_addr = msg->addr;
	    free_buf(buffer, msg->size);
	    qlist_del(&(msg->link));
	    free(msg);
	}else
	{
	    break;
	}
    }
    return;
}

static void iofw_msg_server_buf_free()
{
    

    return;
}

int iofw_msg_isend(
	iofw_msg_t *msg)
{
    int tag = msg->src;
    MPI_Request request;
    MPI_Status status;
 
	//times_start();

    debug(DEBUG_MSG, "isend: src=%d; dst=%d; comm = %d; size = %lu", 
	    msg->src, msg->dst, msg->comm, msg->size);

    MPI_Isend(msg->addr, msg->size, MPI_BYTE, 
	    msg->dst, tag, msg->comm, &(msg->req));

    qlist_add_tail(&(msg->link), &(msg_head->link));

    //debug(DEBUG_TIME, "%f ms", times_end());

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_recv(int rank, MPI_Comm comm)
{
    MPI_Status status;
    int size;
    iofw_msg_t *msg;

    times_start();
    ensure_free_space(buffer, MSG_MAX_SIZE, iofw_msg_server_buf_free);
    debug(DEBUG_TIME, "%f", times_end());

    MPI_Recv(buffer->free_addr, MSG_MAX_SIZE, MPI_BYTE, MPI_ANY_SOURCE, 
	    MPI_ANY_TAG, comm, &status);
    MPI_Get_count(&status, MPI_BYTE, &size);
    debug(DEBUG_MSG, "recv: size = %d", size);

    //debug(DEBUG_MSG, "code = %u", *((uint32_t *)buffer->free_addr));

    if(status.MPI_SOURCE != status.MPI_TAG)
    {
	return -IOFW_MSG_ERROR_MPI;
    }

    msg = create_msg();
    msg->size = size;
    msg->src = status.MPI_SOURCE;
    msg->dst = rank;

    use_buf(buffer, size);
    
    debug_mark(DEBUG_MSG);
    /* need lock */
    pthread_mutex_lock(mutex);
    debug_mark(DEBUG_MSG);
    qlist_add_tail(&(msg->link), &(msg_head->link));
    debug_mark(DEBUG_MSG);
    pthread_mutex_unlock(mutex);
    
    debug_mark(DEBUG_MSG);
    
    if(msg->addr == buffer->start_addr)
    {
	debug_mark(DEBUG_MSG);
    }
    //debug(DEBUG_MSG, "uesd_size = %lu", used_buf_size(buffer));
    debug(DEBUG_MSG, "success return");
    return IOFW_MSG_ERROR_NONE;
}

iofw_msg_t *iofw_msg_get_first()
{
    iofw_msg_t *msg;
    qlist_head_t *link;

    pthread_mutex_lock(mutex);
    link = qlist_pop(&(msg_head->link));
    pthread_mutex_unlock(mutex);
    if(NULL == link)
    {
	msg = NULL;
    }else
    {
	msg = qlist_entry(link, iofw_msg_t, link);
    }

    if(msg != NULL)
    {
	debug(DEBUG_MSG, "get msg->size = %lu", msg->size);
    }
    return msg;
}

int iofw_msg_pack_nc_create(
	iofw_msg_t **_msg, int client_proc_id, 
	const char *path, int cmode, int ncid)
{
    uint32_t code = FUNC_NC_CREATE;
    iofw_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;

    msg->size = 0;
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    msg->size += iofw_buf_str_size(path);
    msg->size += iofw_buf_data_size(sizeof(int));
    msg->size += iofw_buf_data_size(sizeof(int));

    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    iofw_buf_pack_data(&code, sizeof(uint32_t) , buffer);
    iofw_buf_pack_str(path, buffer);
    iofw_buf_pack_data(&cmode, sizeof(int), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);

    debug_mark(DEBUG_MSG);
    iofw_map_forwarding(msg);
    debug_mark(DEBUG_MSG);
    *_msg = msg;
    
    debug(DEBUG_MSG, "path = %s; cmode = %d, ncid = %d", path, cmode, ncid);

    return IOFW_MSG_ERROR_NONE;
}

/**
 *pack msg function
 **/
int iofw_msg_pack_nc_def_dim(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, size_t len, int dimid)
{
    uint32_t code = FUNC_NC_DEF_DIM;
    iofw_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    msg->size += iofw_buf_data_size(sizeof(int));
    msg->size += iofw_buf_str_size(name);
    msg->size += iofw_buf_data_size(sizeof(size_t));
    msg->size += iofw_buf_data_size(sizeof(int));

    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);
    
    msg->addr = buffer->free_addr;

    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);
    iofw_buf_pack_str(name, buffer);
    iofw_buf_pack_data(&len, sizeof(size_t), buffer);
    iofw_buf_pack_data(&dimid, sizeof(int), buffer);

    iofw_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, len = %lu", ncid, name, len);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_def_var(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, int varid)
{
    uint32_t code = FUNC_NC_DEF_VAR;
    iofw_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    msg->size += iofw_buf_data_size(sizeof(int));
    msg->size += iofw_buf_str_size(name);
    msg->size += iofw_buf_data_size(sizeof(nc_type));
    msg->size += iofw_buf_data_array_size(ndims, sizeof(int));
    msg->size += iofw_buf_data_size(sizeof(int));

    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);
    
    debug_mark(DEBUG_MSG);
    
    msg->addr = buffer->free_addr;

    iofw_buf_pack_data(&code , sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);
    iofw_buf_pack_str(name, buffer);
    iofw_buf_pack_data(&xtype, sizeof(nc_type), buffer);
    iofw_buf_pack_data_array(dimids, ndims, sizeof(int), buffer);
    iofw_buf_pack_data(&varid, sizeof(int), buffer);

    iofw_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, ndims = %u", ncid, name, ndims);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_enddef(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid)
{
    uint32_t code = FUNC_NC_ENDDEF;
    iofw_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    msg->size += iofw_buf_data_size(sizeof(int));
    
    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);
    
    msg->addr = buffer->free_addr;

    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);

    iofw_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d", ncid);
    
    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_put_vara(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int ndims,
	const size_t *start, const size_t *count, 
	const int fp_type, const void *fp)
{
    int i;
    size_t data_len;
    uint32_t code = FUNC_NC_PUT_VARA;
    iofw_msg_t *msg;
    
    //times_start();

    debug(DEBUG_MSG, "pack_msg_put_vara_float");
    for(i = 0; i < ndims; i ++)
    {
	debug(DEBUG_MSG, "start[%d] = %lu", i, start[i]);
    }
    for(i = 0; i < ndims; i ++)
    {
	debug(DEBUG_MSG, "count[%d] = %lu", i, count[i]);
    }
    
    data_len = 1;
    for(i = 0; i < ndims; i ++)
    {
	data_len *= count[i]; 
    }
    
    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    msg->size += iofw_buf_data_size(sizeof(int));
    msg->size += iofw_buf_data_size(sizeof(int));
    msg->size += iofw_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += iofw_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += iofw_buf_data_size(sizeof(int));
    switch(fp_type)
    {
	case IOFW_BYTE :
	    msg->size += iofw_buf_data_array_size(data_len, 1);
	    break;
	case IOFW_CHAR :
	    msg->size += iofw_buf_data_array_size(data_len, sizeof(char));
	    break;
	case IOFW_SHORT :
	    msg->size += iofw_buf_data_array_size(data_len, sizeof(short));
	    break;
	case IOFW_INT :
	    msg->size += iofw_buf_data_array_size(data_len, sizeof(int));
	    break;
	case IOFW_FLOAT :
	    msg->size += iofw_buf_data_array_size(data_len, sizeof(float));
	    break;
	case IOFW_DOUBLE :
	    msg->size += iofw_buf_data_array_size(data_len, sizeof(double));
	    break;
    }
	    
    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);
    iofw_buf_pack_data(&varid, sizeof(int), buffer);
    iofw_buf_pack_data_array(start, ndims, sizeof(size_t), buffer);
    iofw_buf_pack_data_array(count, ndims, sizeof(size_t), buffer);
    iofw_buf_pack_data(&fp_type, sizeof(int), buffer);
    switch(fp_type)
    {
	case IOFW_BYTE :
	    iofw_buf_pack_data_array(fp, data_len, 1, buffer);
	    break;
	case IOFW_CHAR :
	    iofw_buf_pack_data_array(fp, data_len, sizeof(char), buffer);
	    break;
	case IOFW_SHORT :
	    iofw_buf_pack_data_array(fp, data_len, sizeof(short), buffer);
	    break;
	case IOFW_INT :
	    iofw_buf_pack_data_array(fp, data_len, sizeof(int), buffer);
	    break;
	case IOFW_FLOAT :
	    iofw_buf_pack_data_array(fp, data_len, sizeof(float), buffer);
	    break;
	case IOFW_DOUBLE :
	    iofw_buf_pack_data_array(fp, data_len, sizeof(double), buffer);
	    break;
    }

    iofw_map_forwarding(msg);
    *_msg = msg;
    
    //debug(DEBUG_TIME, "%f ms", times_end());
    debug(DEBUG_MSG, "ncid = %d, varid = %d, ndims = %d, data_len = %lu", 
	    ncid, varid, ndims, data_len);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_nc_close(
	iofw_msg_t **_msg, int client_proc_id,
	int ncid)
{
    uint32_t code = FUNC_NC_CLOSE;
    iofw_msg_t *msg;
    
    //times_start();
    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    msg->size += iofw_buf_data_size(sizeof(int));
    
    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    iofw_buf_pack_data(&ncid, sizeof(int), buffer);

    iofw_map_forwarding(msg);
    *_msg = msg;
    //debug(DEBUG_TIME, "%f", times_end());

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_pack_io_done(
	iofw_msg_t **_msg, int client_proc_id)
{
    uint32_t code = CLIENT_END_IO;
    iofw_msg_t *msg;
    
    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += iofw_buf_data_size(sizeof(uint32_t));
    
    ensure_free_space(buffer, msg->size, iofw_msg_client_buf_free);
    
    iofw_buf_pack_data(&code, sizeof(uint32_t), buffer);
    
    iofw_map_forwarding(msg);
    *_msg = msg;
    
    return IOFW_MSG_ERROR_NONE;
}
/**
 *unpack msg function
 **/
int iofw_msg_unpack_func_code(iofw_msg_t *msg, uint32_t *func_code)
{
    assert(check_used_addr(msg->addr, buffer));
    
    if(msg->addr == buffer->start_addr)
    {
	debug_mark(DEBUG_MSG);
    }

    buffer->used_addr = msg->addr;
    iofw_buf_unpack_data(func_code, sizeof(uint32_t), buffer);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_create(
	char **path, int *cmode, int *ncid)
{
    iofw_buf_unpack_str(path, buffer);
    iofw_buf_unpack_data(cmode, sizeof(int), buffer);
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);

    debug(DEBUG_MSG, "path = %s; cmode = %d, ncid = %d", *path, *cmode, *ncid);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_def_dim(
	int *ncid, char **name, size_t *len, int *dimid)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_str(name, buffer);
    iofw_buf_unpack_data(len, sizeof(size_t), buffer);
    iofw_buf_unpack_data(dimid, sizeof(int), buffer);
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, len = %lu", *ncid, *name, *len);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_def_var(
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids, int *varid)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_str(name, buffer);
    iofw_buf_unpack_data(xtype, sizeof(nc_type), buffer);
    iofw_buf_unpack_data_array((void **)dimids, ndims, 
	    sizeof(int), buffer);
    iofw_buf_unpack_data(varid, sizeof(int), buffer);
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, ndims = %u", *ncid, *name, *ndims);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_enddef(
	int *ncid)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    debug(DEBUG_MSG, "ncid = %d", *ncid);

    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_put_vara(
	int *ncid, int *varid, int *ndims, 
	size_t **start, size_t **count,
	int *data_len, int *fp_type, char **fp)
{
    int i;

    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    iofw_buf_unpack_data(varid, sizeof(int), buffer);
    
    iofw_buf_unpack_data_array((void**)start, ndims, sizeof(size_t), buffer);
    iofw_buf_unpack_data_array((void**)count, ndims, sizeof(size_t), buffer);

//    iofw_buf_unpack_data_array_ptr((void**)fp, data_len, 
//	    sizeof(float), buffer);
//
    iofw_buf_unpack_data(fp_type, sizeof(int), buffer);
    switch(*fp_type)
    {
	case IOFW_BYTE :
	    iofw_buf_unpack_data_array((void**)fp, data_len, 1, buffer);
	    break;
	case IOFW_CHAR :
	    iofw_buf_unpack_data_array((void**)fp, data_len, 1, buffer);
	    break;
	case IOFW_SHORT :
	    iofw_buf_unpack_data_array((void**)fp, data_len, sizeof(short), buffer);
	    break;
	case IOFW_INT :
	    iofw_buf_unpack_data_array((void**)fp, data_len, sizeof(int), buffer);
	    break;
	case IOFW_FLOAT :
	    iofw_buf_unpack_data_array((void**)fp, data_len, sizeof(float), buffer);
	    break;
	case IOFW_DOUBLE :
	    iofw_buf_unpack_data_array((void**)fp, data_len, sizeof(double), buffer);
	    break;
    }

    debug(DEBUG_MSG, "ncid = %d, varid = %d, ndims = %d, data_len = %u", 
	    *ncid, *varid, *ndims, *data_len);
    debug(DEBUG_MSG, "fp[0] = %f", (*fp)[0]); 
    
    return IOFW_MSG_ERROR_NONE;
}

int iofw_msg_unpack_nc_close(int *ncid)
{
    iofw_buf_unpack_data(ncid, sizeof(int), buffer);
    debug(DEBUG_MSG, "ncid = %d", *ncid);

    return IOFW_MSG_ERROR_NONE;
}


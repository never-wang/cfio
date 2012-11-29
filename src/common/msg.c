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
#include "netcdf.h"
#include "debug.h"
#include "times.h"
#include "map.h"
#include "pthread.h"
#include "id.h"
#include "cfio_types.h"
#include "cfio_error.h"
#include "define.h"

static cfio_msg_t *msg_head;
cfio_buf_t *buffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline cfio_msg_t *create_msg()
{
    cfio_msg_t *msg;
    msg = malloc(sizeof(cfio_msg_t));
    if(NULL == msg)
    {
	return NULL;
    }
    memset(msg, 0, sizeof(cfio_msg_t));
    msg->addr = buffer->free_addr;

    return msg;
}

int cfio_msg_init(int buffer_size)
{
    int error;

    msg_head = malloc(sizeof(cfio_msg_t));
    if(NULL == msg_head)
    {
	return CFIO_ERROR_MALLOC;
    }
    INIT_QLIST_HEAD(&(msg_head->link));

    buffer = cfio_buf_open(buffer_size, &error);

    if(NULL == buffer)
    {
	error("");
	return error;
    }

    return CFIO_ERROR_NONE;
}

int cfio_msg_final()
{
    cfio_msg_t *msg, *next;
    MPI_Status status;
    int i = 0;

    if(msg_head != NULL)
    {
	i ++;
	debug(DEBUG_MSG, "wait msg : %d", i);
        qlist_for_each_entry_safe(msg, next, &(msg_head->link), link)
        {
	    MPI_Wait(&msg->req, &status);
            free(msg);
        }
        free(msg_head);
        msg_head = NULL;
    }

    cfio_buf_close(buffer);

    return CFIO_ERROR_NONE;
}

/**
 * @brief: free unsed space in client buffer
 *
 * @return: 
 */
static void cfio_msg_client_buf_free()
{
    cfio_msg_t *msg, *next;
    int done;
    MPI_Status status;

    //debug(DEBUG_MSG, "should not be here");
    //assert(0);

    msg = qlist_entry(qlist_pop(&msg_head->link), cfio_msg_t, link);
    MPI_Wait(&msg->req, &status);

    assert(check_used_addr(msg->addr, buffer));
    buffer->used_addr = msg->addr;
    free_buf(buffer, msg->size);
    free(msg);

    return;
}

static void cfio_msg_server_buf_free()
{

    pthread_cond_wait(&full_cond, &full_mutex);
    return;
}

int cfio_msg_isend(
	cfio_msg_t *msg)
{
    int tag = msg->src;
    MPI_Request request;
    MPI_Status status;
 
	//times_start();

    debug(DEBUG_MSG, "isend: src=%d; dst=%d; comm = %d; size = %lu", 
	    msg->src, msg->dst, msg->comm, msg->size);

    MPI_Isend(msg->addr, msg->size, MPI_BYTE, 
	    msg->dst, tag, msg->comm, &(msg->req));
    //MPI_Wait(&(msg->req), &status);
    //assert(check_used_addr(msg->addr, buffer));
    //buffer->used_addr = msg->addr;
    //free_buf(buffer, msg->size);

    ///**
    // *TODO , it's not good to put free here
    // **/
    //free(msg);

    qlist_add_tail(&(msg->link), &(msg_head->link));

    //debug(DEBUG_TIME, "%f ms", times_end());
    debug(DEBUG_MSG, "success return.");

    return CFIO_ERROR_NONE;
}

int cfio_msg_recv(int rank, MPI_Comm comm, cfio_msg_t **_msg)
{
    MPI_Status status;
    int size;
    cfio_msg_t *msg;

    //times_start();
    ensure_free_space(buffer, MSG_MAX_SIZE, cfio_msg_server_buf_free);
    //debug(DEBUG_TIME, "%f", times_end());

    MPI_Recv(buffer->free_addr, MSG_MAX_SIZE, MPI_BYTE, MPI_ANY_SOURCE, 
	    MPI_ANY_TAG, comm, &status);
    MPI_Get_count(&status, MPI_BYTE, &size);
    debug(DEBUG_MSG, "recv: size = %d", size);

    //debug(DEBUG_MSG, "code = %u", *((uint32_t *)buffer->free_addr));

    if(status.MPI_SOURCE != status.MPI_TAG)
    {
	return CFIO_ERROR_MPI_RECV;
    }

    msg = create_msg();
    msg->size = size;
    msg->src = status.MPI_SOURCE;
    msg->dst = rank;
    // get the func_code but not unpack it
    msg->func_code = *((uint32_t*)msg->addr); 
    *_msg = msg;

#ifndef SVR_RECV_ONLY
    use_buf(buffer, size);
#endif
    
    /* need lock */
    pthread_mutex_lock(&mutex);
    qlist_add_tail(&(msg->link), &(msg_head->link));
    pthread_cond_signal(&empty_cond);
    pthread_mutex_unlock(&mutex);
    
    //debug(DEBUG_MSG, "uesd_size = %lu", used_buf_size(buffer));
    debug(DEBUG_MSG, "success return");
    
    return CFIO_ERROR_NONE;
}

int cfio_msg_test()
{
    cfio_msg_t *msg;
    MPI_Status status;
    int flag;
    int removable = 1;
    
    qlist_for_each_entry(msg, &(msg_head->link), link)
    {
#ifdef MSG_CLOSE_WAIT
	MPI_Wait(&msg->req, &status);
	qlist_del(&(msg->link));
	free(msg);
#else
	MPI_Test(&msg->req, &flag, &status);
	if(removable)
	{
	    if(flag == 1)
	    {
		qlist_del(&(msg->link));
		free(msg);
	    }else
	    {
		removable = 0;
	    }
	}
#endif
    }

    return CFIO_ERROR_NONE;
}


cfio_msg_t *cfio_msg_get_first()
{
    cfio_msg_t *msg = NULL;
    qlist_head_t *link;

    pthread_cond_signal(&full_cond);

    while(NULL == msg)
    {
	pthread_mutex_lock(&mutex);
	link = qlist_pop(&(msg_head->link));
	if(NULL == link)
	{
	    msg = NULL;
	    pthread_cond_wait(&empty_cond, &mutex);
	}else
	{
	    msg = qlist_entry(link, cfio_msg_t, link);
	}
	pthread_mutex_unlock(&mutex);
    }

    if(msg != NULL)
    {
	debug(DEBUG_MSG, "get msg->size = %lu", msg->size);
    }
    return msg;
}

int cfio_msg_pack_create(
	cfio_msg_t **_msg, int client_proc_id, 
	const char *path, int cmode, int ncid)
{
    uint32_t code = FUNC_NC_CREATE;
    cfio_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;

    msg->size = 0;
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_str_size(path);
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_data_size(sizeof(int));

    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code, sizeof(uint32_t) , buffer);
    cfio_buf_pack_str(path, buffer);
    cfio_buf_pack_data(&cmode, sizeof(int), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "path = %s; cmode = %d, ncid = %d", path, cmode, ncid);

    return CFIO_ERROR_NONE;
}

/**
 *pack msg function
 **/
int cfio_msg_pack_def_dim(
	cfio_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, size_t len, int dimid)
{
    uint32_t code = FUNC_NC_DEF_DIM;
    cfio_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_str_size(name);
    msg->size += cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(int));

    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);
    
    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_str(name, buffer);
    cfio_buf_pack_data(&len, sizeof(size_t), buffer);
    cfio_buf_pack_data(&dimid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, len = %lu", ncid, name, len);

    return CFIO_ERROR_NONE;
}

int cfio_msg_pack_def_var(
	cfio_msg_t **_msg, int client_proc_id,
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, int varid)
{
    uint32_t code = FUNC_NC_DEF_VAR;
    cfio_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_str_size(name);
    msg->size += cfio_buf_data_size(sizeof(nc_type));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(int));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(int));

    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);
    
    debug_mark(DEBUG_MSG);
    
    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code , sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_str(name, buffer);
    cfio_buf_pack_data(&xtype, sizeof(nc_type), buffer);
    cfio_buf_pack_data_array(dimids, ndims, sizeof(int), buffer);
    cfio_buf_pack_data_array(start, ndims, sizeof(size_t), buffer);
    cfio_buf_pack_data_array(count, ndims, sizeof(size_t), buffer);
    cfio_buf_pack_data(&varid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, ndims = %u", ncid, name, ndims);

    return CFIO_ERROR_NONE;
}

int cfio_msg_pack_put_att(
	cfio_msg_t **_msg, int client_proc_id,
	int ncid, int varid, const char *name, 
	nc_type xtype, size_t len, const void *op)
{
    uint32_t code = FUNC_PUT_ATT;
    cfio_msg_t *msg;
    size_t att_size;

    msg = create_msg();
    msg->src = client_proc_id;

    cfio_types_size(att_size, xtype);

    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_str_size(name);
    msg->size += cfio_buf_data_size(sizeof(nc_type));
    msg->size += cfio_buf_data_array_size(len, att_size);

    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code , sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_data(&varid, sizeof(int), buffer);
    cfio_buf_pack_str(name, buffer);
    cfio_buf_pack_data(&xtype, sizeof(nc_type), buffer);
    cfio_buf_pack_data_array(op, len, att_size, buffer);

    cfio_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d, varid = %d, name = %s, len = %lu", 
	    ncid, varid, name, len);

    return CFIO_ERROR_NONE;

}

int cfio_msg_pack_enddef(
	cfio_msg_t **_msg, int client_proc_id,
	int ncid)
{
    uint32_t code = FUNC_NC_ENDDEF;
    cfio_msg_t *msg;

    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    
    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);
    
    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    *_msg = msg;
    
    debug(DEBUG_MSG, "ncid = %d", ncid);
    
    return CFIO_ERROR_NONE;
}

int cfio_msg_pack_put_vara(
	cfio_msg_t **_msg, int client_proc_id,
	int ncid, int varid, int ndims,
	const size_t *start, const size_t *count, 
	const int fp_type, const void *fp)
{
    int i;
    size_t data_len;
    uint32_t code = FUNC_NC_PUT_VARA;
    cfio_msg_t *msg;
    
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
    
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    switch(fp_type)
    {
	case CFIO_BYTE :
	    msg->size += cfio_buf_data_array_size(data_len, 1);
	    break;
	case CFIO_CHAR :
	    msg->size += cfio_buf_data_array_size(data_len, sizeof(char));
	    break;
	case CFIO_SHORT :
	    msg->size += cfio_buf_data_array_size(data_len, sizeof(short));
	    break;
	case CFIO_INT :
	    msg->size += cfio_buf_data_array_size(data_len, sizeof(int));
	    break;
	case CFIO_FLOAT :
	    msg->size += cfio_buf_data_array_size(data_len, sizeof(float));
	    break;
	case CFIO_DOUBLE :
	    msg->size += cfio_buf_data_array_size(data_len, sizeof(double));
	    break;
    }
	    
    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_data(&varid, sizeof(int), buffer);
    cfio_buf_pack_data_array(start, ndims, sizeof(size_t), buffer);
    cfio_buf_pack_data_array(count, ndims, sizeof(size_t), buffer);
    cfio_buf_pack_data(&fp_type, sizeof(int), buffer);
    switch(fp_type)
    {
	case CFIO_BYTE :
	    cfio_buf_pack_data_array(fp, data_len, 1, buffer);
	    break;
	case CFIO_CHAR :
	    cfio_buf_pack_data_array(fp, data_len, sizeof(char), buffer);
	    break;
	case CFIO_SHORT :
	    cfio_buf_pack_data_array(fp, data_len, sizeof(short), buffer);
	    break;
	case CFIO_INT :
	    cfio_buf_pack_data_array(fp, data_len, sizeof(int), buffer);
	    break;
	case CFIO_FLOAT :
	    cfio_buf_pack_data_array(fp, data_len, sizeof(float), buffer);
	    break;
	case CFIO_DOUBLE :
	    cfio_buf_pack_data_array(fp, data_len, sizeof(double), buffer);
	    break;
    }

    cfio_map_forwarding(msg);
    *_msg = msg;
    
    //debug(DEBUG_TIME, "%f ms", times_end());
    debug(DEBUG_MSG, "ncid = %d, varid = %d, ndims = %d, data_len = %lu", 
	    ncid, varid, ndims, data_len);

    return CFIO_ERROR_NONE;
}

int cfio_msg_pack_close(
	cfio_msg_t **_msg, int client_proc_id,
	int ncid)
{
    uint32_t code = FUNC_NC_CLOSE;
    cfio_msg_t *msg;
    
    //times_start();
    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    
    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    *_msg = msg;
    //debug(DEBUG_TIME, "%f", times_end());

    return CFIO_ERROR_NONE;
}

int cfio_msg_pack_io_done(
	cfio_msg_t **_msg, int client_proc_id)
{
    uint32_t code = FUNC_END_IO;
    cfio_msg_t *msg;
    
    msg = create_msg();
    msg->src = client_proc_id;
    
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    
    ensure_free_space(buffer, msg->size, cfio_msg_client_buf_free);
    
    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    
    cfio_map_forwarding(msg);
    *_msg = msg;
    
    return CFIO_ERROR_NONE;
}
/**
 *unpack msg function
 **/
int cfio_msg_unpack_func_code(cfio_msg_t *msg, uint32_t *func_code)
{
    assert(check_used_addr(msg->addr, buffer));
    
    //if(msg->addr == buffer->start_addr)
    //{
    //    debug_mark(DEBUG_MSG);
    //}

    buffer->used_addr = msg->addr;
    cfio_buf_unpack_data(func_code, sizeof(uint32_t), buffer);

    return CFIO_ERROR_NONE;
}

int cfio_msg_unpack_create(
	char **path, int *cmode, int *ncid)
{
    cfio_buf_unpack_str(path, buffer);
    cfio_buf_unpack_data(cmode, sizeof(int), buffer);
    cfio_buf_unpack_data(ncid, sizeof(int), buffer);

    debug(DEBUG_MSG, "path = %s; cmode = %d, ncid = %d", *path, *cmode, *ncid);

    return CFIO_ERROR_NONE;
}

int cfio_msg_unpack_def_dim(
	int *ncid, char **name, size_t *len, int *dimid)
{
    cfio_buf_unpack_data(ncid, sizeof(int), buffer);
    cfio_buf_unpack_str(name, buffer);
    cfio_buf_unpack_data(len, sizeof(size_t), buffer);
    cfio_buf_unpack_data(dimid, sizeof(int), buffer);
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, len = %lu", *ncid, *name, *len);

    return CFIO_ERROR_NONE;
}

int cfio_msg_unpack_def_var(
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids, 
	size_t **start, size_t **count, int *varid)
{
    cfio_buf_unpack_data(ncid, sizeof(int), buffer);
    cfio_buf_unpack_str(name, buffer);
    cfio_buf_unpack_data(xtype, sizeof(nc_type), buffer);
    cfio_buf_unpack_data_array((void **)dimids, ndims, 
	    sizeof(int), buffer);
    cfio_buf_unpack_data_array((void **)start, ndims, 
	    sizeof(size_t), buffer);
    cfio_buf_unpack_data_array((void **)count, ndims, 
	    sizeof(size_t), buffer);
    cfio_buf_unpack_data(varid, sizeof(int), buffer);
    
    debug(DEBUG_MSG, "ncid = %d, name = %s, ndims = %u", *ncid, *name, *ndims);

    return CFIO_ERROR_NONE;
}
int cfio_msg_unpack_put_att(
	int *ncid, int *varid, char **name, 
	nc_type *xtype, int *len, void **op)
{
    size_t data_size;

    cfio_buf_unpack_data(ncid, sizeof(int), buffer);
    cfio_buf_unpack_data(varid, sizeof(int), buffer);
    cfio_buf_unpack_str(name, buffer);
    cfio_buf_unpack_data(xtype, sizeof(nc_type), buffer);
    cfio_types_size(data_size, *xtype);
    cfio_buf_unpack_data_array(op, len, data_size, buffer);

    debug(DEBUG_MSG, "ncid = %d, varid = %d, name = %s, len = %d",
	    *ncid, *varid, *name, *len);

    return CFIO_ERROR_NONE;
}

int cfio_msg_unpack_enddef(
	int *ncid)
{
    cfio_buf_unpack_data(ncid, sizeof(int), buffer);
    debug(DEBUG_MSG, "ncid = %d", *ncid);

    return CFIO_ERROR_NONE;
}

int cfio_msg_unpack_put_vara(
	int *ncid, int *varid, int *ndims, 
	size_t **start, size_t **count,
	int *data_len, int *fp_type, char **fp)
{
    int i;

    cfio_buf_unpack_data(ncid, sizeof(int), buffer);
    cfio_buf_unpack_data(varid, sizeof(int), buffer);
    
    cfio_buf_unpack_data_array((void**)start, ndims, sizeof(size_t), buffer);
    cfio_buf_unpack_data_array((void**)count, ndims, sizeof(size_t), buffer);

//    cfio_buf_unpack_data_array_ptr((void**)fp, data_len, 
//	    sizeof(float), buffer);
//
    cfio_buf_unpack_data(fp_type, sizeof(int), buffer);
    switch(*fp_type)
    {
	case CFIO_BYTE :
	    cfio_buf_unpack_data_array((void**)fp, data_len, 1, buffer);
	    break;
	case CFIO_CHAR :
	    cfio_buf_unpack_data_array((void**)fp, data_len, 1, buffer);
	    break;
	case CFIO_SHORT :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(short), buffer);
	    break;
	case CFIO_INT :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(int), buffer);
	    break;
	case CFIO_FLOAT :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(float), buffer);
	    break;
	case CFIO_DOUBLE :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(double), buffer);
	    break;
    }

    debug(DEBUG_MSG, "ncid = %d, varid = %d, ndims = %d, data_len = %u", 
	    *ncid, *varid, *ndims, *data_len);
    //debug(DEBUG_MSG, "fp[0] = %f", (*fp)[0]); 
    
    return CFIO_ERROR_NONE;
}

int cfio_msg_unpack_close(int *ncid)
{
    cfio_buf_unpack_data(ncid, sizeof(int), buffer);
    debug(DEBUG_MSG, "ncid = %d", *ncid);

    return CFIO_ERROR_NONE;
}


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
#include "send.h"
#include "recv.h"
#include "netcdf.h"
#include "debug.h"
#include "times.h"
#include "map.h"
#include "pthread.h"
#include "id.h"
#include "cfio_types.h"
#include "cfio_error.h"
#include "define.h"
#include "quicklist.h"

static cfio_msg_t *msg_head, *merge_msg = NULL;
static cfio_buf_t *buffer;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t sender;

static int rank;

static int max_msg_size;

static inline int _send_msg(
	cfio_msg_t *msg)
{
    MPI_Send(msg->addr, msg->size, MPI_BYTE, msg->dst, msg->src, 
	    msg->comm);

    pthread_mutex_lock(&full_mutex);
    assert(check_used_addr(msg->addr, buffer));
    buffer->used_addr = msg->addr;
    free_buf(buffer, msg->size);
    pthread_mutex_unlock(&full_mutex);
    
    pthread_cond_signal(&full_cond);

    return CFIO_ERROR_NONE;
}

static inline void _add_msg(
	cfio_msg_t *msg)
{
    int tag = msg->src;
    MPI_Request request;
    MPI_Status status;
 
	//times_start();

    debug(DEBUG_SEND, "isend: src=%d; dst=%d; comm = %d; size = %lu", 
	    msg->src, msg->dst, msg->comm, msg->size);
    assert(msg->size <= max_msg_size);

//    MPI_Isend(msg->addr, msg->size, MPI_BYTE, 
//	    msg->dst, tag, msg->comm, &(msg->req));
    //MPI_Wait(&(msg->req), &status);
    //assert(check_used_addr(msg->addr, buffer));
    //buffer->used_addr = msg->addr;
    //free_buf(buffer, msg->size);

    ///**
    // *TODO , it's not good to put free here
    // **/
    //free(msg);
    pthread_mutex_lock(&mutex);
    if(msg->func_code == FUNC_END_IO)
    {
	if(merge_msg != NULL)
	{
	    qlist_add_tail(&(merge_msg->link), &(msg_head->link));
	}
	qlist_add_tail(&(msg->link), &(msg_head->link));
    }else
    {
	if(merge_msg != NULL)
	{
	    if(merge_msg->addr < msg->addr && 
		    (msg->size + merge_msg->size) <= max_msg_size)
	    {
		assert(msg->addr - merge_msg->addr == merge_msg->size);
		merge_msg->size += msg->size;
		free(msg);
	    }else
	    {
		qlist_add_tail(&(merge_msg->link), &(msg_head->link));
		merge_msg = msg;
	    }
	}else
	{
	    merge_msg = msg;
	}
    }
    //if(!qlist_empty(&(msg_head->link)) && msg->func_code != FUNC_END_IO)
    //{
    //    last_msg = qlist_entry(msg_head->link.prev, cfio_msg_t, link);
    //    if(last_msg->addr < msg->addr && 
    //    	(msg->size + last_msg->size) <= max_msg_size)
    //    {
    //        assert(msg->addr - last_msg->addr == last_msg->size);
    //        last_msg->size += msg->size;
    //        free(msg);
    //    }else
    //    {
    //        qlist_add_tail(&(msg->link), &(msg_head->link));
    //    }
    //}else
    //{
    //    qlist_add_tail(&(msg->link), &(msg_head->link));
    //}
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&empty_cond);

    //debug(DEBUG_TIME, "%f ms", times_end());
    debug(DEBUG_SEND, "success return.");
}

static inline cfio_msg_t *_get_first_msg()
{
    cfio_msg_t *msg = NULL;
    qlist_head_t *link;

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

    if(msg != NULL)
    {
	debug(DEBUG_SEND, "get msg size : %lu", msg->size);
    }

    return msg;
}

static void* sender_thread(void *arg)
{
    cfio_msg_t *msg;
    int sender_finish = 0;

    while(sender_finish == 0)
    {
	msg = _get_first_msg();
	if(msg != NULL)
	{
	    _send_msg(msg);
	    if(msg->func_code == FUNC_END_IO)
	    {
		sender_finish = 1;
	    }
	}
	free(msg);
    }

    debug(DEBUG_CFIO, "Proc %d : sender finish", rank);
    return (void*)0;
}

int cfio_send_init()
{
    int error, ret, server_id, client_num_of_server;

    msg_head = malloc(sizeof(cfio_msg_t));
    if(NULL == msg_head)
    {
	return CFIO_ERROR_MALLOC;
    }
    INIT_QLIST_HEAD(&(msg_head->link));

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    max_msg_size = cfio_msg_get_max_size(rank);
    
    buffer = cfio_buf_open(SEND_BUF_SIZE, &error);

    if(NULL == buffer)
    {
	error("");
	return error;
    }

    if( (ret = pthread_create(&sender,NULL,sender_thread,NULL))<0  )
    {
	error("Thread Writer create error()");
	return CFIO_ERROR_PTHREAD_CREATE;
    }

    return CFIO_ERROR_NONE;
}

int cfio_send_final()
{
    cfio_msg_t *msg, *next;
    MPI_Status status;
    int i = 0;

    //if(msg_head != NULL)
    //{
    //    i ++;
    //    debug(DEBUG_SEND, "wait msg : %d", i);
    //    qlist_for_each_entry_safe(msg, next, &(msg_head->link), link)
    //    {
    //        MPI_Wait(&msg->req, &status);
    //        free(msg);
    //    }
    //    free(msg_head);
    //    msg_head = NULL;
    //}

    pthread_join(sender, NULL);
    
    if(msg_head != NULL)
    {
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
static void cfio_send_client_buf_free()
{
    pthread_cond_wait(&full_cond, &full_mutex);

    return;
}

int cfio_send_create(
	const char *path, int cmode, int ncid)
{
    uint32_t code = FUNC_NC_CREATE;
    cfio_msg_t *msg;

    debug(DEBUG_SEND, "path = %s; cmode = %d, ncid = %d", path, cmode, ncid);
    msg = cfio_msg_create();
    msg->src = rank;

    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_str_size(path);
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_data_size(sizeof(int));

    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code, sizeof(uint32_t) , buffer);
    cfio_buf_pack_str(path, buffer);
    cfio_buf_pack_data(&cmode, sizeof(int), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    _add_msg(msg);
    
    debug(DEBUG_SEND, "path = %s; cmode = %d, ncid = %d", path, cmode, ncid);

    return CFIO_ERROR_NONE;
}

/**
 *pack msg function
 **/
int cfio_send_def_dim(
	int ncid, const char *name, size_t len, int dimid)
{
    uint32_t code = FUNC_NC_DEF_DIM;
    cfio_msg_t *msg;

    msg = cfio_msg_create();
    msg->src = rank;
    
    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_str_size(name);
    msg->size += cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(int));

    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);
    
    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_str(name, buffer);
    cfio_buf_pack_data(&len, sizeof(size_t), buffer);
    cfio_buf_pack_data(&dimid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    _add_msg(msg);
    
    debug(DEBUG_SEND, "ncid = %d, name = %s, len = %lu", ncid, name, len);

    return CFIO_ERROR_NONE;
}

int cfio_send_def_var(
	int ncid, const char *name, nc_type xtype,
	int ndims, const int *dimids, 
	const size_t *start, const size_t *count, int varid)
{
    uint32_t code = FUNC_NC_DEF_VAR;
    cfio_msg_t *msg;

    msg = cfio_msg_create();
    msg->src = rank;
    
    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_str_size(name);
    msg->size += cfio_buf_data_size(sizeof(nc_type));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(int));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += cfio_buf_data_array_size(ndims, sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(int));

    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);
    
    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code , sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_str(name, buffer);
    cfio_buf_pack_data(&xtype, sizeof(nc_type), buffer);
    cfio_buf_pack_data_array(dimids, ndims, sizeof(int), buffer);
    cfio_buf_pack_data_array(start, ndims, sizeof(size_t), buffer);
    cfio_buf_pack_data_array(count, ndims, sizeof(size_t), buffer);
    cfio_buf_pack_data(&varid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    _add_msg(msg);
    
    debug(DEBUG_SEND, "ncid = %d, name = %s, ndims = %u", ncid, name, ndims);

    return CFIO_ERROR_NONE;
}

int cfio_send_put_att(
	int ncid, int varid, const char *name, 
	nc_type xtype, size_t len, const void *op)
{
    uint32_t code = FUNC_PUT_ATT;
    cfio_msg_t *msg;
    size_t att_size;

    msg = cfio_msg_create();
    msg->src = rank;

    cfio_types_size(att_size, xtype);

    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_data_size(sizeof(int));
    msg->size += cfio_buf_str_size(name);
    msg->size += cfio_buf_data_size(sizeof(nc_type));
    msg->size += cfio_buf_data_array_size(len, att_size);

    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code , sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);
    cfio_buf_pack_data(&varid, sizeof(int), buffer);
    cfio_buf_pack_str(name, buffer);
    cfio_buf_pack_data(&xtype, sizeof(nc_type), buffer);
    cfio_buf_pack_data_array(op, len, att_size, buffer);

    cfio_map_forwarding(msg);
    _add_msg(msg);
    
    debug(DEBUG_SEND, "ncid = %d, varid = %d, name = %s, len = %lu", 
	    ncid, varid, name, len);

    return CFIO_ERROR_NONE;

}

int cfio_send_enddef(
	int ncid)
{
    uint32_t code = FUNC_NC_ENDDEF;
    cfio_msg_t *msg;

    msg = cfio_msg_create();
    msg->src = rank;
    
    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    
    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);
    
    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    _add_msg(msg);
    
    debug(DEBUG_SEND, "ncid = %d", ncid);
    
    return CFIO_ERROR_NONE;
}

int cfio_send_put_vara(
	int ncid, int varid, int ndims,
	const size_t *start, const size_t *count, 
	const int fp_type, const void *fp)
{
    int i;
    size_t data_len;
    uint32_t code = FUNC_NC_PUT_VARA;
    cfio_msg_t *msg;
    
    //times_start();

    debug(DEBUG_SEND, "pack_msg_put_vara_float");
    for(i = 0; i < ndims; i ++)
    {
	debug(DEBUG_SEND, "start[%d] = %lu", i, start[i]);
    }
    for(i = 0; i < ndims; i ++)
    {
	debug(DEBUG_SEND, "count[%d] = %lu", i, count[i]);
    }
    
    data_len = 1;
    for(i = 0; i < ndims; i ++)
    {
	data_len *= count[i]; 
    }
    
    msg = cfio_msg_create();
    msg->src = rank;
    
    msg->size = cfio_buf_data_size(sizeof(size_t));
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
	    
    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
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
    _add_msg(msg);
    
    //debug(DEBUG_TIME, "%f ms", times_end());
    debug(DEBUG_SEND, "ncid = %d, varid = %d, ndims = %d, data_len = %lu", 
	    ncid, varid, ndims, data_len);

    return CFIO_ERROR_NONE;
}

int cfio_send_close(
	int ncid)
{
    uint32_t code = FUNC_NC_CLOSE;
    cfio_msg_t *msg;
    
    //times_start();
    msg = cfio_msg_create();
    msg->src = rank;
    
    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    msg->size += cfio_buf_data_size(sizeof(int));
    
    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);

    msg->addr = buffer->free_addr;

    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    cfio_buf_pack_data(&ncid, sizeof(int), buffer);

    cfio_map_forwarding(msg);
    _add_msg(msg);
    //debug(DEBUG_TIME, "%f", times_end());

    return CFIO_ERROR_NONE;
}

int cfio_send_io_done()
{
    uint32_t code = FUNC_END_IO;
    cfio_msg_t *msg;
    
    msg = cfio_msg_create();
    msg->src = rank;
    msg->func_code = code;
    
    msg->size = cfio_buf_data_size(sizeof(size_t));
    msg->size += cfio_buf_data_size(sizeof(uint32_t));
    
    pthread_mutex_lock(&full_mutex);
    ensure_free_space(buffer, msg->size, cfio_send_client_buf_free);
    pthread_mutex_unlock(&full_mutex);
    
    msg->addr = buffer->free_addr;
    
    cfio_buf_pack_data(&msg->size, sizeof(size_t) , buffer);
    cfio_buf_pack_data(&code, sizeof(uint32_t), buffer);
    
    cfio_map_forwarding(msg);
    _add_msg(msg);
    
    return CFIO_ERROR_NONE;
}

int cfio_send_io_end()
{
    pthread_mutex_lock(&mutex);
    if(merge_msg != NULL)
    {
	qlist_add_tail(&(merge_msg->link), &(msg_head->link));
	merge_msg = NULL;
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&empty_cond);
    
    return CFIO_ERROR_NONE;
}


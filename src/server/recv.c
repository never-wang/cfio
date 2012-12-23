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
#include "recv.h"
#include "send.h"
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
//use two buffer swap, in client :writer for pack, reader for send
static cfio_buf_t **buffer;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t empty_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER;
static int rank;
static int client_num;
/* index used when call cfio_recv_get_first */
static int client_get_index = 0;
static int max_msg_size;

int cfio_recv_init()
{
    int i, error;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    client_num = cfio_map_get_client_num_of_server(rank);

    msg_head = malloc(client_num * sizeof(cfio_msg_t));
    if(NULL == msg_head)
    {
	return CFIO_ERROR_MALLOC;
    }
    for(i = 0; i < client_num; i ++)
    {
	INIT_QLIST_HEAD(&(msg_head[i].link));
    }

    buffer = malloc(client_num *sizeof(cfio_buf_t*));
    if(NULL == buffer)
    {
	return CFIO_ERROR_MALLOC;
    }
    for(i = 0; i < client_num; i ++)
    {
	buffer[i] = cfio_buf_open(RECV_BUF_SIZE / client_num, &error);
	if(NULL == buffer[i])
	{
	    error("");
	    return error;
	}
    }

    max_msg_size = RECV_BUF_SIZE / client_num / 2;
    if(max_msg_size > SEND_BUF_SIZE / 2)
    {
	max_msg_size = SEND_BUF_SIZE / 2;
    }

    return CFIO_ERROR_NONE;
}

int cfio_recv_final()
{
    cfio_msg_t *msg, *next;
    MPI_Status status;
    int i = 0;

    if(msg_head != NULL)
    {
	free(msg_head);
    }

    if(buffer != NULL)
    {
	for(i = 0; i < client_num; i ++)
	{
	    cfio_buf_close(buffer[i]);
	}
	free(buffer);
    }

    return CFIO_ERROR_NONE;
}

static void cfio_recv_server_buf_free()
{
    pthread_mutex_lock(&full_mutex);
    pthread_cond_wait(&full_cond, &full_mutex);
    pthread_mutex_unlock(&full_mutex);
    return;
}

int cfio_recv(
	int src, int rank, MPI_Comm comm, uint32_t *func_code)
{
    MPI_Status status;
    int size;
    cfio_msg_t *msg;
    int client_index;

    client_index = cfio_map_get_client_index_of_server(src);
    //times_start();
    debug(DEBUG_RECV, "client_index = %d", client_index);
    ensure_free_space(buffer[client_index], max_msg_size, 
	    cfio_recv_server_buf_free);
    //debug(DEBUG_TIME, "%f", times_end());

    MPI_Recv(buffer[client_index]->free_addr, max_msg_size, MPI_BYTE, src,  
	    MPI_ANY_TAG, comm, &status);
    MPI_Get_count(&status, MPI_BYTE, &size);
    debug(DEBUG_RECV, "recv: size = %d", size);

    //printf("proc %d , recv: size = %d, from %d\n",rank, size, src);
    //debug(DEBUG_RECV, "code = %u", *((uint32_t *)buffer->free_addr));

    if(status.MPI_SOURCE != status.MPI_TAG)
    {
	return CFIO_ERROR_MPI_RECV;
    }

    msg = cfio_msg_create();
    msg->addr = buffer[client_index]->free_addr;
    msg->size = size;
    msg->src = status.MPI_SOURCE;
    msg->dst = rank;
    // get the func_code but not unpack it
    msg->func_code = *((uint32_t*)(msg->addr + sizeof(size_t))); 
    *func_code = msg->func_code;
    debug(DEBUG_RECV, "func_code = %u", *func_code);

#ifndef SVR_RECV_ONLY
    use_buf(buffer[client_index], size);
#endif
    
    /* need lock */
    pthread_mutex_lock(&mutex);
    qlist_add_tail(&(msg->link), &(msg_head[client_index].link));
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&empty_cond);
    
    //debug(DEBUG_RECV, "uesd_size = %lu", used_buf_size(buffer));
    debug(DEBUG_RECV, "success return");
    
    return CFIO_ERROR_NONE;
}

cfio_msg_t *cfio_recv_get_first()
{
    cfio_msg_t *_msg = NULL, *msg;
    qlist_head_t *link;
    size_t size;

    pthread_cond_signal(&full_cond);

    pthread_mutex_lock(&mutex);
    debug(DEBUG_RECV, "client_get_index = %d", client_get_index);
    if(qlist_empty(&(msg_head[client_get_index].link)))
    {
	link = NULL;
    }else
    {
	link = msg_head[client_get_index].link.next;
    }

    if(NULL == link)
    {
	msg = NULL;
	pthread_cond_wait(&empty_cond, &mutex);
    }else
    {
	msg = qlist_entry(link, cfio_msg_t, link);
	cfio_recv_unpack_msg_size(msg, &size);
	if(msg->size == size) //only contain one single msg
	{
	    qlist_del(link);
	    _msg = msg;
	}else
	{
	    msg->size -= size;
	    _msg = cfio_msg_create();
	    _msg->addr = msg->addr;
	    _msg->src = msg->src;
	    _msg->dst = msg->dst;
	    msg->addr += size;
	}
	client_get_index = (client_get_index + 1) % client_num;
    }
    pthread_mutex_unlock(&mutex);

    if(_msg != NULL)
    {
	debug(DEBUG_RECV, "get msg size : %lu", _msg->size);
    }

    return _msg;
}

/**
 *unpack msg function
 **/
int cfio_recv_unpack_msg_size(cfio_msg_t *msg, size_t *size)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    debug(DEBUG_RECV, "client_index = %d", client_index);

//    printf("%lu : %lu : %lu \n", msg->addr, buffer[client_index]->start_addr,
//	buffer[client_index]->start_addr + buffer[client_index]->size);

    assert(check_used_addr(msg->addr, buffer[client_index]));
    
    buffer[client_index]->used_addr = msg->addr;
    cfio_buf_unpack_data(size, sizeof(size_t), buffer[client_index]);

    debug(DEBUG_RECV, "size : %lu", *size);

    return CFIO_ERROR_NONE;
}

int cfio_recv_unpack_func_code(cfio_msg_t *msg, uint32_t *func_code)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    debug(DEBUG_RECV, "client_index = %d", client_index);

    //if(msg->addr == buffer->start_addr)
    //{
    //    debug_mark(DEBUG_RECV);
    //}

    cfio_buf_unpack_data(func_code, sizeof(uint32_t), buffer[client_index]);

    return CFIO_ERROR_NONE;
}

int cfio_recv_unpack_create(
	cfio_msg_t *msg,
	char **path, int *cmode, int *ncid)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    
    cfio_buf_unpack_str(path, buffer[client_index]);
    cfio_buf_unpack_data(cmode, sizeof(int), buffer[client_index]);
    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);

    debug(DEBUG_RECV, "path = %s; cmode = %d, ncid = %d", *path, *cmode, *ncid);

    return CFIO_ERROR_NONE;
}

int cfio_recv_unpack_def_dim(
	cfio_msg_t *msg,
	int *ncid, char **name, size_t *len, int *dimid)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);

    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);
    cfio_buf_unpack_str(name, buffer[client_index]);
    cfio_buf_unpack_data(len, sizeof(size_t), buffer[client_index]);
    cfio_buf_unpack_data(dimid, sizeof(int), buffer[client_index]);
    
    debug(DEBUG_RECV, "ncid = %d, name = %s, len = %lu", *ncid, *name, *len);

    return CFIO_ERROR_NONE;
}

int cfio_recv_unpack_def_var(
	cfio_msg_t *msg,
	int *ncid, char **name, nc_type *xtype,
	int *ndims, int **dimids, 
	size_t **start, size_t **count, int *varid)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    
    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);
    cfio_buf_unpack_str(name, buffer[client_index]);
    cfio_buf_unpack_data(xtype, sizeof(nc_type), buffer[client_index]);
    cfio_buf_unpack_data_array((void **)dimids, ndims, 
	    sizeof(int), buffer[client_index]);
    cfio_buf_unpack_data_array((void **)start, ndims, 
	    sizeof(size_t), buffer[client_index]);
    cfio_buf_unpack_data_array((void **)count, ndims, 
	    sizeof(size_t), buffer[client_index]);
    cfio_buf_unpack_data(varid, sizeof(int), buffer[client_index]);
    
    debug(DEBUG_RECV, "ncid = %d, name = %s, ndims = %u", *ncid, *name, *ndims);

    return CFIO_ERROR_NONE;
}
int cfio_recv_unpack_put_att(
	cfio_msg_t *msg,
	int *ncid, int *varid, char **name, 
	nc_type *xtype, int *len, void **op)
{
    size_t att_size;
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    

    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);
    cfio_buf_unpack_data(varid, sizeof(int), buffer[client_index]);
    cfio_buf_unpack_str(name, buffer[client_index]);
    cfio_buf_unpack_data(xtype, sizeof(nc_type), buffer[client_index]);
    
    cfio_types_size(att_size, *xtype);
    cfio_buf_unpack_data_array(op, len, att_size, buffer[client_index]);

    debug(DEBUG_RECV, "ncid = %d, varid = %d, name = %s, len = %d",
	    *ncid, *varid, *name, *len);

    return CFIO_ERROR_NONE;
}

int cfio_recv_unpack_enddef(
	cfio_msg_t *msg,
	int *ncid)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    
    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);
    debug(DEBUG_RECV, "ncid = %d", *ncid);

    return CFIO_ERROR_NONE;
}

int cfio_recv_unpack_put_vara(
	cfio_msg_t *msg,
	int *ncid, int *varid, int *ndims, 
	size_t **start, size_t **count,
	int *data_len, int *fp_type, char **fp)
{
    int i;
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    

    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);
    cfio_buf_unpack_data(varid, sizeof(int), buffer[client_index]);
    
    cfio_buf_unpack_data_array((void**)start, ndims, sizeof(size_t), 
	    buffer[client_index]);
    cfio_buf_unpack_data_array((void**)count, ndims, sizeof(size_t),
	    buffer[client_index]);

//    cfio_buf_unpack_data_array_ptr((void**)fp, data_len, 
//	    sizeof(float), buffer[client_index]);
//
    cfio_buf_unpack_data(fp_type, sizeof(int), buffer[client_index]);
    switch(*fp_type)
    {
	case CFIO_BYTE :
	    cfio_buf_unpack_data_array((void**)fp, data_len, 1, 
		    buffer[client_index]);
	    break;
	case CFIO_CHAR :
	    cfio_buf_unpack_data_array((void**)fp, data_len, 1, 
		    buffer[client_index]);
	    break;
	case CFIO_SHORT :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(short), 
		    buffer[client_index]);
	    break;
	case CFIO_INT :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(int), 
		    buffer[client_index]);
	    break;
	case CFIO_FLOAT :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(float), 
		    buffer[client_index]);
	    break;
	case CFIO_DOUBLE :
	    cfio_buf_unpack_data_array((void**)fp, data_len, sizeof(double), 
		    buffer[client_index]);
	    break;
    }

    debug(DEBUG_RECV, "ncid = %d, varid = %d, ndims = %d, data_len = %u", 
	    *ncid, *varid, *ndims, *data_len);
    //debug(DEBUG_RECV, "fp[0] = %f", (*fp)[0]); 
    
    return CFIO_ERROR_NONE;
}
	
int cfio_recv_unpack_close(
	cfio_msg_t *msg,
	int *ncid)
{
    int client_index;

    client_index = cfio_map_get_client_index_of_server(msg->src);
    
    cfio_buf_unpack_data(ncid, sizeof(int), buffer[client_index]);
    debug(DEBUG_RECV, "ncid = %d", *ncid);

    return CFIO_ERROR_NONE;
}


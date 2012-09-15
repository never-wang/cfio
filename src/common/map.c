/****************************************************************************
 *       Filename:  map.c
 *
 *    Description:  map from IO proc to IO Forwarding Proc
 *
 *        Version:  1.0
 *        Created:  12/14/2011 02:47:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>

#include "map.h"
#include "debug.h"

static int client_amount;
static int client_x_num;
static int client_y_num;
static MPI_Comm comm;
static int server_amount;

int iofw_map_init(
	int _client_x_num, int _client_y_num,
	int _server_amount, MPI_Comm _comm)
{
    int i;

    client_x_num = _client_x_num;
    client_y_num = _client_y_num;
    client_amount = client_x_num * client_y_num;
    comm = _comm;

    server_amount = _server_amount;
    
    debug(DEBUG_MAP, "success return.");
    return IOFW_MAP_ERROR_NONE;
}
int iofw_map_final()
{
    return IOFW_MAP_ERROR_NONE;
}

int iofw_map_get_server_amount(int *_server_amount)
{
    *_server_amount = server_amount;
    return IOFW_MAP_ERROR_NONE;
}
int iofw_map_get_client_amount()
{
    return client_amount;
}

int iofw_map_get_client_num_of_server(int server_id, int *client_num)
{
    int client_num_per_server, low_server_num;

    low_server_num = server_amount - (client_y_num % server_amount);
    client_num_per_server = (client_y_num / server_amount) * client_x_num;
    
    if(server_id < low_server_num) 
    {
	/**
	 *low servers
	 **/
	*client_num = client_num_per_server;
    }else
    {
	*client_num = client_num_per_server + client_x_num;
    }

    debug(DEBUG_MAP, "client number of server(%d) : %d", server_id, *client_num);
    return IOFW_MAP_ERROR_NONE;
}

int iofw_map_get_client_index_of_server(int client_id, int *client_index)
{
    int client_num_per_server, low_server_num;

    low_server_num = server_amount - (client_y_num % server_amount);
    client_num_per_server = (client_y_num / server_amount) * client_x_num;
    
    if(client_num_per_server * low_server_num > client_id) 
    {
	/**
	 *low servers
	 **/
	*client_index = client_id % client_num_per_server;
    }else
    {
	client_id -= client_num_per_server * low_server_num;
	client_num_per_server += client_x_num;
	*client_index = client_id % client_num_per_server;
    }
    
    debug(DEBUG_MAP, "client index of client(%d) : %d", client_id, *client_index);
    return IOFW_MAP_ERROR_NONE;
}

int iofw_map_forwarding(
	iofw_msg_t *msg)
{
    int x_proc, y_proc;
    int low_server_num, client_y_per_server;

    x_proc = msg->src % client_x_num;
    y_proc = msg->src / client_x_num;
    
    assert(client_y_num >= server_amount);

    low_server_num = server_amount - (client_y_num % server_amount);
    client_y_per_server = client_y_num / server_amount;
    
    if(client_y_per_server * low_server_num <= y_proc)
    {
	msg->dst = (y_proc - client_y_per_server * low_server_num) / 
	    (client_y_per_server + 1) + low_server_num;
    }else
    {
	msg->dst = y_proc / client_y_per_server; 
    }
    
    msg->comm = comm;

    debug(DEBUG_MAP, "client(%d)->server(%d)", msg->src, msg->dst);

    debug(DEBUG_MAP, "success return.");
    return IOFW_MAP_ERROR_NONE;
}

int iofw_map_is_bitmap_full(
	int server_id, uint8_t *bitmap, int *is_full)
{
    int client_num_per_server;
    int start_client, end_client;
    int low_server_num;
    int start_index, end_index, head, tail;
    int i;

    low_server_num = server_amount - (client_y_num % server_amount);
    client_num_per_server = (client_y_num / server_amount) * client_x_num;
    
    if(server_id < low_server_num) 
    {
	/**
	 *low servers
	 **/
	start_client = client_num_per_server * server_id;
	end_client = start_client + client_num_per_server - 1;
    }else
    {
	start_client = client_num_per_server * low_server_num;
	client_num_per_server += client_x_num;
	start_client += client_num_per_server * (server_id - low_server_num);
	end_client = start_client + client_num_per_server - 1;
    }

    start_index = start_client >> 3;
    head = start_client - (start_index << 3);
    end_index = end_client >> 3;
    tail = end_client - (end_index << 3) + 1;
    
    debug(DEBUG_MAP, "start : %d; end : %d; head : %d; tail : %d; bitmap : %x", 
	    start_index, end_index, head, tail, bitmap[0]);

    for(i = start_index + 1; i < end_index; i ++)
    {
	if(bitmap[i] != 255)
	{
	    *is_full = 0;
	    return IOFW_MAP_ERROR_NONE;
	}
    }

    if(start_index == end_index)
    {
	if((bitmap[start_index] >> head) != (1 << (tail - head)) - 1)
	{
	    *is_full = 0;
	    return IOFW_MAP_ERROR_NONE;
	}
    }else
    {
    if((bitmap[start_index] >> head) != ((1 << (8 - head)) - 1))
    {
	*is_full = 0;
	return IOFW_MAP_ERROR_NONE;
    }
    if(bitmap[end_index] != ((1 << tail) - 1))
    {
	*is_full = 0;
	return IOFW_MAP_ERROR_NONE;
    }
    }

    *is_full = 1;
    debug(DEBUG_MAP, "success return, bitmap is full."); 
    return IOFW_MAP_ERROR_NONE;
}

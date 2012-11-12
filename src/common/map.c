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

#include "mpi.h"
#include "map.h"
#include "debug.h"
#include "iofw_error.h"

static int client_amount;
static int client_x_num;
static int client_y_num;
static MPI_Comm comm;
static int server_amount;
static int server_x_num;
static int server_y_num;

/**
 * @brief: get all factor of a interger n
 *
 * @param n: the integer
 * @param factor: pointer to where store all factor
 * @param _factor_num: factor number
 */
static void _get_factor(int n, int *factor, int *_factor_num)
{
    int factor_num = 0;
    int index;
    int i;

    for(i = 1; i * i <= n; i ++)
    {
	if((n % i) == 0)
	{
	    factor[factor_num] = i;
	    factor_num ++;
	}
    }
    index = factor_num - 1;
    
    if(factor[index] * factor[index] == n)
    {
	index --;
    }
    
    while(index >= 0)
    {
	factor[factor_num] = n / factor[index];
	index --;
	factor_num ++;
    }
    *_factor_num = factor_num;
}

/**
 * @brief: computer server_x_num and server_y_num, adjust server_amount, exam the
 *	validity of init arg
 *
 * @return: 
 */
static int _gen_server_x_and_y(int best_server_amount)
{
    int *factor_x, *factor_y;
    int factor_x_num, factor_y_num;
    int index_x, index_y;
    int min_index_x, min_index_y, min_sub;
    int mul, sub;

    debug(DEBUG_MAP, "best_server_amount : %d; client_amount : %d",
	    best_server_amount, client_amount);

    factor_x = malloc(client_x_num * sizeof(int));
    if(factor_x == NULL)
    {
	error("malloc fail for factor_x.");
	return IOFW_ERROR_MALLOC;
    }
    factor_y = malloc(client_y_num * sizeof(int));
    if(factor_y == NULL)
    {
	free(factor_x);
	error("malloc fail for factor_y.");
	return IOFW_ERROR_MALLOC;
    }
    _get_factor(client_x_num, factor_x, &factor_x_num);
    _get_factor(client_y_num, factor_y, &factor_y_num);

    index_x = 0; index_y = factor_y_num - 1;

    min_sub = client_amount;
    while((index_x < factor_x_num) && (index_y >= 0))
    {
	mul = factor_x[index_x] * factor_y[index_y];
	sub =  best_server_amount - mul;
	if(sub == 0)
	{
	    min_sub = sub;
	    min_index_x = index_x;
	    min_index_y = index_y;
	    break;
	}else if(sub > 0)
	{
	    index_x ++;
	}else
	{
	    index_y --;
	    sub = - sub;
	}
	if(sub < min_sub)
	{
	    min_index_x = index_x;
	    min_index_y = index_y;
	}
    }

    if(((double)(min_sub) / (double)(best_server_amount)) < 0.1)
    {
	best_server_amount = factor_x[min_index_x] * factor_y[min_index_y];
	debug(DEBUG_MAP, "best_server_amount : %d(%d*%d); client_amount : %d",
		best_server_amount, factor_x[min_index_x], factor_y[min_index_y],
		client_amount);
	if(server_amount < best_server_amount)
	{
	    error("You should start more proccess, the best value is %d",
		    best_server_amount + client_amount);
	    free(factor_x);
	    free(factor_y);
	    return IOFW_ERROR_INVALID_INIT_ARG;
	}
	/* reassign server amount for some on may start more proc than needed */
	server_amount = best_server_amount; 
	server_x_num = factor_x[min_index_x];
	server_y_num = factor_y[min_index_y];
	free(factor_x);
	free(factor_y);
	debug(DEBUG_MAP, "success return");
	return IOFW_ERROR_NONE;
    }else
    {
	error("You should start proper amount of proccess, the best value is %d",
		best_server_amount + client_amount);
	free(factor_x);
	free(factor_y);
	return IOFW_ERROR_INVALID_INIT_ARG;
    }
}

int iofw_map_init(
	int _client_x_num, int _client_y_num,
	int _server_amount, int best_server_amount,
	MPI_Comm _comm)
{
    assert(_client_x_num > 0);
    assert(_client_y_num > 0);
    assert(best_server_amount > 0);

    int i, ret;

    client_x_num = _client_x_num;
    client_y_num = _client_y_num;
    client_amount = client_x_num * client_y_num;
    comm = _comm;

    server_amount = _server_amount;

    if((ret = _gen_server_x_and_y(best_server_amount)) < 0)
    {
	error("");
	return ret;
    }
    
    debug(DEBUG_MAP, "success return.");
    return IOFW_ERROR_NONE;
}
int iofw_map_final()
{
    return IOFW_ERROR_NONE;
}
int iofw_map_proc_type(int proc_id)
{
    assert(proc_id >= 0);
    /* client is 0 ~ client_amount - 1 , server is client_amount ~ server_amount
     * + client_amount - 1 */
    if(proc_id < client_amount)
    {
	return IOFW_MAP_TYPE_CLIENT;
    }else if(proc_id < client_amount + server_amount)
    {
	return IOFW_MAP_TYPE_SERVER;
    }else
    {
	return IOFW_MAP_TYPE_BLANK;
    }
}
int iofw_map_get_comm()
{
    return comm; 
}

int iofw_map_get_server_amount()
{
    return server_amount;
}
int iofw_map_get_client_amount()
{
    debug(DEBUG_MAP, "client_amount : %d", client_amount);
    return client_amount;
}

int iofw_map_get_client_num_of_server(int server_id)
{

    assert(iofw_map_proc_type(server_id) == IOFW_MAP_TYPE_SERVER);
    /* consider only partition of one dimension, and not  divisible(zheng cu)*/
    //int client_num_per_server, low_server_num;
    //low_server_num = server_amount - (client_y_num % server_amount);
    //client_num_per_server = (client_y_num / server_amount) * client_x_num;
    //
    //if(server_id < low_server_num) 
    //{
    //    /**
    //     *low servers
    //     **/
    //    *client_num = client_num_per_server;
    //}else
    //{
    //    *client_num = client_num_per_server + client_x_num;
    //}
    
    /* consider partition of two dimension and only divisible*/
    int client_per_server, client_num;

    client_per_server = client_amount / server_amount;
    client_num = client_per_server;

    debug(DEBUG_MAP, "client number of server(%d) : %d", server_id, client_num);
    return client_num;
}

int iofw_map_get_client_index_of_server(int client_id)
{
    assert(iofw_map_proc_type(client_id) == IOFW_MAP_TYPE_CLIENT);
    /* consider only partition of one dimension, and not  divisible(zheng cu)*/
    //int client_num_per_server, low_server_num;

    //low_server_num = server_amount - (client_y_num % server_amount);
    //client_num_per_server = (client_y_num / server_amount) * client_x_num;
    //
    //if(client_num_per_server * low_server_num > client_id) 
    //{
    //    /**
    //     *low servers
    //     **/
    //    *client_index = client_id % client_num_per_server;
    //}else
    //{
    //    client_id -= client_num_per_server * low_server_num;
    //    client_num_per_server += client_x_num;
    //    *client_index = client_id % client_num_per_server;
    //}
    
    /* consider partition of two dimension and only divisible*/
    int client_x_index, client_y_index;
    int client_per_server_x, client_per_server_y;
    int client_x_index_in_server, client_y_index_in_server;
    int client_index;
    
    client_x_index = client_id % client_x_num;
    client_y_index = client_id / client_x_num;

    client_per_server_x = client_x_num / server_x_num;
    client_per_server_y = client_y_num / server_y_num;

    client_x_index_in_server = client_x_index % client_per_server_x;
    client_y_index_in_server = client_y_index % client_per_server_y;

    client_index = client_x_index_in_server + 
	client_y_index_in_server * client_per_server_x;

    debug(DEBUG_MAP, "client index of client(%d) : %d", client_id, client_index);
    return client_index;
}

int iofw_map_forwarding(
	iofw_msg_t *msg)
{
    /* consider only partition of one dimension, and not  divisible(zheng cu)*/
    //int x_proc, y_proc;
    //int low_server_num, client_y_per_server;

    //x_proc = msg->src % client_x_num;
    //y_proc = msg->src / client_x_num;
    //
    //assert(client_y_num >= server_amount);

    //low_server_num = server_amount - (client_y_num % server_amount);
    //client_y_per_server = client_y_num / server_amount;
    //
    //if(client_y_per_server * low_server_num <= y_proc)
    //{
    //    msg->dst = (y_proc - client_y_per_server * low_server_num) / 
    //        (client_y_per_server + 1) + low_server_num;
    //}else
    //{
    //    msg->dst = y_proc / client_y_per_server; 
    //}
    
    /* consider partition of two dimension and only divisible*/
    int client_id, server_id;
    int client_x_index, client_y_index;
    int server_x_index, server_y_index;
    int client_per_server_x, client_per_server_y;
    
    client_id = msg->src;
    client_x_index = client_id % client_x_num;
    client_y_index = client_id / client_x_num;

    client_per_server_x = client_x_num / server_x_num;
    client_per_server_y = client_y_num / server_y_num;

    server_x_index = client_x_index / client_per_server_x;
    server_y_index = client_y_index / client_per_server_y;

    server_id = server_x_index + server_y_index * server_x_num;
    msg->dst = server_id + client_amount;
    
    msg->comm = comm;

    debug(DEBUG_MAP, "client(%d)->server(%d)", msg->src, msg->dst);

    return IOFW_ERROR_NONE;
}

/* no use in this vision */
//int iofw_map_is_bitmap_full(
//	int server_id, uint8_t *bitmap, int *is_full)
//{
//    /* consider only partition of one dimension, and not  divisible(zheng cu)*/
//    int client_num_per_server;
//    int start_client, end_client;
//    int low_server_num;
//    int start_index, end_index, head, tail;
//    int i;
//
//    low_server_num = server_amount - (client_y_num % server_amount);
//    client_num_per_server = (client_y_num / server_amount) * client_x_num;
//    
//    if(server_id < low_server_num) 
//    {
//        /**
//         *low servers
//         **/
//        start_client = client_num_per_server * server_id;
//        end_client = start_client + client_num_per_server - 1;
//    }else
//    {
//        start_client = client_num_per_server * low_server_num;
//        client_num_per_server += client_x_num;
//        start_client += client_num_per_server * (server_id - low_server_num);
//        end_client = start_client + client_num_per_server - 1;
//    }
//
//    start_index = start_client >> 3;
//    head = start_client - (start_index << 3);
//    end_index = end_client >> 3;
//    tail = end_client - (end_index << 3) + 1;
//    
//    debug(DEBUG_MAP, "start : %d; end : %d; head : %d; tail : %d; bitmap : %x", 
//	    start_index, end_index, head, tail, bitmap[0]);
//
//    for(i = start_index + 1; i < end_index; i ++)
//    {
//	if(bitmap[i] != 255)
//	{
//	    *is_full = 0;
//	    return IOFW_ERROR_NONE;
//	}
//    }
//
//    if(start_index == end_index)
//    {
//	if((bitmap[start_index] >> head) != (1 << (tail - head)) - 1)
//	{
//	    *is_full = 0;
//	    return IOFW_ERROR_NONE;
//	}
//    }else
//    {
//    if((bitmap[start_index] >> head) != ((1 << (8 - head)) - 1))
//    {
//	*is_full = 0;
//	return IOFW_ERROR_NONE;
//    }
//    if(bitmap[end_index] != ((1 << tail) - 1))
//    {
//	*is_full = 0;
//	return IOFW_ERROR_NONE;
//    }
//    }
//
//    *is_full = 1;
//    debug(DEBUG_MAP, "success return, bitmap is full."); 
//    return IOFW_ERROR_NONE;
//}

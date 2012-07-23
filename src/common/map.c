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

static int client_proc_num;
static int server_group_num;
static int *server_group_size;
static MPI_Comm *comm;

int iofw_map_init(
	int _client_proc_num, int _server_group_num,
	int *_server_group_size, MPI_Comm *_comm)
{
    assert(_server_group_size != NULL);
    assert(_comm != NULL);

    int i;

    client_proc_num = _client_proc_num;
    server_group_num = _server_group_num;
    server_group_size = malloc(server_group_num * sizeof(int));
    if(NULL == server_group_size)
    {
	debug(DEBUG_MAP, "malloc fail.");
	return IOFW_MAP_ERROR_MALLOC_FAIL;
    }
    comm = malloc(server_group_num * sizeof(int));
    if(NULL == comm)
    {
	debug(DEBUG_MAP, "malloc fail.");
	return IOFW_MAP_ERROR_MALLOC_FAIL;
    }

    for(i = 0; i < server_group_num; i ++)
    {
	server_group_size[i] = _server_group_size[i];
        comm[i] = _comm[i];
    }
    
    debug(DEBUG_MAP, "success return.");
    return IOFW_MAP_ERROR_NONE;
}

int iofw_map_forwarding(
	iofw_msg_t *msg)
{
    //msg->dst = msg->src * server_group_size[0] / client_proc_num;
    msg->comm = comm[0];
    msg->dst = 0;

    debug(DEBUG_MAP, "success return.");
    return IOFW_MAP_ERROR_NONE;
}

int iofw_map_client_num(
	int rank, int size, int *client_num)
{
    int server_proc_num = size;
    
    *client_num = client_proc_num / server_proc_num;
    if( rank <= client_proc_num % server_proc_num )
    {
	*client_num++;
    }

    debug(DEBUG_MAP, "success return."); 
    return IOFW_MAP_ERROR_NONE;
}

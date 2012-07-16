/****************************************************************************
 *       Filename:  map.h
 *
 *    Description:  map from IO proc to IO Forwarding Proc
 *
 *        Version:  1.0
 *        Created:  12/14/2011 02:21:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _MAP_H
#define _MAP_H
#include "msg.h"

#define IOFW_MAP_ERROR_NONE	     0	/* success return */
#define IOFW_MAP_ERROR_MALLOC_FAIL  -1	/* malloc fail */
/**
 * @brief: iofw map var init, only be called in iofw_init adn iofw_server's main
 *	function
 *
 * @param _client_proc_num: 
 * @param _server_group_num: 
 * @param _server_group_size: 
 * @param _comm: 
 *
 * @return: error code
 */
int iofw_map_init(
	int _client_proc_num, int _server_group_num,
	int *_server_group_size, MPI_Comm *_comm);
/**
 * @brief: map from client proc to server proc, store map information in msg struct 
 *
 * @param msg: pointer to msg struct
 *
 * @return: error code
 */
int iofw_map_forwarding(
	iofw_msg_t *msg);
/**
 * @brief iofw_map_client_num 
 *
 * @param rank: the rank of the server
 * @param size : the size of group which the server is belong to 
 * @param clien_num: return of the number of client assigned to this server
 *
 * @return : error code
 */
int iofw_map_client_num(
	int rank, int size, int *clien_num);

#endif

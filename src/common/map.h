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
 * @param _client_num: 
 * @param _server_group_num: 
 * @param _server_group_size: 
 * @param _comm: 
 *
 * @return: error code
 */
/**
 * @brief: iofw map var init, only be called in iofw_init adn iofw_server's main
 *	function
 *
 * @param _client_x_num: client proc num of x axis 
 * @param _client_y_num: client proc num of y axis
 * @param _server_amount: server proc num
 * @param _comm: 
 *
 * @return: error cod
 */
int iofw_map_init(
	int _client_x_num, int _client_y_num,
	int _server_amount, MPI_Comm _comm);
/**
 * @brief: iofw map finalize
 *
 * @return: 
 */
int iofw_map_final();
/**
 * @brief: get server proc amount
 *
 * @param _server_amount: 
 *
 * @return: error code
 */
int iofw_map_get_server_amount(int *_server_amount);
int iofw_map_get_client_amount();
/**
 * @brief: get client number of a server
 *
 * @param server_id: the server's id
 * @param client_num: client number of the server
 *
 * @return: error code
 */
int iofw_map_get_client_num_of_server(int server_id, int *client_num);
/**
 * @brief: get client index in a server
 *
 * @param client_id: the client's id
 * @param client_index: the client index in it's server
 *
 * @return: error code
 */
int iofw_map_get_client_index_of_server(int client_id, int *client_index);
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
 * @brief: check whether a server's bitmap is full
 *
 * @param server_id: server's id
 * @param bitmap: server's' bitmap
 * @param is_full: 
 *
 * @return: error code
 */
int iofw_map_is_bitmap_full(
	int server_id, uint8_t *bitmap, int *is_full);

#endif

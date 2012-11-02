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

#define IOFW_MAP_TYPE_SERVER	1
#define IOFW_MAP_TYPE_CLIENT	2
#define IOFW_MAP_TYPE_BLANK	3 /* the proc who do nothing, because someon may 
				     start more proc than needed*/
/**
 * @brief: iofw map var init, only be called in iofw_init adn iofw_server's main
 *	function
 *
 * @param _client_x_num: client proc num of x axis 
 * @param _client_y_num: client proc num of y axis
 * @param _server_amount: server proc num
 * @param best_server_amount: best server proc num, = client_amount * SERVER_RATIO
 * @param _comm: 
 *
 * @return: error cod
 */
int iofw_map_init(
	int _client_x_num, int _client_y_num,
	int _server_amount, int best_server_amount,
	MPI_Comm _comm);
/**
 * @brief: iofw map finalize
 *
 * @return: 
 */
int iofw_map_final();
/**
 * @brief: determine a proc's type 
 *
 * @param proc_id: proc id
 *
 * @return: IOFW_MAP_TYPE_SERVER, IOFW_MAP_TYPE_CLIENT or IOFW_MAP_TYPE_BLANK
 */
int iofw_map_proc_type(int porc_id);
/**
 * @brief: get MPI communication
 *
 * @return: MPI Communication
 */
int iofw_map_get_comm();
/**
 * @brief: get server proc amount
 *
 * @return: server amount
 */
int iofw_map_get_server_amount();
/**
 * @brief: get client amount
 *
 * @return: client amount
 */
int iofw_map_get_client_amount();
/**
 * @brief: get client number of a server
 *
 * @param server_id: the server's id
 *
 * @return: client num
 */
int iofw_map_get_client_num_of_server(int server_id);
/**
 * @brief: get client index in a server
 *
 * @param client_id: the client's id
 *
 * @return: client_index
 */
int iofw_map_get_client_index_of_server(int client_id);
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
//int iofw_map_is_bitmap_full(
//	int server_id, uint8_t *bitmap, int *is_full);

#endif

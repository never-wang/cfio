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
/**
 * @brief: iofw map variable init, only be called in iofw_init and 
 *	iofw_server's main function
 *
 * @param init_app_proc_num: init value of app_proc_num
 * @param init_server_proc_num: init value of server_proc_num
 *
 * @return: 0
 */
int iofw_map_init(int init_app_proc_num, int init_server_proc_num);
/**
 * @brief: map from IO proc to IO Forwarding Proc
 *
 * @param io_proc_id: the id of IO Proc
 * @param forwarding_proc_id: the id of Forwarding Proc
 *
 * @return: 0 if sucess
 */
int iofw_map_forwarding_proc(
	int io_proc_id, int *forwarding_proc_id);
/**
 * @brief iofw_map_client_num 
 *
 * @param rank: the rank of the server
 * @param clien_num: return of the number of client assigned to this server
 *
 * @return : < 0 for error, 0 for succss
 */
int iofw_map_client_num(
	int rank, int *clien_num);

#endif

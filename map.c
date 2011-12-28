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
#include "map.h"
extern int app_proc_num;
extern int server_proc_num;
/**
 * @brief: map from IO proc to IO Forwarding Proc
 *
 * @param io_proc_id: the id of IO Proc
 * @param forwarding_proc_id: the id of Forwarding Proc
 *
 * @return: 0 if sucess
 */
int iofw_map_forwarding_proc(
	int io_proc_id, int *forwarding_proc_id)
{
    *forwarding_proc_id =  app_proc_num + io_proc_id%server_proc_num;

    return 0;
}
/**
 * @brief iofw_map_client_num 
 *
 * @param rank: the runk of the server
 * @param clien_num: return of the number of client assigned to this server
 *
 * @return : < 0 for error, 0 for succss
 */
int iofw_map_client_num(
	int rank, int *client_num)
{
    *client_num = app_proc_num / server_proc_num;
    if( rank <= app_proc_num + app_proc_num%server_proc_num )
    {
	*client_num++;
    }
    return 0;
}

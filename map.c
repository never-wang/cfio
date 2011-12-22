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

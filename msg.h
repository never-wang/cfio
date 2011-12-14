/****************************************************************************
 *       Filename:  msg.h
 *
 *    Description:  define for msg between IO process and IO forwarding process
 *
 *        Version:  1.0
 *        Created:  12/13/2011 10:39:50 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _MSG_H
#define _MSG_H
#include <stdlib>

#include "mpi.h"

#define FUNC_NC_CREATE 1

int iofw_send_msg(int dst_proc_id, iofw_msg_t *msg);
int iofw_recv_msg(int src_proc_id, iofw_msg_t *msg);

int iofw_pack_msg_nc_create(
	const char *path, int cmode, int *ncidp);

#endif

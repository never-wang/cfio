/****************************************************************************
 *       Filename:  msg.c
 *
 *    Description:  implement for msg between IO process adn IO forwarding process
 *
 *        Version:  1.0
 *        Created:  12/13/2011 03:11:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include "msg.h"

int iofw_send_msg(int dst_proc_id, iofw_buf *buf)
{
    int tag = 1;

    MPI_Send(buf->head, buf->processed, MPI_BYTE, dst_proc_id, tag, MPI_COMM_WORLD);
 
    return 0;
}

int iofw_recv_int1(int src_proc_id, int *data)
{
    int tag = 1;
    MPI_Status status;

    MPI_Recv(data, 1, MPI_INT, src_proc_id, tag, MPI_COMM_WORLD, &status);

    return 0;
}

int iofw_pack_msg_nc_create(
	iofw_buf *buf,
	const char *path, int cmode)
{
    pack32(FUNC_NC_CREATE, buf);
    packstr(path, buf);
    pack32(cmode, buf);
}

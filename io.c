/****************************************************************************
 *       Filename:  io.c
 *
 *    Description:  IO interface
 *
 *        Version:  1.0
 *        Created:  12/13/2011 04:25:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>

#include "io.h"
#include "pack.h"

int iofw_nc_create(
	const int io_proc_id,
	const char *path, int cmode, int *ncidp)
{
    assert(path == NULL);
    assert(ncidp == NULL);

    iofw_buf *buf;
    int dst_proc_id;

    buf = init_buf(BUF_SIZE);
    iofw_pack_msg_nc_create(path, cmode);
    iofw_map_forwading_proc(src_proc_id, &dst_proc_id);
    iofw_send_msg(dst_proc_id, buf);

    iofw_recv_int1(dst_proc_id, ncidp);

    return 0;
}

int iofw_nc_def_dim(
	int ncid, const char *name, size_t len, int *idp)
{
    asser
}




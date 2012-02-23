/****************************************************************************
 *       Filename:  spawn_async.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/22/2012 02:58:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>

#include "mpi.h"
#include "debug.h"
#include "times.h"

int main(int argc, char** argv)
{
    MPI_Comm inter_comm;
    MPI_Status status;
    int size, rank;
    int start_size, end_size, step, loop, send_proc;
    int i, j;
    char *buf;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //set_debug_mask(DEBUG_USER);
    debug(DEBUG_USER, "size : %d, rank : %d", size, rank);

    MPI_Comm_get_parent(&inter_comm);

    if(argc != 6)
    {
	error("wrong argc");
	return -1;
    }
    
    start_size = atoi(argv[1]);
    end_size = atoi(argv[2]);
    step = atoi(argv[3]);
    loop = atoi(argv[4]);
    send_proc = atoi(argv[5]);

    for(i = start_size; i <= end_size; i *= step)
    {
	buf = malloc(i);
	for(j = 0; j < loop; j ++)
	{
	    MPI_Recv(buf, i, MPI_BYTE, send_proc, 0, inter_comm, &status);
	}
	debug(DEBUG_USER, "recv buf[0] = %d", buf[0]);
	free(buf);
    }

    MPI_Finalize();
    return 0;
}

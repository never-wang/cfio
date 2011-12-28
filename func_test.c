/****************************************************************************
 *       Filename:  func_test.c
 *
 *    Description:  test io function
 *
 *        Version:  1.0
 *        Created:  12/14/2011 11:20:04 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdio.h>

#include "mpi.h"
#include "io.h"
#include "gossip.h"
#include "debug.h"

int main(int argc, char** argv)
{
    int rank, size;
    char *path = "./output/test.nc";
    int ncidp;
    int is_server;

    MPI_Comm comm = MPI_COMM_WORLD;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    gossip_enable_stderr();
    gossip_set_debug_mask(GOSSIP_DEBUG_ON, DEBUG_USER); 

    
    iofw_init(size / 3, &is_server);

    if(!is_server)
    {
		iofw_nc_create(rank, path, 0, &ncidp);
    }

    iofw_finalize();
    MPI_Finalize();
    return 0;
}

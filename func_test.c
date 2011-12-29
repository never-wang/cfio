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
    char *path = "./output/test";
    int ncidp;
	int dim1,var1,i;
    int is_server;

    MPI_Comm comm = MPI_COMM_WORLD;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    gossip_enable_stderr();
    gossip_set_debug_mask(GOSSIP_DEBUG_ON, DEBUG_USER); 

    
    iofw_init(size / 3, &is_server);
	char fileName[100];
	sprintf(fileName,"%s_%d.nc",path,rank);
    if(!is_server)
    {
		iofw_nc_create(rank, fileName, 0, &ncidp);
		iofw_nc_def_dim(rank, ncidp, "time", 10L,&dim1);

		int dimids[1];
		dimids[0] = dim1;
		iofw_nc_def_var(rank, ncidp,"time_v", NC_FLOAT, 1,dimids,&var1);
		iofw_nc_enddef(rank,ncidp);
		size_t start[1],count[1];
		float *fp = malloc(10*sizeof(float));
		start[0] = 0;
		count[0] = 10;

		for( i = 0; i< 10; i++)
		{
			fp[i]=i*1.0;
		}
		iofw_nc_put_vara_float(rank,ncidp,var1, start, count,fp); 

    }

    iofw_finalize();
    MPI_Finalize();
    return 0;
}

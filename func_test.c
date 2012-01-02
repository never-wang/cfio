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
#include "debug.h"


int main(int argc, char** argv)
{
    int rank, size;
    char *path = "./output/test";
    int ncidp;
	int dim1,var1,i;
    int is_server;

	size_t len = 10;
    MPI_Comm comm = MPI_COMM_WORLD;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    set_debug_mask(DEBUG_USER); 

    debug_mark(DEBUG_USER);
    
    iofw_init( size / 3 , &is_server);
	char fileName[100];
	sprintf(fileName,"%s_%d.nc",path,rank);
    if(!is_server)
    {
		int dimids[2];
		iofw_nc_create(rank, fileName, 0, &ncidp);
		iofw_nc_def_dim(rank, ncidp, "lat", 5,&dimids[0]);
		iofw_nc_def_dim(rank, ncidp, "kon", 5,&dimids[1]);

		dimids[0] = dim1;
		iofw_nc_def_var(rank, ncidp,"time_v", NC_FLOAT, 2,dimids,&var1);
		iofw_nc_enddef(rank,ncidp);
		size_t start[2],count[2];
		float *fp = malloc(25*sizeof(float));
		start[0] = 0;
		start[1] = 0;
		count[0] = 5;
		count[1] = 5;

		for( i = 0; i< 25; i++)
		{
			fp[i]=1.0;
		}

		iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 

		iofw_nc_close(rank,ncidp);
		free(fp);
	}

    iofw_finalize();
    MPI_Finalize();
    return 0;
}

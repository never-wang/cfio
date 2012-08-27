/****************************************************************************
 *       Filename:  perform_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/26/2012 10:09:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdio.h>
#include <assert.h>

#include "mpi.h"
#include "debug.h"
#include "pnetcdf.h"
#include "times.h"

#define LAT 8192
#define LON 8192

int main(int argc, char** argv)
{
    int rank, size;
    char *path = "./output/test";
    int ncidp;
    int dim1,var1,i;

    int LAT_PROC, LON_PROC;
    MPI_Offset start[2],count[2];

    LAT_PROC = LON_PROC = atoi(argv[1]);

    size_t len = 10;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    times_init();
    times_start();

    assert(size == LAT_PROC * LON_PROC);
    //set_debug_mask(DEBUG_USER | DEBUG_MSG | DEBUG_IOFW | DEBUG_ID); 
    //set_debug_mask(DEBUG_ID); 
    //set_debug_mask(DEBUG_TIME); 
    start[0] = (rank % LAT_PROC) * (LAT / LAT_PROC);
    start[1] = (rank / LON_PROC) * (LON / LON_PROC);
    count[0] = LAT / LAT_PROC;
    count[1] = LON / LON_PROC;
    double *fp = malloc(count[0] * count[1] *sizeof(double));

    for( i = 0; i< count[0] * count[1]; i++)
    {
	fp[i] = i + rank * count[0] * count[1];
    }

    for(i = 0; i < 10; i ++)
    {
	sleep(1);
	times_start();
	char fileName[100];
	sprintf(fileName,"%s/pnetcdf-%d.nc", argv[2], i);
	int dimids[2];
	debug_mark(DEBUG_USER);
	ncmpi_create(MPI_COMM_WORLD, fileName, 0, MPI_INFO_NULL, &ncidp);
	debug_mark(DEBUG_USER);
	int lat = LAT;
	ncmpi_def_dim(ncidp, "lat", lat,&dimids[0]);
	ncmpi_def_dim(ncidp, "lon", LON,&dimids[1]);

	ncmpi_def_var(ncidp,"time_v", NC_DOUBLE, 2,dimids,&var1);
	ncmpi_enddef(ncidp);

	ncmpi_put_vara_double_all(ncidp,var1, start, count,fp);
	ncmpi_close(ncidp);
	printf("proc %d, loop %d time : %f\n", rank, i, times_end());
    }
    //iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 
    //iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 

    free(fp);
    MPI_Finalize();
    printf("proc %d total time : %f\n", rank, times_end());
    times_final();
    return 0;
}


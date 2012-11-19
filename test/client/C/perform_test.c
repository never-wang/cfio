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
#include "iofw.h"
#include "debug.h"
#include "times.h"
#include "test_def.h"

int main(int argc, char** argv)
{
    int rank, size;
    int ncidp;
    int dim1,var1,i, j;

    int LAT_PROC, LON_PROC;
    size_t start[2],count[2];
    char fileName[100];
    char var_name[16];
    int var[VALN];

    size_t len = 10;
    MPI_Comm comm = MPI_COMM_WORLD;

    if(3 != argc)
    {
	printf("Usage : perform_test LAT_PROC output_dir\n");
	return -1;
    }
    
    LAT_PROC = LON_PROC = atoi(argv[1]);
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    times_init();
    times_start();
    times_start();

    //assert(size == LAT_PROC * LON_PROC);
    //set_debug_mask(DEBUG_SERVER | DEBUG_MSG); 
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

    iofw_init( LAT_PROC, LON_PROC, IOFW_RATIO);
    IOFW_START(rank);
    for(i = 0; i < LOOP; i ++)
    {
	sleep(SLEEP_TIME);
	times_start();
	sprintf(fileName,"%s/iofw-%d.nc", argv[2], i);
	int dimids[2];
	iofw_create(fileName, 0, &ncidp);
	int lat = LAT;
	iofw_def_dim(ncidp, "lat", LAT,&dimids[0]);
	iofw_def_dim(ncidp, "lon", LON,&dimids[1]);

	for(j = 0; j < VALN; j++)
	{
	    sprintf(var_name, "time_v%d", j);
	    iofw_def_var(ncidp,var_name, NC_DOUBLE, 2,dimids, 
		    start, count, &var[j]);
	}
	iofw_enddef(ncidp);

	for(j = 0; j < VALN; j++)
	{
	    iofw_put_vara_double(ncidp,var[j], 2,start, count,fp);
	}
	//iofw_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 
	//iofw_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 

	iofw_close(ncidp);
	printf("proc %d, loop %d time : %f\n", rank, i, times_end());
    }
    free(fp);

    IOFW_END();
    iofw_finalize();
    printf("proc %d iofw time : %f\n", rank, times_end());
    MPI_Finalize();
    printf("proc %d total time : %f\n", rank, times_end());
    times_final();
    return 0;
}
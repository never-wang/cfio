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
#include <assert.h>

#include "mpi.h"
#include "iofw.h"
#include "debug.h"

#define LAT 6
#define LON 6

#define LAT_PROC 3
#define LON_PROC 3

int main(int argc, char** argv)
{
    int rank, size;
    char *path = "./output/test";
    int ncidp;
    int dim1,var1,i;

    size_t len = 10;
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    assert(size == LAT_PROC * LON_PROC);
    //set_debug_mask(DEBUG_USER | DEBUG_MSG | DEBUG_IOFW | DEBUG_ID); 
    //set_debug_mask(DEBUG_ID); 
    //set_debug_mask(DEBUG_TIME); 
    size_t start[2],count[2];
    start[0] = (rank / LAT_PROC) * (LAT / LAT_PROC);
    start[1] = (rank % LON_PROC) * (LON / LON_PROC);
    count[0] = LAT / LAT_PROC;
    count[1] = LON / LON_PROC;
    float *fp = malloc(count[0] * count[1] *sizeof(float));

    for( i = 0; i< count[0] * count[1]; i++)
    {
	fp[i] = i + rank * count[0] * count[1];
    }


    iofw_init( LAT_PROC, LON_PROC);
    char fileName[100];
    sprintf(fileName,"%s.nc",path);
    int dimids[2];
    debug_mark(DEBUG_USER);
    iofw_nc_create(rank, fileName, 0, &ncidp);
    debug_mark(DEBUG_USER);
    int lat = LAT;
    iofw_nc_def_dim_(&rank, &ncidp, "lat", &lat,&dimids[0]);
    iofw_nc_def_dim(rank, ncidp, "lon", LON,&dimids[1]);

    iofw_nc_def_var(rank, ncidp,"time_v", NC_FLOAT, 2,dimids,&var1);
    iofw_def_var_range(ncidp, var1, 2, start, count);
    iofw_nc_enddef(rank,ncidp);
    iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 
    //iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 
    //iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 

    iofw_nc_close(rank,ncidp);
    free(fp);

    iofw_finalize();
    printf("fuck111111\n");
    MPI_Finalize();
    printf("fuck22222\n");
    return 0;
}

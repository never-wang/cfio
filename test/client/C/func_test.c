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
#include "cfio.h"
#include "debug.h"

#define LAT 10
#define LON 16

#define LAT_PROC 5
#define LON_PROC 8

#define ratio 8

int main(int argc, char** argv)
{
    int rank, size;
    char *path = "./output/test";
    int ncidp;
    int dim1,var1,i;

    size_t len = 10;
    char *test="test";
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    //assert(size == LAT_PROC * LON_PROC);
    //set_debug_mask(DEBUG_CFIO | DEBUG_SERVER | DEBUG_MSG); 
    //set_debug_mask(DEBUG_SERVER | DEBUG_MAP); 
    size_t start[2],count[2];
    start[0] = (rank % LAT_PROC) * (LAT / LAT_PROC);
    start[1] = (rank / LAT_PROC) * (LON / LON_PROC);
    count[0] = LAT / LAT_PROC;
    count[1] = LON / LON_PROC;
    float *fp = malloc(count[0] * count[1] *sizeof(float));

    for( i = 0; i< count[0] * count[1]; i++)
    {
	fp[i] = i + rank * count[0] * count[1];
    }


    cfio_init( LAT_PROC, LON_PROC, ratio);
    CFIO_START(rank);

    char fileName[100];
    memset(fileName, 0, sizeof(fileName));
    sprintf(fileName,"%s.nc",path);
    int dimids[2];
    cfio_create(fileName, 0, &ncidp);
    debug_mark(DEBUG_USER);
    int lat = LAT;
    cfio_def_dim(ncidp, "lat", LAT,&dimids[0]);
    cfio_def_dim(ncidp, "lon", LON,&dimids[1]);
    cfio_def_dim(ncidp, "lat", LAT,&dimids[0]);

    int a = 3;
    cfio_put_att(ncidp, NC_GLOBAL, "test", NC_INT, 1, &a);

    cfio_def_var(ncidp,"time_v", NC_FLOAT, 2, dimids, start, count, &var1);
    cfio_put_att(ncidp, var1, "test", NC_CHAR, strlen(test), test);
    cfio_enddef(ncidp);
    cfio_put_vara_float(ncidp,var1, 2,start, count,fp); 

    cfio_close(ncidp);
    free(fp);

    CFIO_END();
    cfio_finalize();
    printf("Proc %d : fuck111111\n", rank);
    MPI_Finalize();
    printf("Proc %d : fuck22222\n", rank);
    return 0;
}

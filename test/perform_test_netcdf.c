/****************************************************************************
 *       Filename:  perform_test_netcdf.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/26/2012 06:20:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdio.h>
#include <assert.h>

#include "debug.h"
#include "netcdf.h"
#include "times.h"

#define LAT 8192
#define LON 4096

int main(int argc, char** argv)
{
    int rank, size;
    int ncidp;
    int dim1,var1,i;

    size_t start[2],count[2];

    size_t len = 10;

    times_init();
    times_start();

    rank = 0;
    //set_debug_mask(DEBUG_USER | DEBUG_MSG | DEBUG_IOFW | DEBUG_ID); 
    //set_debug_mask(DEBUG_ID); 
    //set_debug_mask(DEBUG_TIME); 
    start[0] = 0;
    start[1] = 0;
    count[0] = LAT;
    count[1] = LON;
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
	sprintf(fileName,"%s/netcdf-%d.nc", argv[1], i);
	int dimids[2];
	debug_mark(DEBUG_USER);
	nc_create(fileName, 0, &ncidp);
	debug_mark(DEBUG_USER);
	int lat = LAT;
	nc_def_dim(ncidp, "lat", lat,&dimids[0]);
	nc_def_dim(ncidp, "lon", LON,&dimids[1]);

	nc_def_var(ncidp,"time_v", NC_DOUBLE, 2,dimids,&var1);
	nc_enddef(ncidp);

	nc_put_vara_double(ncidp,var1, start, count,fp);
	nc_close(ncidp);
	printf("proc %d, loop %d time : %f\n", rank, i, times_end());
    }
    //iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 
    //iofw_nc_put_vara_float(rank,ncidp,var1, 2,start, count,fp); 

    free(fp);
    printf("proc %d total time : %f\n", rank, times_end());
    times_final();
    return 0;
}

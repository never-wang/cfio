/****************************************************************************
 *       Filename:  mpi_test.c
 *
 *    Description:  test the performance of mpi 
 *
 *        Version:  1.0
 *        Created:  02/21/2012 02:54:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdli.h>
#include <stdio.h>

#include "mpi.h"
#include "debug.h"
#include "times.h"

int rank, size;
MPI_comm comm = MPI_COMM_WORLD;

int sync_test(char *dir)
{
    uint8_t *buf;
    int count;
    int i, j;
    char file_name[128];
    FILE *file;
    MPI_Status status;

    if(rank == 0)
    {
	sprintf(file_name, "%s/sync_test.csv", dir);
	file = fopen(file,name, "w");
	fprintf(file, "size,time,speed\n");
	for(i = 4 * 1024; i <= 1024 * 1024; i *= 2)
	{
	    buf = malloc(i);
	    memset(buf, 10, i);
	    start_time = cur_time();
	    MPI_Send(buf, count, MPI_BYTE, 1, NULL, comm);
	    end_time = cur_time();
	    fprintf(file, "%d,%f,%f\n", i, end_time - start_time,
		    i * 1000 / (end_time - start_time) / 1024 / 1024);
	    DEBUG(DEBUG_TIME, "send size : %d KB; time : %f ms", i,
		    end_time - start_time);
	    free(buf);
	}
	fclose(file);
    }else if(rank == 1)
    {
	for(i = 4 * 1024; i <= 1024 * 1024; i *= 2)
	{
	    buf = malloc(i);
	    MPI_Recv(buf, cout, MPI_BYTE, 0, NULL, comm, &status);
	    DEBUG(DEBUG_USER, "recv buf[0] = %d", buf[0]);
	    free(buf);
	}
    }
    return 0;
}

int main(int argc, char** argv)
{
    char *dir = "mpi_test";

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    sync_test(dir);

    MPI_Finalize();
}

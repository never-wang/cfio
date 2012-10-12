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
#include <stdlib.h>
#include <stdio.h>

#include "mpi.h"
#include "debug.h"
#include "times.h"

int loop = 100;

int rank, size;
MPI_Comm comm = MPI_COMM_WORLD;
int start_size = 4096, end_size = 256 * 1024 * 1024;
int step = 2;

int sync_test(char *dir, int send_proc, int recv_proc)
{
    uint8_t *buf;
    int i, j;
    char file_name[128];
    FILE *file;
    MPI_Status status;
    double start_time, end_time;

    if(rank == send_proc)
    {
	sprintf(file_name, "%s/sync_test.csv", dir);
	file = fopen(file_name, "w");
	fprintf(file, "size,time,speed\n");
	for(i = start_size; i <= end_size; i *= step)
	{
	    buf = malloc(i);
	    memset(buf, 10, i);
	    start_time = times_cur();
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Send(buf, i, MPI_BYTE, recv_proc, 0, comm);
	    }
	    end_time = times_cur();
	    fprintf(file, "%d,%f,%f\n", i / 1024 , (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, "send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    free(buf);
	}
	fclose(file);
    }else if(rank == recv_proc)
    {
	for(i = start_size; i <= end_size; i *= step)
	{
	    buf = malloc(i);
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Recv(buf, i, MPI_BYTE, send_proc, 0, comm, &status);
	    }
	    free(buf);
	}
    }
    return 0;
}

int async_test(char *dir, int send_proc, int recv_proc)
{
    uint8_t *buf;
    int i, j;
    char file_name[128];
    FILE *file;
    MPI_Status status, *status_array;
    MPI_Request *send_array;
    double start_time, end_time;

    if(rank == send_proc)
    {
	status_array = malloc(loop * sizeof(MPI_Status));
	send_array = malloc(loop * sizeof(MPI_Request));

	sprintf(file_name, "%s/async_test.csv", dir);
	file = fopen(file_name, "w");
	fprintf(file, "size, time, speed, real time, real speed\n");
	for(i = start_size; i <= end_size; i *= step)
	{
	    buf = malloc(i);
	    memset(buf, 10, i);
	    start_time = times_cur();
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Isend(buf, i, MPI_BYTE, recv_proc, 0, comm, &send_array[j]);
	    }
	    end_time = times_cur();
	    fprintf(file, "%d,%f,%f,", i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, "send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    MPI_Waitall(loop, send_array, status_array);
	    end_time = times_cur();
	    fprintf(file, "%f,%f\n", (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, 
		    "real send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    free(buf);
	}

	free(status_array);
	free(send_array);
	fclose(file);
    }else if(rank == recv_proc)
    {
	for(i = start_size; i <= end_size; i *= step)
	{
	    buf = malloc(i);
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Recv(buf, i, MPI_BYTE, send_proc, 0, comm, &status);
	    }
	    //debug(DEBUG_USER, "recv buf[0] = %d", buf[0]);
	    free(buf);
	}
    }

    return 0;
}

int remote_test(char *dir, int local_proc, int remote_proc)
{
    uint8_t *buf;
    int i, j;
    char file_name[128];
    FILE *file;
    MPI_Win win;
    MPI_Info info;
    double start_time, end_time;

    if(rank == local_proc)
    {
	sprintf(file_name, "%s/remote_test.csv", dir);
	file = fopen(file_name, "w");
	fprintf(file, "size, time, speed, real time, real speed\n");
    }

    for(i = start_size; i <= end_size; i *= step)
    {
	buf = malloc(i);
	MPI_Win_create(buf, i, sizeof(uint8_t), info, MPI_COMM_WORLD, &win);

	MPI_Win_fence(0, win);
	if(rank == local_proc)
	{
	    memset(buf, 10, i);
	    
	    start_time = times_cur();
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Put(buf, i, MPI_BYTE, remote_proc, 0, i, MPI_BYTE, win);
	    }
	    end_time = times_cur();
	    fprintf(file, "%d,%f,%f,", i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, "send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	}
	MPI_Win_fence(0, win);
	if(rank == local_proc)
	{
	    end_time = times_cur();
	    fprintf(file, "%f,%f\n", (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, 
		    "real send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	}
	if(rank == remote_proc)
	{
	    debug(DEBUG_USER, "recv buf[0] = %d", buf[0]);
	    
	}
	MPI_Win_free(&win);
	free(buf);
    }
    if(rank == local_proc)
    {
	fclose(file);
    }

    return 0;
}

int spawn_sync_test(char *dir, int root_proc)
{
    MPI_Comm inter_comm;
    int error;
    uint8_t *buf;
    int i, j;
    char file_name[128];
    FILE *file;
    double start_time, end_time;

    int ret;
    char error_str[64];
    int len;

    char** argv;

    if(rank == root_proc)
    {
	sprintf(file_name, "%s/spawn_sync_test.csv", dir);
	file = fopen(file_name, "w");
	fprintf(file, "size, time, speed\n");

	argv = malloc(6 * sizeof(char*));
	for(i  = 0; i < 5; i ++)
	{
	    argv[i] = malloc(128);
	}
	argv[5] = NULL;
	sprintf(argv[0], "%d", start_size);
	sprintf(argv[1], "%d", end_size);
	sprintf(argv[2], "%d", step);
	sprintf(argv[3], "%d", loop);
	sprintf(argv[4], "%d", root_proc);

	debug(DEBUG_USER, "%s %s %s %s %s", 
		argv[0], argv[1], argv[2], argv[3], argv[4]);

	ret = MPI_Comm_spawn("./mpi_test_spawn", argv, 1, MPI_INFO_NULL, 
		root_proc, MPI_COMM_SELF, &inter_comm, &error);

	debug_mark(DEBUG_USER);

	if(ret != MPI_SUCCESS)
	{
	    MPI_Error_string(ret, error_str, &len);
	    error("%s", error_str);
	    return -1;
	}

	debug_mark(DEBUG_USER);
	for(i = start_size; i <= end_size; i *= step)
	{
	    buf = malloc(i);
	    memset(buf, 10, i);
	    start_time = times_cur();
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Send(buf, i, MPI_BYTE, 0, 0, inter_comm);
	    }
	    end_time = times_cur();
	    fprintf(file, "%d,%f,%f\n", i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, "send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    free(buf);
	}
	fclose(file);

	for(i = 0; i < 5; i ++)
	{
	    free(argv[i]);
	}
	free(argv);
    }

    return 0;
}

int spawn_async_test(char *dir, int root_proc)
{
    MPI_Comm inter_comm;
    int error;
    uint8_t *buf;
    int i, j;
    char file_name[128];
    FILE *file;
    MPI_Status status, *status_array;
    MPI_Request *send_array;
    double start_time, end_time;

    int ret;
    char error_str[64];
    int len;

    char** argv;

    if(rank == root_proc)
    {
	status_array = malloc(loop * sizeof(MPI_Status));
	send_array = malloc(loop * sizeof(MPI_Request));

	sprintf(file_name, "%s/spawn_async_test.csv", dir);
	file = fopen(file_name, "w");
	fprintf(file, "size, time, speed, real time, real speed\n");

	argv = malloc(6 * sizeof(char*));
	for(i  = 0; i < 5; i ++)
	{
	    argv[i] = malloc(128);
	}
	argv[5] = NULL;
	sprintf(argv[0], "%d", start_size);
	sprintf(argv[1], "%d", end_size);
	sprintf(argv[2], "%d", step);
	sprintf(argv[3], "%d", loop);
	sprintf(argv[4], "%d", root_proc);

	debug(DEBUG_USER, "%s %s %s %s %s", 
		argv[0], argv[1], argv[2], argv[3], argv[4]);

	ret = MPI_Comm_spawn("./mpi_test_spawn", argv, 1, MPI_INFO_NULL, 
		root_proc, MPI_COMM_SELF, &inter_comm, &error);

	debug_mark(DEBUG_USER);

	if(ret != MPI_SUCCESS)
	{
	    MPI_Error_string(ret, error_str, &len);
	    error("%s", error_str);
	    return -1;
	}

	debug_mark(DEBUG_USER);
	for(i = start_size; i <= end_size; i *= step)
	{
	    buf = malloc(i);
	    memset(buf, 10, i);
	    start_time = times_cur();
	    for(j = 0; j < loop; j ++)
	    {
		MPI_Isend(buf, i, MPI_BYTE, 0, 0, inter_comm, 
			&send_array[j]);
	    }
	    end_time = times_cur();
	    fprintf(file, "%d,%f,%f,", i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, "send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    MPI_Waitall(loop, send_array, status_array);
	    end_time = times_cur();
	    fprintf(file, "%f,%f\n", (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    debug(DEBUG_TIME, 
		    "real send size : %d KB; time : %f ms; speed : %f MB/s", 
		    i / 1024, (end_time - start_time) / loop,
		    i / (end_time - start_time) / 1024 / 1024 * loop * 1000);
	    free(buf);
	}
	free(status_array);
	free(send_array);
	fclose(file);

	for(i = 0; i < 5; i ++)
	{
	    free(argv[i]);
	}
	free(argv);
    }

    return 0;
}

int main(int argc, char** argv)
{
    char *dir = "mpi_test_output";
    int send_proc, recv_proc, root_proc;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if(argc != 4)
    {
        error("wrong argc");
    }

    send_proc = atoi(argv[1]);
    recv_proc = atoi(argv[2]);
    root_proc = atoi(argv[3]);

    //set_debug_mask(DEBUG_TIME);
    set_debug_mask(DEBUG_TIME | DEBUG_USER);
    if(rank == 0)
    {
	mkdir(dir, 0700);
    }

    //sync_test(dir, send_proc, recv_proc);
    //async_test(dir, send_proc ,recv_proc);
    //remote_test(dir, send_proc, recv_proc);
    spawn_sync_test(dir, root_proc);
    spawn_async_test(dir, root_proc);

    MPI_Finalize();
}

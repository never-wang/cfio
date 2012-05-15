/****************************************************************************
 *       Filename:  test.c
 *
 *    Description:  performance test for origin netcdf
 *
 *        Version:  1.0
 *        Created:  12/29/2011 11:08:37 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <time.h>
#include <sys/time.h>
#include <error.h>

#include "netcdf.h"
#include "debug.h"
#include "times.h"

#include "mpi.h"

#define ENABLE_DEBUG

static const int lon_points = 360;
static const int lat_points = 180;
static const int nl_soil = 10;
static int ntime = 1;
static const int nfldv = 92;

static const int nflxvar = 65;
static int n2dflxvar = 62;
char flxname[nflxvar][32];
int flxdims[nflxvar];
int flxvid[nflxvar];
int flxmap[nfldv];
int flxbeg[nfldv];

double longxy[lat_points][lon_points];
double latixy[lat_points][lon_points];
double soilz[nl_soil] = {0.0071006, 0.0279250, 0.062258, 0.118865, 0.212193,
                         0.3660658, 0.6197585, 1.038027, 1.727635, 2.864607};
double timeval[1] = {0};
double lon[lon_points];
double lat[lat_points];

int rank, size;
MPI_Comm comm = MPI_COMM_WORLD;

double start_time, end_time;

char *file_name="perform_test_origin.csv";
FILE* file;

int nc_hist_create(const char *path, int *nc_id, int *idate)
{
    int dimids1[1];
    int dimids2[2];
    int dimids3[3];
    int dimids4[4];

    int i, j;
    
    int ncid;
    int dim_lon, dim_lat, dim_soilz, dim_time;
    int vid_year, vid_day, vid_second;
    int vid_lon, vid_lat, vid_soilz, vid_time, vid_longxy, vid_latixy;

    times_start();
    nc_create(path, NC_CLOBBER, &ncid);
    debug(DEBUG_TIME, "Porc %03d : nc_create time : %f", times_end());

    times_start();
    nc_def_dim(ncid, "lon", lon_points, &dim_lon);
    nc_def_dim(ncid, "lat", lat_points, &dim_lat);
    nc_def_dim(ncid, "soilz", nl_soil, &dim_soilz);
    nc_def_dim(ncid, "time", ntime, &dim_time);
    debug(DEBUG_TIME, "Porc %03d : nc_def_dim time : %f", times_end());

    times_start();
    nc_def_var(ncid, "origin_year", NC_INT, 0, NULL, &vid_year);
    nc_def_var(ncid, "origin_day", NC_INT, 0, NULL, &vid_day);
    nc_def_var(ncid, "origin_second", NC_INT, 0, NULL, &vid_second);
    debug(DEBUG_TIME, "Porc %03d : nc_def_dim time : %f", times_end());

    times_start();
    dimids1[0] = dim_lon;
    nc_def_var(ncid, "lon", NC_DOUBLE, 1, dimids1, &vid_lon);
    dimids1[0] = dim_lat;
    nc_def_var(ncid, "lat", NC_DOUBLE, 1, dimids1, &vid_lat);
    dimids1[0] = dim_soilz;
    nc_def_var(ncid, "soilz", NC_DOUBLE, 1, dimids1, &vid_soilz);
    dimids1[0] = dim_time;
    nc_def_var(ncid, "time", NC_DOUBLE, 1, dimids1, &vid_time);
    dimids2[0] = dim_lon;
    dimids2[1] = dim_lat;
    nc_def_var(ncid, "longxy", NC_DOUBLE, 2, dimids2, &vid_longxy);
    nc_def_var(ncid, "latixy", NC_DOUBLE, 2, dimids2, &vid_latixy);
    
    dimids3[0] = dim_lon;
    dimids3[1] = dim_lat;
    dimids3[2] = dim_time;
    dimids4[0] = dim_lon;
    dimids4[1] = dim_lat;
    dimids4[2] = dim_soilz;
    dimids4[3] = dim_time;
    for(i = 0; i < nflxvar; i++)
    {
	if(2 == flxdims[i])
	{
	    nc_def_var(ncid, flxname[i], NC_FLOAT, 3 ,dimids3, &flxvid[i]);
	}else if(3 == flxdims[i])
	{
	    nc_def_var(ncid, flxname[i], NC_FLOAT, 4, dimids4, &flxvid[i]);
	}
    }
    debug(DEBUG_TIME, "Porc %03d : nc_def_var time : %f", times_end());

    /**
     *Just no attr TODO
     **/
    nc_enddef(ncid);
    
    //nc_put_var(ncid, vid_lon, lon);
    //nc_put_var(ncid, vid_lat, lat);
    //nc_put_var(ncid, vid_time, timeval);
    //nc_put_var(ncid, vid_soilz, soilz);
    //nc_put_var(ncid, vid_year, &idate[0]);
    //nc_put_var(ncid, vid_day, &idate[1]);
    //nc_put_var(ncid, vid_second, &idate[2]);
    //nc_put_var(ncid, vid_longxy, longxy);
    //nc_put_var(ncid, vid_latixy, latixy);

    *nc_id = ncid;
    return 0;
    
}

int nc_hist_add_field(int loop, int nc_id, float *data)
{
    int field_id, ndim, zlev;
    size_t start_3d[3], start_4d[4];
    size_t count_3d[3], count_4d[4];

    field_id = flxmap[loop];
    ndim = flxdims[field_id];
    debug(DEBUG_USER, "var loop : %d", loop);
    debug(DEBUG_USER, "field_id = %d; ndim = %d", field_id, ndim);
    if(2 == ndim)
    {
	start_3d[0] = 0;
	start_3d[1] = 0;
	start_3d[2] = 0;
	count_3d[0] = lon_points;
	count_3d[1] = lat_points;
	count_3d[2] = 1;
	int status = nc_put_vara_float(nc_id, flxvid[field_id], start_3d, count_3d, data);
	debug(DEBUG_USER, "%s", nc_strerror(status));
    }else if(3 == ndim)
    {
	zlev = loop - flxbeg[field_id];
	start_4d[0] = 0;
	start_4d[1] = 0;
	start_4d[2] = zlev;
	start_4d[3] = 0;
	count_4d[0] = lon_points;
	count_4d[1] = lat_points;
	count_4d[2] = 1;
	count_4d[3] = 1;
	int status = nc_put_vara_float(nc_id, flxvid[field_id], start_4d, count_4d, data);
	debug(DEBUG_USER, "%s", nc_strerror(status));
    }

    return 0;
}

int nc_hist_close(int ncid)
{
    nc_close(ncid);
    return 0;
}

int write_hist_data(int *idate, char* prefix)
{
    char path[128];
    float fldxy[lon_points][lat_points];
    int nc_id, i, j, l;
    double longxy_val;
    double latixy_val;
    
    debug_mark(DEBUG_USER);
    /**
     *gen data
     **/
    for(i = 0; i < n2dflxvar; i ++)
    {
	sprintf(flxname[i], "flxvar%02d", i);
	flxdims[i] = 2;
    }
    for(i = n2dflxvar; i < nflxvar; i++)
    {
	sprintf(flxname[i], "flxvar%02d", i);
	flxdims[i] = 3;
    }
    latixy_val = 89.5;
    for(i = 0; i < lat_points; i ++)
    {
	longxy_val = -179.5;
	for(j = 0; j < lon_points; j ++)
	{
	    longxy[i][j] = longxy_val;
	    longxy_val += 1;
	    latixy[i][j] = latixy_val;
	}
	latixy_val -= 1;
    }
    for(i = 0; i < lon_points; i ++)
    {
	lon[i] = longxy[0][i];
    }
    for(i = 0; i < lat_points; i ++)
    {
	lat[i] = latixy[i][0];
    }
    j = 0;
    for(i = 0; i < nfldv; i ++)
    {
	flxbeg[i] = j;
	if(2 == flxdims[i])
	{
	    debug(DEBUG_USER, "map(%d->%d)", j, i);
	    flxmap[j] = i;
	    j ++;
	}else if(3 == flxdims[i])
	{
	    for(l = 0; l < nl_soil; l ++)
	    {
		debug(DEBUG_USER, "map(%d->%d)", j+l, i);
		flxmap[j + l] = i;
	    }
	    j += nl_soil;
	}
    }

    srand(time(NULL));
    for(i = 0; i < nfldv; i ++)
    {
	for(j = 0; j < lon_points; j ++)
	{
	    for(l = 0; l < lat_points; l ++)
	    {
		fldxy[j][l] = ((float)(rand() % 10000)) / 100.0;
	    }
	}
    }
    float *data = fldxy;
    debug(DEBUG_USER, "data[0] = %f; data[400] = %f", data[0], data[400]);
    debug(DEBUG_USER, "fldxy[0][0] = %f; fldxy[2][40] = %f", fldxy[0][0],
	    fldxy[2][40]);

    debug(DEBUG_USER, "(%04d, %02d, %02d)", idate[0], idate[1], idate[2]);
    sprintf(path, "%s/hist-%04d-%02d-%03d.nc", prefix, idate[0], idate[1], rank);

    debug(DEBUG_USER, "file : %s", path);

    start_time = times_cur();
    nc_hist_create(path, &nc_id, idate);
    end_time = times_cur();
    
    debug(DEBUG_TIME, "Porc %03d : create file time : %f", 
	    rank, end_time - start_time);
	if(0 == rank)
	{
		fprintf(file, "%f, ", end_time - start_time);
	}

    start_time = times_cur();
    for(i = 0; i < nfldv; i ++)
    {
	nc_hist_add_field(i, nc_id, fldxy);
    }
    end_time = times_cur();
    
    debug(DEBUG_TIME, "Porc %03d : write file time : %f", 
	    rank, end_time - start_time);
	if(0 == rank)
	{
		fprintf(file, "%f, ", end_time - start_time);
	}

    start_time = times_cur();
    nc_hist_close(nc_id);
    end_time = times_cur();
    
    debug(DEBUG_TIME, "Porc %03d : close file time : %f", 
	    rank, end_time - start_time);
	
	if(0 == rank)
	{
		fprintf(file, "%f\n", end_time - start_time);
	}

    return 0;
}

int main(int argc, char** argv)
{
    int idate[3] = {1990, 1, 1};
    int i;
    int cycle = 5;
    char *prefix = "output/";

    MPI_Init(&argc, &argv);
    
    times_init();

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    //set_debug_mask(DEBUG_TIME | DEBUG_USER);
    set_debug_mask(DEBUG_TIME);

    file = fopen(file_name, "w");
    fprintf(file, "loop, create, write, close\n");

    times_start();

    for(i = 0; i < cycle; i ++)
    {
	if(0 == rank)
	{
	    debug(DEBUG_TIME, "loop %d :", i);
	    fprintf(file, "%d, ", i);
	}
	start_time = times_cur();
	sleep(1);
	end_time = times_cur();
	debug(DEBUG_TIME, "Porc %03d : sleep time : %f", 
		rank, end_time - start_time);
	write_hist_data(idate, prefix);
	//MPI_Barrier(comm);
	idate[1] ++;
    }

    fclose(file);


    MPI_Finalize();

    debug(DEBUG_TIME, "total time : %f ms", times_end());
    times_final();

    return 0;
}

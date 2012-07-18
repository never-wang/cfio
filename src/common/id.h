/****************************************************************************
 *       Filename:  id.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/27/2012 03:15:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _ID_H
#define _ID_H

#include <stdint.h>

#include "quicklist.h"

#define MAX_OPEN_NC_NUM 1024
#define MAP_HASH_TABLE_SIZE 1024

#define IOFW_ID_INIT_CLIENT 0
#define IOFW_ID_INIT_SERVER 1

#define IOFW_ID_ERROR_NONE	     0
#define IOFW_ID_ERROR_TOO_MANY_OPEN -1	/* the opened nc file amount exceed 
					   MAX_OPEN_NC_NUM */
#define IOFW_ID_ERROR_GET_NULL	    -2	/* can't find the client id in Map Hash 
					   Table*/
#define IOFW_ID_ERROR_WRONG_FLAG    -3  /* wrong flag in iofw_id_init */
#define IOFW_ID_ERROR_EXCEED_BOUND  -4	/* data index exceeds dimension bound */
#define IOFW_ID_ERROR_MALLOC	    -5	/* malloc fail */

#define DEFINE_MODE 0
#define DATA_MODE   1

/** @brief: store opened nc file 's information in client */
typedef struct 
{
    uint8_t used;  /* indicate if the id has been used by an opened nc file */
    int var_a;     /* amount of defined var */
    int dim_a;     /* amount of defined dim */
    int id;        /* id assigned to the nc , should be index + 1*/
}iofw_nc_t;

/** @brief: store a nc file information in server */
typedef struct 
{
    int nc_id;		    /* id of nc file */
    int recv_client_num;    /* number fo client whose nc file request has been
			       received by server */
    int nc_status;	    /* the status of nc file : DEFINE_MODE or DATA_MODE */
}iofw_id_nc_t;

/** @brief: store a dimension information in server */
typedef struct
{
    int nc_id;		    /* id of nc file which the dimension belong to */
    int dim_id;		    /* id of the dimension */
    int dim_len;	    /* length of the dim */
    int recv_client_num;    /* number fo client whose dimension request has been
			       received by server */
}iofw_id_dim_t;
/** @brief: store a variable information in server */
typedef struct
{
    int nc_id;		    /* id of nc file which the variable is belong to */
    int var_id;		    /* id of var */
    int recv_client_num;    /* number of client whose variable I/O request
			       has been received by server */
    int ndims;		    /* number of dimensions for the variable */
    int *dims_len;	    /* vector of ndims dimension length for the variable */
    size_t ele_size;	    /* size of each element in the variable array */
    char *data;		    /* data array for the variable */

}iofw_id_var_t;

/** @brief: id hash entry key */
typedef struct
{
    int client_nc_id;	/* id of nc file in client */
    int client_dim_id;	/* id of nc dim in client */
    int client_var_id;	/* id of nc var in client */
}iofw_id_key_t;

/** @brief: id hash entry key */
typedef struct
{
    int client_nc_id;	/* id of nc file in client */
    int client_dim_id;	/* id of nc dim in client */
    int client_var_id;	/* id of nc var in client */
    iofw_id_nc_t *nc;   /* nc file infomation in server */
    iofw_id_dim_t *dim; /* dim infomation in server */
    iofw_id_var_t *var; /* nc var infomation in server */
    qlist_head_t hash_link;	
}iofw_id_val_t;

/**
 * @brief: init 
 *
 * @param flag: indicate whether server or client calls the init function
 *
 * @return: error code
 */
int iofw_id_init(int flag);
/**
 * @brief: finalize
 *
 * @return: error code
 */
int iofw_id_final();
/**
 * @brief: assign a nc id in client
 *
 * @param nc_id: the assigned nc id
 *
 * @return: error code
 */
int iofw_id_assign_nc(int *nc_id);
/**
 * @brief: assign a nc dim id in client
 *
 * @param ncid: the nc file id
 * @param dimid: the assigned dim id
 *
 * @return: error code
 */
int iofw_id_assign_dim(int ncid, int *dimid);
/**
 * @brief: assign a nc var id in client
 *
 * @param ncid: the nc file id
 * @param varid: the assigne var id
 *
 * @return: error code
 */
int iofw_id_assign_var(int ncid, int *varid);
/**
 * @brief: add a new map(client_ncid->server_ncid) in server
 *
 * @param client_ncid: the nc file id in client
 * @param server_ncid: the nc file id in server
 *
 * @return: error code
 */
int iofw_id_map_nc(int client_ncid, int server_ncid);
/**
 * @brief: add a new map((client_ncid, client_dimid)->(server_ncid, 
 *	server_dimid)) in server
 *
 * @param client_ncid: the nc file id in client
 * @param client_dimid: the dim id in client
 * @param server_ncid: the nc file id in server
 * @param server_dimid: the dim id in server
 * @param dim_len: length of dim
 *
 * @return: error code
 */
int iofw_id_map_dim(
	int client_ncid, int client_dimid, 
	int server_ncid, int server_dimid, int dim_len);
/**
 * @brief: add a new map((client_ncid, client_varid)->(server_ncid,
 *	server_dimid)) in server
 *
 * @param client_ncid: the nc file id in client
 * @param client_varid: the var id in client
 * @param server_ncid: the nc file id in server
 * @param client_varid: the var id in server
 * @param ndims: number of dimensions for the variable 
 * @param dims_len: vector of ndims dimension length for the variable 
 * @param ele_size: size of each element in the variable array 
 *
 * @return: error code
 */
int iofw_id_map_var(
	int client_ncid, int client_varid,
	int server_ncid, int server_varid,
	int ndims, int *dims_len, int ele_size)
/**
 * @brief: get server_ncid by client_ncid in server
 *
 * @param client_ncid: the nc file id in client
 * @param nc: pointer to the nc file information in server
 *
 * @return: error code
 */
int iofw_id_get_nc( 
	int client_ncid, iofw_id_nc_t **nc)
/**
 * @brief: get (server_ncid, server_dimid) by (client_dimid) in
 *	server
 *
 * @param client_ncid: the nc file id in client
 * @param client_dimid: the dim id in client
 * @param dim: pointer to the dim information in server
 *
 * @return: error code
 */
int iofw_id_get_dim(
	int client_ncid, int client_dimid, 
	iofw_id_dim_t **dim)
/**
 * @brief: get (server_ncid, server_varid) by (client_varid) in
 *	server
 *
 * @param client_ncid: the nc file id in client
 * @param client_varid: the var id in client
 * @param var: pointer to the variable information in the server
 *
 * @return: error code
 */
int iofw_id_get_var(
	int client_ncid, int client_varid, 
	iofw_id_var_t **var);

/**
 * @brief: put part of variable data in the whole var buf which is stored in the 
 *	hash table
 *
 * @param client_ncid: the nc file id in client
 * @param client_varid: the variable id in client
 * @param start: start index of the variable in the whole variable array
 * @param count: count of the variable
 * @param data: pointer to the date
 *
 * @return: error code
 */
int iofw_id_put_var(
	int client_ncid, int client_varid,
	size_t *start, size_t *count, 
	char *data);

#endif

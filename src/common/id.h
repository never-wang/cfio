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

#define MAX_OPEN_NC_NUM 1024
#define MAP_HASH_TABLE_SIZE 1024

#define IOFW_ID_INIT_CLIENT 0
#define IOFW_ID_INIT_SERVER 1

#define IOFW_ID_ERROR_NONE 0
#define IOFW_ID_ERROR_TOO_MANY_OPEN 1 /* the opened nc file amount exceed MAX_OPEN_NC_NUM */
#define IOFW_ID_ERROR_GET_NULL /* can't find the client id in Map Hash Table*/

typedef struct 
{
    uint8_t used;  /* indicate if the id has been used by an opened nc file */
    int var_a;     /* amount of defined var */
    int dim_a;     /* amount of defined dim */
    int id;        /* id assigned to the nc , should be index + 1*/
}iofw_nc_t;

typedef struct
{
    int client_id;	/* id of client proc */
    int client_nc_id;	/* id of nc file in client */
    int client_var_id;	/* id of nc var in client */
    int client_dim_id;	/* id of nc dim in client */
}iofw_id_key_t;

typedef struct
{
    int client_id;	/* id of client proc */
    int client_nc_id;	/* id of nc file in client */
    int client_var_id;	/* id of nc var in client */
    int client_dim_id;	/* id of nc dim in client */
    int server_nc_id;	/* id of nc file in server */
    int server_var_id;	/* id of nc var in server */
    int server_dim_id;	/* id of nc dim in server */
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
 * @param client_id: the client proc id
 * @param client_ncid: the nc file id in client
 * @param server_ncid: the nc file id in server
 *
 * @return: error code
 */
int iofw_id_map_nc(int client_id, int client_ncid, int server_ncid);
/**
 * @brief: add a new map((client_ncid, client_dimid)->(server_ncid, 
 *	server_dimid)) in server
 *
 * @param client_id: the client proc id
 * @param client_ncid: the nc file id in client
 * @param client_dimid: the dim id in client
 * @param server_ncid: the nc file id in server
 * @param server_dimid: the dim id in server
 *
 * @return: error code
 */
int ifow_id_map_dim(int client_id,
	int client_ncid, int client_dimid, 
	int server_ncid, server_dimid);
/**
 * @brief: add a new map((client_ncid, client_varid)->(server_ncid,
 *	server_dimid)) in server
 *
 * @param client_id: the client proc id
 * @param client_ncid: the nc file id in client
 * @param client_varid: the var id in client
 * @param server_ncid: the nc file id in server
 * @param client_varid: the var id in server
 *
 * @return: 
 */
int iofw_id_map_var(int client_id,
	int client_ncid, int client_varid,
	int server_ncid, int client_varid);
/**
 * @brief: get server_ncid by client_ncid in server
 *
 * @param client_id: the client proc id
 * @param client_ncid: the nc file id in client
 * @param server_ncid: the var file id in server
 *
 * @return: error code
 */
int iofw_id_get_nc(int client_id,
	int client_ncid, int *server_ncid);
/**
 * @brief: get (server_ncid, server_dimid) by (client_ncid, client_dimid) in
 *	server
 *
 * @param client_id: the client proc id
 * @param client_ncid: the nc file id in client
 * @param client_dimid: the dim id in client
 * @param server_ncid: the nc file id in server
 * @param server_dimid: the dim id in server
 *
 * @return: error code
 */
int ifow_id_get_dim(int client_id,
	int client_ncid, int client_dimid, 
	int *server_ncid, int *server_dimid);
/**
 * @brief: get (server_ncid, server_varid) by (client_ncid, client_varid) in
 *	server
 *
 * @param client_id: the client proc id
 * @param client_ncid: the nc file id in client
 * @param client_varid: the var id in client
 * @param server_ncid: the nc file id in server
 * @param server_varid: the var id in server
 *
 * @return: error code
 */
int ifow_id_get_var(int client_id,
	int client_ncid, int client_varid, 
	int *server_ncid, int *server_varid);

#endif

/****************************************************************************
 *       Filename:  iofw_error.h
 *
 *    Description:  macro define for errors
 *
 *        Version:  1.0
 *        Created:  10/22/2012 03:15:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _IOFW_ERROR_H
#define _IOFW_ERROR_H

#define IOFW_ERROR_NONE               0

#define IOFW_ERROR_MALLOC	    -1
#define IOFW_ERROR_HASH_TABLE_INIT  -2    /* data index exceeds dimension bound */
#define IOFW_ERROR_INVALID_INIT_ARG -3
#define IOFW_ERROR_ARG_NULL	    -4	    /* some arg is NULL */
/* In server.c */
#define IOFW_ERROR_PTHREAD_CREATE   -100
#define IOFW_ERROR_UNEXPECTED_MSG   -101
/* In iofw.c */
#define IOFW_ERROR_FINAL_AFTER_MPI  -200    /* iofw_final should be called before
					       mpi_final*/
#define IOFW_ERROR_RANK_INVALID	    -201    
/* In msg.c */
#define IOFW_ERROR_MPI_RECV	    -300    /* MPI_Recv error */
/* In id.c */
#define IOFW_ERROR_EXCEED_BOUND	    -400    /* data index exceeds dimension bound */
#define IOFW_ERROR_NC_NO_EXIST	    -401    /* nc_id not found in assign_table */

#endif

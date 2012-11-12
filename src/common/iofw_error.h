/****************************************************************************
 *       Filename:  error.h
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
#ifndef _ERROR_H
#define _ERROR_H

#define IOFW_ERROR_NONE               0

#define IOFW_ERROR_MALLOC	    -1
#define IOFW_ERROR_INVALID_INIT_ARG -2
#define IOFW_ERROR_TOO_MANY_OPEN    -3	    /* Too Many Open NC File */
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

#endif

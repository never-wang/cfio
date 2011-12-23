/*
 * =====================================================================================
 *
 *       Filename:  iofw.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/19/2011 06:29:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#ifndef _IO_FW_H
#define	_IO_FW_H

const int BUFFER_SIZE = (1024*1024*1024);
const int CHUNK_SIZE = 32*1024;
const int MAX_QUEUE_SIZE = 1024*1024*1024;
#include "pomme_queue.h"
#include "unmap.h"
#include "pack.h"

#ifdef DEBUG
#define debug(msg,argc...) fprintf(stderr,msg,##argc)
#else 
#define debug(msg,argc...) 0
#endif

/**********************************************************
 *  des: init the io forwrding invironment
 *  @param iofw_servers: the number of iofw_servers;
 *  @param is_server: whether current proc is a iofw_server
 *********************************************************/
int iofw_init(int iofw_servers,
	      int *is_server);

/**
 * @brief iofw_Finalize : stop the iofw services, the function 
 * should be called before the mpi_Finalize
 *
 * @return 
 */
int iofw_Finalize();

#endif

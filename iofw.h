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
#define CHUNK_SIZE 32*1024

/**********************************************************
 *  des: init the io forwrding invironment
 *  @param iofw_servers: the number of iofw_servers;
 *  @param is_server: whether current proc is a iofw_server
 *********************************************************/
int iofw_init(int iofw_servers,
	      int *is_server
		);

/*
 ************************************************************************************
 *  des: the function for the server,receiving and do the io operation
 * @param buffer_size: the total buffer_size of the server
 *************************************************************************************
 */
int iofw_server(unsigned int buffer_size);

#endif

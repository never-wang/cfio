/****************************************************************************
 *       Filename:  server.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/19/2012 01:57:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _SERVER_H
#define _SERVER_H

#define SERVER_BUF_SIZE 1024*1024*1024

/**
 * @brief: init
 *
 * @return: error code
 */
int iofw_server_init();
/**
 * @brief: final 
 *
 * @return: error code
 */
int iofw_server_final();
/**
 * @brief: start iofw server
 *
 * @return: error code
 */
int iofw_server_start();

#endif

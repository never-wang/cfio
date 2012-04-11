/****************************************************************************
 *       Filename:  client_buf.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/11/2012 04:17:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _CLINET_BUF_H
#define _CLIENT_BUF_H

typedef struct
{
    uint16_t magic;	/* magic of the buffer */
    size_t size;	/* space size of the buffer */
    void *start_p;	/* start address of the buffer */
    void *free_p;	/* start address of free buffer */
    void *alloc_p;	/* start address of alloc buffer */
    uint16_t magic2;	/* upper magic of the buffer */
}iofw_buf_t

iofw_buf_t *iofw_buf_open(size_t size, int *error);

void *iofw_buf_malloc(iofw_buf_t

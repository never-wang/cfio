/*
 * =====================================================================================
 *
 *       Filename:  unmap.h
 *
 *    Description: unmap the io_forward op into a real operation 
 *
 *        Version:  1.0
 *        Created:  12/20/2011 04:54:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#ifndef _UNMAP_H
#define _UNMAP_H
#include "utils.h"

#define FFL __FILE__,__func__,__LINE__

#ifdef DEBUG
#define debug(msg,argc...) fprintf(stderr,msg,##argc)
#else 
#define debug(msg,argc...) 0
#endif

typedef struct io_op
{
	void *head;
	int offset;
	int length;//both the length of the head and data
	queue_body_t next_head;
}io_op_t;

typedef struct io_op_queue
{
	pomme_buffer_t *buffer;
	pomme_queue_t *opq;
}ioop_queue_t;
/*
 ************************************************************************************
 *  unmap an oper and real do it
 *  @param data: the data header
 *  @param size: the size of the data
 *************************************************************************************
 */
int unmap(void *data ,int size);

/*
 ************************************************************************************
 *  init an io_op_queue structure
 *  @param size: the size of the buffer
 *  @param chunk_size: the max message size
 *  @param max_qlength: the max length of the queue
 *  @queue_name: the name of the queue
 *************************************************************************************
 */
int io_op_queue_init(int size, int chunk_size, int max_qlength,char *queue_name);

#endif

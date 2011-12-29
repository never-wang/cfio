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
#include "pomme_buffer.h"

#define FFL __FILE__,__func__,__LINE__
#define DEBUG 1

#ifdef DEBUG
#define debug(msg,argc...) fprintf(stderr,msg,##argc)
#else 
#define debug(msg,argc...) 0
#endif
/* return codes */
#define	ALL_CLINET_REPORT_DONE 1
/* the msg is delt, the buffer could be reused inmmediately */
#define DEALT_MSG 2
/* the msg is put into the queue, the buffer should be keep util
 * the write to free it */
#define ENQUEUE_MSG 3

typedef struct io_op
{
	int src;
	int tag;
	void *head;
	int head_len;
	void *body;
	int body_len;
	queue_body_t next_head;
}io_op_t;

typedef struct io_op_queue
{
	pomme_buffer_t *buffer;
	pomme_queue_t *opq;
}ioop_queue_t;
/*
 ************************************************************************************
 *  brief:unmap an oper , buffer of real do it
 *  @param src, where the msg come from
 *  @param tag, the msg tag
 *  @param my_rank, self rank
 *  @param data: the data header
 *  @param size: the size of the data
 *  @param data_len: the length of the data in bytes
 *  @return : the rest length of the real data
 *************************************************************************************
 */
int unmap(int src, int tag, int my_rank,void *data ,int size, size_t *data_len);

/*
 ************************************************************************************
 *  @brief:init an io_op_queue structure
 *  @param size: the size of the buffer
 *  @param chunk_size: the max message size
 *  @param max_qlength: the max length of the queue
 *  @queue_name: the name of the queue
 *************************************************************************************
 */
int io_op_queue_init(ioop_queue_t *io_queue,int size, 
		int chunk_size, int max_qlength,char *queue_name);

/**
 * @brief iofw_do_io: write the buffered data into ncfile
 *
 * @param source: where the msg is from
 * @param tag:the tag of the msg
 * @param my_rank: the rank of the server
 * @param op: op infomation
 *
 * @return 0 for success, < 0 failure
 */
int iofw_do_io(int source,int tag, int my_rank, io_op_t *op);
#endif

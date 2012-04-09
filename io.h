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
#include "pack.h"
#include "mpi.h"

/* return codes */
#define	ALL_CLINET_REPORT_DONE 1
/* the msg is delt, the buffer could be reused inmmediately */
#define DEALT_MSG 2
/* the msg is put into the queue, the buffer should be keep util
 * the write to free it */
#define ENQUEUE_MSG 3
#define IMM_MSG 4

typedef struct io_op
{
	int src;
	int tag;
	void *head;
	void *body;
	int head_start;
	int head_len;
	int body_start;
	int body_len;
	queue_body_t next_head;
}io_op_t;

typedef struct io_op_queue
{
	pomme_buffer_t *buffer;
	pomme_queue_t *opq;
}ioop_queue_t;
/**
 * @brief: init io variable, only be called in iofw_server's main
 * 
 * @param init_inter_comm: init value of inter_comm
 *
 * @return: 0
 */
int iofw_io_init(MPI_Comm init_inter_comm);
/**
 * @brief: check if the server can be stopped, server can stop when 
 *	all it's clients io done
 *
 * @param client_done: number of clients whose io is done
 * @param server_done: whether server can be stopped
 * @param client_to_serve: number of the server's clients
 *
 * @return: 0
 */
int iofw_io_client_done(int *client_done, int *server_done,
	int client_to_serve);
/*
 * @brief: do the real nc create, 
 * @param source : where the msg is from
 * @param my_rank: self rank
 * @iofw_buf_t * buf: pack bufer
 */
int iofw_io_nc_create(int source, int my_rank, iofw_buf_t *buf);
/*@brief: nc define dim 
 *@param source: where the msg is from 
 *@param my_rank: self rank
 *@param buf: msg content 
 * */
int iofw_io_nc_def_dim(int source, int my_rank,iofw_buf_t *buf);
/**
 * @brief iofw_nc_def_var : define a varible in an ncfile
 *
 * @param src: the rank of the client
 * @param my_rank: self rank
 * @param buf: data content
 *
 * @return : o for success ,< for error
 */
int iofw_io_nc_def_var(int src, int my_rank, iofw_buf_t *buf);
/**
 * @brief iofw_io_nc_end_def 
 *
 * @param src
 * @param my_rank
 * @param buf
 *
 * @return 
 */
int iofw_io_nc_end_def( int src, int my_rank, iofw_buf_t *buf);
/**
 * @brief iofw_nc_put_vara_float : write an array to an varible
 *
 * @param src: where the msg com from 
 * @param my_rank: the rank of the server
 * @param h_buf: the buf head to header  
 * @param d_buf: the buf to the data
 *
 * @return 
 */
int iofw_io_nc_put_vara_float(int src, int my_rank, iofw_buf_t *h_buf);
/**
 * @brief iofw_io_nc_close 
 *
 * @param src
 * @param my_rank
 * @param buf
 *
 * @return 
 */
int iofw_io_nc_close(int src, int my_rank,iofw_buf_t *buf);
/**
 * @brief iofw_nc_put_var1_float : 
 *
 * @param src: the rank of the client
 * @param my_rank: 
 * @param buf
 *
 * @return 
 */
int iofw_io_nc_put_var1_float(int src, int my_rank,iofw_buf_t *buf);
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

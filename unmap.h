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
typedef struct io_op
{
	void *head;
	int offset;
	int length;//both the length of the head and data
	queue_body_t next_head;
}io_op_t;

/*
 ************************************************************************************
 *  unmap an oper and real do it
 *************************************************************************************
 */
int unmap(io_op_t *);

#endif

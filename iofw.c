/*
 * =====================================================================================
 *
 *       Filename:  iofw.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/19/2011 06:44:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#include "iofw.h"
#include "pomme_buffer.h"
#include <pthread.h>
#include <mpi.h>
/* the thread read the buffer and write to the real io node */
static pthread_t writer;
/* the thread listen to the mpi message and put data into buffer */
static pthread_t reader;
/* buffer */
static pomme_buffer_t *buffer = NULL;
static void * iofw_writer(void *argv)
{
	return NULL;	
}
int iofw_server(unsigned int buffer_size)
{
	int ret = 0;
	ret = pomme_buffer_init(&buffer,buffer_size,CHUNK_SIZE);
	if( ret < 0 )
	{
		fprintf(stderr,"Init buffer failed\n");
		return -1;
	}
	reader = pthread_self();
	if( (ret = pthread_create(&writer,NULL,&iofw_writer,NULL))<0  )
	{
		fprintf(stderr,"Thread create error%s %s %d\n",__FILE__,__func__,__LINE__);
		return ret;
	}
	static int done = 0;
	void *p_buffer = NULL;
	while( done == 0 )
	{
		int offset = 0;
		do{
			offset = pomme_buffer_next(buffer,buffer->chunk_size);
		}while(offset<0);	
		p_buffer = buffer->buffer+offset;
	}
		
}

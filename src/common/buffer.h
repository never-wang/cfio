/****************************************************************************
 *       Filename:  buffer.h
 *
 *    Description:  ring buffer for msg send and recv
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
#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdint.h>

#include "debug.h"

#define IOFW_BUF_ERROR_NONE	0
#define IOFW_BUF_ERROR_NO_MEM	1 /* the buffer has no available memory */
#define IOFW_BUF_ERROR_NO_DATA	2 /* the buffer has no enough data to unpack */
#define IOFW_BUF_ERROR_SBRK	3 /* sbrk fail */
#define IOFW_BUF_ERROR_OVER	4 /* the iofw_buf_t struct was overwritten */

#define IOFW_BUF_MAGIC 0xABCD

#define SET_ERROR(pnt, val) \
    do { \
	if(NULL != (pnt)) { \
	    (*(pnt)) = (val); \
	}} while(0)

#define iofw_buf_data_size(len) (len)
#define iofw_buf_data_array_size(len, size) \
    (iofw_buf_data_size(len) * (size) + sizeof(int))
#define iofw_buf_str_size(str) \
    iofw_buf_data_array_size(strlen((char*)str) + 1, sizeof(char))

typedef struct
{
    uint16_t magic;	/* magic of the buffer */
    size_t size;	/* space size of the buffer */
    char *start_addr;	/* start address of the buffer */
    char *free_addr;	/* start address of free buffer */
    char *used_addr;	/* start address of used buffer */
    uint16_t magic2;	/* upper magic of the buffer */
}iofw_buf_t;

/**
 * @brief: increase a buffer's free_addr, means more buffer space was used
 *	buffer space was freed
 *
 * @param buf_p: pointer to the buffer
 * @param size: the size to increase
 */
static inline void use_buf(iofw_buf_t *buf_p, size_t size)
{
    buf_p->free_addr += size;
    if(buf_p->free_addr >= ((buf_p->start_addr) + buf_p->size))
    {
	buf_p->free_addr -= buf_p->size;
    }
}
/**
 * @brief: increase a buffer's used_addr or free_addr, means some buffer space 
 *	was freed
 *
 * @param buf_p: pointer to the buffer
 * @param size: the size to increase
 */
static inline void free_buf(iofw_buf_t *buf_p, size_t size)
{
    buf_p->used_addr += size;
    if(buf_p->used_addr >= ((buf_p->start_addr) + buf_p->size))
    {
	buf_p->used_addr -= buf_p->size;
    }
}


/**
 * get the free space size in a buffer
 **/
#define free_buf_size(buf_p) \
    (((buf_p)->size + (buf_p)->used_addr - (buf_p)->free_addr - 1) \
     % (buf_p)->size)

/**
 * @brief: get the used space size in a buffer
 *
 * @param buf_p: pointer to the buffer
 *
 * @return: used space size of the buffer
 */
#define used_buf_size(buf_p) \
    (((buf_p)->size + (buf_p)->free_addr - (buf_p)->used_addr) \
     % (buf_p)->size)

static inline void ensure_free_space(iofw_buf_t *buf_p, size_t size, void(*free)())
{
    size_t left_space = buf_p->start_addr + buf_p->size - buf_p->free_addr;
    volatile size_t free_size;
    
    while((free_size = free_buf_size(buf_p)) < size)
    {
	free();
    }
    /* if buffer tail left size < data size, move free_addr to start of buffer*/
    if(size > left_space)
    {
	use_buf(buf_p, left_space);
	while((free_size = free_buf_size(buf_p)) < size)
	{
	    free();
	}
    }
}

/**
 * @brief: check whether addr is in one buffer's used pace
 *
 * @param addr: the to check addr
 * @param buf_p: the buffer
 *
 * @return: true if in 
 */
static inline int check_used_addr(char *addr, iofw_buf_t *buf_p)
{
    assert(addr >= buf_p->start_addr && addr < buf_p->start_addr + buf_p->size);

    if(buf_p->used_addr < buf_p->free_addr)
    {
	if(buf_p->used_addr <= addr && addr < buf_p->free_addr)
	{
	    return 1;
	}else
	{
	    return 0;
	}
    }else
    {
	if(buf_p->free_addr <= addr && addr < buf_p->used_addr)
	{
	    return 0;
	}else
	{
	    return 1;
	}
    }
}

/**
 * @brief: create a new buffer , and init
 *
 * @param size: size of the buffer
 * @param error: error code 
 *
 * @return: pointer to the new buffer
 */
iofw_buf_t *iofw_buf_open(size_t size, int *error);

/**
 * @brief: clear the buffer's data
 *
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_clear(iofw_buf_t *buf_p);

/**
 * @brief: pack one data in the buffer
 *
 * @param data: pointer to the to pack data
 * @param size: size of the to pack data
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_pack_data(
	const void *data, size_t size, iofw_buf_t *buf_p);

/**
 * @brief: unpack one data from the buffer
 *
 * @param data: pointer to the unpacked data
 * @param size: size of the to unpack data
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_unpack_data(
	void *data, size_t size, iofw_buf_t *buf_p);
/**
 * @brief: pack an array of data into the buffer
 *
 * @param data: pointer to the data array
 * @param len: length of the array
 * @param size: size of one data
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_pack_data_array(
	const void *data, size_t len,
	size_t size, iofw_buf_t *buf_p);

/**
 * @brief: unpack an array of data from the buffer, the func will malloc
 *	space for the unpacked data
 *
 * @param data: pointer to the unpacked data array
 * @param len: length of the array
 * @param size: size of one data
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_unpack_data_array(
	void **data, int *len, 
	size_t size, iofw_buf_t *buf_p);

/**
 * @brief: unpack an array of data from the buffer, the unpacked data pointer 
 *	just point to the address in buffer, no memcpy happen; the caller should
 *	free the data when finishing using the data
 *
 * @param data: pointer to the unpacked data array
 * @param len: length of the array
 * @param size: size of one data
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_unpack_data_array_ptr(
	void **data, int *len,
	const size_t size, iofw_buf_t *buf_p);

    
#define iofw_buf_pack_str(str, buf) \
    iofw_buf_pack_data_array((const void*)(str), strlen((char*)str) + 1, \
	    sizeof(char), buf)
#define iofw_buf_unpack_str(str, buf) \
    iofw_buf_unpack_data_array((void **)str, NULL, sizeof(char), buf)
#define iofw_buf_unpack_str_ptr(str, buf) \
    iofw_buf_unpack_data_array_ptr((void **)str, NULL, sizeof(char), buf)

#endif


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

typedef struct
{
    uint16_t magic;	/* magic of the buffer */
    size_t size;	/* space size of the buffer */
    void *start_addr;	/* start address of the buffer */
    void *free_addr;	/* start address of free buffer */
    void *used_addr;	/* start address of used buffer */
    int (*free)();	/* free function */
    uint16_t magic2;	/* upper magic of the buffer */
}iofw_buf_t;

/**
 * @brief: increase a buffer's used_addr or free_addr
 *
 * @param buf_p: pointer to the buffer
 * @param addr: pointer to the addr which is to be increased
 * @param size: the size to increase
 */
static inline void inc_buf_addr(iofw_buf_t *buf_p, char **addr, size_t size)
{
    (*addr) += size;
    if((*addr) >= (((char *)buf_p->start_addr) + buf_p->size))
    {
	(*addr) -= buf_p->size;
    }
}

/**
 * @brief: get the free space size in a buffer
 *
 * @param buf_p: pointer to the buffer
 *
 * @return: free space size of the buffer
 */
static inline size_t free_buf_size(iofw_buf_t *buf_p)
{
    if(buf_p->free_addr < buf_p->used_addr)
    {
	return buf_p->used_addr - buf_p ->free_addr - 1;
    }else
    {
	return buf_p->size + buf_p->used_addr - buf_p->free_addr - 1;
    }
}

/**
 * @brief: get the used space size in a buffer
 *
 * @param buf_p: pointer to the buffer
 *
 * @return: used space size of the buffer
 */
static inline size_t used_buf_size(iofw_buf_t *buf_p)
{
    if(buf_p->free_addr >= buf_p->usd_addr)
    {
	return buf_p->free_addr - buf_p->used_addr;
    }else
    {
	return buf_p->size + buf_p->free_addr - buf_p->used_addr;
    }
}

static inline void ensure_free_space(iofw_buf_t *buf_p, size_t size)
{
    while(free_buf_size(buf_p) < size)
    {
	buf_p->free();
    }
    return;
}

/**
 * @brief: create a new buffer , and init
 *
 * @param size: size of the buffer
 * @param free: free fuction for the buffer
 * @param error: error code 
 *
 * @return: pointer to the new buffer
 */
iofw_buf_t *iofw_buf_open(size_t size, void (*free)(), int *error);

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
	const void *data, const size_t size, iofw_buf_t *buf_p);

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
	const void *data, const unsigned len,
	const size_t size, iofw_buf_t *buf_p);

/**
 * @brief: unpack an array of data from the buffer
 *
 * @param data: pointer to the unpacked data array
 * @param len: length of the array
 * @param size: size of one data
 * @param buf_p: pointer to the buffer
 *
 * @return: error code
 */
int iofw_buf_unpack_data_array(
	void **data, unsigned int *len, 
	const size_t size, iofw_buf_t *buf_p);

#endif


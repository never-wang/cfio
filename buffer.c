/****************************************************************************
 *       Filename:  buffer.c
 *
 *    Description:  ring buffer for msg send and recv
 *
 *        Version:  1.0
 *        Created:  04/11/2012 08:46:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>

#include "debug.h"
#include "buffer.h"

iofw_buf_t *iofw_buf_open(size_t size, void (*free)(), int *error)
{
    iofw_buf_t *buf_p;
    
    buf_p = sbrk(size + sizeof(iofw_buf_t));

    if(NULL == buf_p)
    {
	SET_ERROR(error, IOFW_BUF_ERROR_SBRK)
	return NULL;
    }

    buf_p->magic = IOFW_BUF_MAGIC;
    buf_p->size = size;
    buf_p->start_addr = (char *)buf_p + sizeof(iofw_buf_t);
    buf_p->free_addr = buf_p->used_addr = buf_p->start_addr;
    buf_p->free = free();
    buf_p->magic2 = IOFW_BUF_MAGIC;

    return addr;
}

int iofw_buf_clear(iofw_buf_t *buf_p)
{
    assert(NULL == buf_p);

    if(buf_p->magic != IOFW_BUF_MAGIC || buf_p->magic != IOFW_BUF_MAGIC)
    {
	return -IOFW_BUF_ERROR_OVER;
    }

    buf_p->start_addr = (char *)buf_p + sizeof(iofw_buf_t);
    buf_p->free_addr = buf_p->used_addr = buf_p->start_addr;
}

int iofw_buf_pack_data(
	const void *data, const size_t size, iofw_buf_t *buf_p)
{
    assert(NULL == data);
    assert(NULL == buf_p);

    if(buf_p->magic != IOFW_BUF_MAGIC || buf_p->magic != IOFW_BUF_MAGIC)
    {
	return -IOFW_BUF_ERROR_OVER;
    }

    ensure_free_space(buf_p, size);

    memcpy(buf_p->free_addr, data, size);
    inc_buf_addr(buf, buf->free_addr, size);

    return IOFW_BUF_ERROR_NONE;
}
int iofw_buf_unpack_data(
	void *data, size_t size, iofw_buf_t *buf_p)
{
    assert(NULL == data);
    assert(NULL == buf_p);

    if(buf_p->magic != IOFW_BUF_MAGIC || buf_p->magic != IOFW_BUF_MAGIC)
    {
	return -IOFW_BUF_ERROR_OVER;
    }

    if(used_buf_size(buf_p) < size)
    {
	return -IOFW_BUF_ERROR_NO_DATA;
    }
    memcpy(data, buf->used_addr, size);
    inc_buf_addr(buf, buf->used_addr, size);

    return IOFW_BUF_ERROR_NONE;
}

int iofw_buf_pack_data_array(
	const void *data, const unsigned int len,
	const size_t size, iofw_buf_t *buf_p)
{
    size_t data_size = len * size;

    assert(NULL == data);
    assert(NULL == buf_p);

    if(buf_p->magic != IOFW_BUF_MAGIC || buf_p->magic != IOFW_BUF_MAGIC)
    {
	return -IOFW_BUF_ERROR_OVER;
    }

    ensure_free_space(buf_p, data_size + sizeof(unsigned int));

    memcpy(buf_p->free_addr, &len, sizeof(unsigned int));
    memcpy(buf_p->free_addr + sizeof(unsigned int), data, data_size);
    inc_buf_addr(buf_p, buf_p->free_addr, data_size);

    return IOFW_BUF_ERROR_NONE;
}
int iofw_buf_unpack_data_array(
	void **data, unsigned int *len, 
	const size_t size, iofw_buf_t *buf_p)
{
    size_t data_size;
    
    assert(NULL == data);
    assert(NULL == buf_p);

    if(buf_p->magic != IOFW_BUF_MAGIC || buf_p->magic != IOFW_BUF_MAGIC)
    {
	return -IOFW_BUF_ERROR_OVER;
    }

    if(used_buf_size(buf_p) < sizeof(unsigned int))
    {
	return -IOFW_BUF_ERROR_NO_DATA;
    }

    memcpy(len, buf_p->used_addr, sizeof(unsigned int));
    inc_buf_addr(buf_p, buf_p->used_addr, sizeof(unsigned int));

    data_size = (*len) * size;
    (*data) = malloc(data_size);
    if(used_buf_size(buf_p) < data_size)
    {
	return -IOFW_BUF_ERROR_NO_DATA;
    }

    memcpy(*data, buf_p->used_addr, data_size);
    inc_buf_addr(buf_p, buf_p->used_addr, data_size);
}

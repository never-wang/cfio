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

/**
 * @brief: 
 *
 * @param buf_p: 
 * @param data: 
 * @param size: 
 */
static inline void put_buf_data(iofw_buf_t *buf_p, const void *data, size_t size)
{
    char *_data = data;
	
    if(0 == size)
    {
	return;
    }
    memcpy(buf_p->free_addr, _data, size);
}
static inline void get_buf_data(iofw_buf_t *buf_p, void *data, size_t size)
{
    char *_data = data;

    if(0 == size)
    {
	return;
    }

    memcpy(_data, buf_p->used_addr, size);
}


iofw_buf_t *iofw_buf_open(size_t size, int *error)
{
    iofw_buf_t *buf_p;
    
    buf_p = sbrk(size + sizeof(iofw_buf_t));

    if(NULL == buf_p)
    {
	SET_ERROR(error, IOFW_BUF_ERROR_SBRK);
	return NULL;
    }

    buf_p->magic = IOFW_BUF_MAGIC;
    buf_p->size = size;
    buf_p->start_addr = (char *)buf_p + sizeof(iofw_buf_t);
    buf_p->free_addr = buf_p->used_addr = buf_p->start_addr;
    buf_p->magic2 = IOFW_BUF_MAGIC;

    return buf_p;
}

int iofw_buf_clear(iofw_buf_t *buf_p)
{
    assert(NULL != buf_p);

    assert(buf_p->magic == IOFW_BUF_MAGIC && buf_p->magic == IOFW_BUF_MAGIC);

    buf_p->free_addr = buf_p->used_addr = buf_p->start_addr;

    return IOFW_BUF_ERROR_NONE;
}

size_t iofw_buf_pack_size(
	const void *data, size_t size)
{
    return size;
}

int iofw_buf_pack_data(
	const void *data, size_t size, iofw_buf_t *buf_p)
{
    assert(NULL != data);
    assert(NULL != buf_p);

    assert(buf_p->magic == IOFW_BUF_MAGIC && buf_p->magic == IOFW_BUF_MAGIC);

    assert(free_buf_size(buf_p) >= size);

    put_buf_data(buf_p, data, size);
    use_buf(buf_p, size);

    return IOFW_BUF_ERROR_NONE;
}
int iofw_buf_unpack_data(
	void *data, size_t size, iofw_buf_t *buf_p)
{
    assert(NULL != data);
    assert(NULL != buf_p);
    volatile size_t used_size;

    assert(buf_p->magic == IOFW_BUF_MAGIC && buf_p->magic == IOFW_BUF_MAGIC);

    assert(used_buf_size(buf_p) >= size);

    get_buf_data(buf_p, data, size);
    free_buf(buf_p, size);

    return IOFW_BUF_ERROR_NONE;
}

size_t iofw_buf_pack_array_size(
	const void *data, int len, size_t size)
{
    return len * (size_t)size + (sizeof(int));
}


int iofw_buf_pack_data_array(
	const void *data, int len,
	size_t size, iofw_buf_t *buf_p)
{
    
    /**
     *remember consider len = 0
     **/

    size_t data_size = (size_t)len * size;

    assert(NULL != buf_p);

    assert((buf_p->magic == IOFW_BUF_MAGIC && buf_p->magic == IOFW_BUF_MAGIC));

    assert((free_buf_size(buf_p) >= (data_size + sizeof(int))));

    put_buf_data(buf_p, &len, sizeof(int));
    use_buf(buf_p, sizeof(int));
    put_buf_data(buf_p, data, data_size);
    use_buf(buf_p, data_size);

    return IOFW_BUF_ERROR_NONE;
}

int iofw_buf_unpack_data_array(
	void **data, int *len, 
	size_t size, iofw_buf_t *buf_p)
{
    assert(NULL != data);
    assert(NULL != buf_p);

    size_t data_size;
    volatile size_t used_size;
    int _len;
    
    assert((buf_p->magic == IOFW_BUF_MAGIC && buf_p->magic == IOFW_BUF_MAGIC));
    
    assert(used_buf_size(buf_p) >= sizeof(int));

    get_buf_data(buf_p, &_len, sizeof(int));
    free_buf(buf_p, sizeof(int));
    if(NULL != len)
    {
	*len = _len;
    }

    if(0 == _len)
    {
	return IOFW_BUF_ERROR_NONE;
    }

    data_size = _len * size;
    (*data) = malloc(data_size);

    assert(used_buf_size(buf_p) >= data_size);

    debug(DEBUG_MSG, "data_size = %lu", data_size);

    get_buf_data(buf_p, *data, data_size);
    free_buf(buf_p, data_size);

    return IOFW_BUF_ERROR_NONE;
}

int iofw_buf_unpack_data_array_ptr(
	void **data, int *len, 
	size_t size, iofw_buf_t *buf_p)
{
    assert(NULL != data);
    assert(NULL != buf_p);

    size_t data_size;
    volatile size_t used_size;
    int _len;
    
    assert(buf_p->magic == IOFW_BUF_MAGIC && buf_p->magic == IOFW_BUF_MAGIC);

    assert(used_buf_size(buf_p) >= sizeof(int));

    get_buf_data(buf_p, &_len, sizeof(int));
    free_buf(buf_p, sizeof(int));
    if(NULL != len)
    {
	*len = _len;
    }
    free_buf(buf_p, sizeof(int));

    data_size = (*len) * size;
    assert(used_buf_size(buf_p) >= data_size);

    (*data) = buf_p->used_addr;

    return IOFW_BUF_ERROR_NONE;
}

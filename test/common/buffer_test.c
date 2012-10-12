/****************************************************************************
 *       Filename:  buffer_test.c
 *
 *    Description:  test buffer.c
 *
 *        Version:  1.0
 *        Created:  09/26/2012 02:03:54 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
/**
 *no need to test
 **/
#include <time.h>

#include "buffer.h"
#include "debug.h"

#define test_loop 100
#define BUF_SIZE 1024*1024

iofw_buf_t *buffer;

void pack_int()
{
    int data = rand();
    debug(DEBUG_USER, "pack data = %d", data);
    iofw_buf_pack_data(&data, sizeof(int), buffer);
}

void unpack_int()
{
    int data;
    iofw_buf_unpack_data(&data, sizeof(int), buffer);
    debug(DEBUG_USER, "unpack data = %d", data);
}
void unpack_int_array();

void pack_int_array()
{
    int max_array_len = BUF_SIZE / sizeof(int) / 2;
    int len = rand() % max_array_len + 1;
    int *data = malloc(len * sizeof(int));
    int i;
    int size = 0;

    for(i = 0; i < len; i ++)
    {
	data[i] = rand();
    }
    size += iofw_buf_data_array_size(len, sizeof(int));
    ensure_free_space(buffer, size, unpack_int_array);
    iofw_buf_pack_data_array(data, len, sizeof(int), buffer);
    debug(DEBUG_USER, "pack data array, len = %d", len);
    debug(DEBUG_USER, "data[0] = %d, data[%d] = %d, data[%d] = %d\n",
	    data[0], len / 2, data[len / 2], len - 1, data[len - 1]);
    free(data);
}

void unpack_int_array()
{
    int *data;
    int len;

    debug_mark(DEBUG_USER);
    iofw_buf_unpack_data_array((void **)&data, &len, sizeof(int), buffer); 
    debug(DEBUG_USER, "unpack data array, len = %d", len);
    debug(DEBUG_USER, "data[0] = %d, data[%d] = %d, data[%d] = %d\n",
	    data[0], len / 2, data[len / 2], len - 1, data[len - 1]);
    free(data);
}

int main()
{
    int ret;
    int i;
 
    srand(time(NULL));
    set_debug_mask(DEBUG_USER | DEBUG_BUF);

    buffer = iofw_buf_open(BUF_SIZE, &ret);
 
    pack_int_array();

    iofw_buf_close(buffer);
    return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "pack.h"
#include "debug.h"

/* If we unpack a buffer that contains bad data, we want to avoid
 * memory allocation error due to array or buffer sizes that are
 * unreasonably large. Increase this limits as needed. */
#define MAX_PACK_ARRAY_LEN	(128 * 1024)
#define MAX_PACK_MEM_LEN	(16 * 1024 * 1024)
#define MAX_PACK_STR_LEN	(16 * 1024 * 1024)

#define _ERROR -1
#define _SUCCESS 0

static void _error(const char *str)
{
    fprintf(stderr, "%s\n", str);
}

/* Basic buffer management routines */
/* create_buf - create a buffer with the supplied contents, contents must
 * be yuga_xalloc'ed */
Buf create_buf(char *data, int size)
{
	Buf my_buf;

	if (size > MAX_BUF_SIZE) {
		fprintf(stderr, "create_buf: buffer size too large\n");
		return NULL;
	}

	my_buf = malloc(sizeof(struct yuga_buf));
	my_buf->magic = BUF_MAGIC;
	my_buf->size = size;
	my_buf->processed = 0;
	my_buf->head = data;

	return my_buf;
}

/* free_buf - release memory associated with a given buffer */
void free_buf(Buf my_buf)
{
	assert(my_buf->magic == BUF_MAGIC);
	free(my_buf->head);
	free(my_buf);
}

/* Grow a buffer by the specified amount */
void grow_buf (Buf buffer, int size)
{
	if (buffer->size > (MAX_BUF_SIZE - size)) {
		_error("grow_buf: buffer size too large");
		return;
	}

	buffer->size += size;
	buffer->head = realloc(buffer->head, buffer->size);
}

/* init_buf - create an empty buffer of the given size */
Buf init_buf(int size)
{
	Buf my_buf;

	if (size > MAX_BUF_SIZE) {
		_error("init_buf: buffer size too large");
		return NULL;
	}
	if(size <= 0)
		size = BUF_SIZE;
	my_buf = malloc(sizeof(struct yuga_buf));
	my_buf->magic = BUF_MAGIC;
	my_buf->size = size;
	my_buf->processed = 0;
	my_buf->head = malloc(sizeof(char)*size);
	return my_buf;
}

/*
 * Given a 64-bit integer in host byte order, convert to network byte order
 * store in buffer, and adjust buffer counters.
 */
void pack64(uint64_t val, Buf buffer)
{
	uint64_t nl =  val;
	    
	debug(DEBUG_PACK, "%lu", val);

	if (remaining_buf(buffer) < sizeof(nl)) {
		if (buffer->size > (MAX_BUF_SIZE - BUF_SIZE)) {
			_error("pack64: buffer size too large");
			return;
		}
		buffer->size += BUF_SIZE;
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &nl, sizeof(nl));
	buffer->processed += sizeof(nl);
}

/*
 * Given a buffer containing a network byte order 64-bit integer,
 * store a host integer at 'valp', and adjust buffer counters.
 */
int unpack64(uint64_t * valp, Buf buffer)
{
	uint64_t nl;
	if (remaining_buf(buffer) < sizeof(nl))
		return _ERROR;

	memcpy(&nl, &buffer->head[buffer->processed], sizeof(nl));
	*valp = nl;
	buffer->processed += sizeof(nl);
	return _SUCCESS;
}

/*
 * Given a 32-bit integer in host byte order, convert to network byte order
 * store in buffer, and adjust buffer counters.
 */
void pack32(uint32_t val, Buf buffer)
{
	uint32_t nl = val;

	if (remaining_buf(buffer) < sizeof(nl)) {
		if (buffer->size > (MAX_BUF_SIZE - BUF_SIZE)) {
			_error("pack32: buffer size too large");
			return;
		}
		buffer->size += BUF_SIZE;
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &nl, sizeof(nl));
	buffer->processed += sizeof(nl);
}

/*
 * Given a buffer containing a network byte order 32-bit integer,
 * store a host integer at 'valp', and adjust buffer counters.
 */
int unpack32(uint32_t * valp, Buf buffer)
{
	uint32_t nl;
	if (remaining_buf(buffer) < sizeof(nl))
		return _ERROR;

	memcpy(&nl, &buffer->head[buffer->processed], sizeof(nl));
	*valp = nl;
	buffer->processed += sizeof(nl);
	return _SUCCESS;
}

/*
 * Given a 16-bit integer in host byte order, convert to network byte order,
 * store in buffer and adjust buffer counters.
 */
void pack16(uint16_t val, Buf buffer)
{
	uint16_t ns = val;

	if (remaining_buf(buffer) < sizeof(ns)) {
		if (buffer->size > (MAX_BUF_SIZE - BUF_SIZE)) {
			_error("pack16: buffer size too large");
			return;
		}
		buffer->size += BUF_SIZE;
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &ns, sizeof(ns));
	buffer->processed += sizeof(ns);
}

/*
 * Given a buffer containing a network byte order 16-bit integer,
 * store a host integer at 'valp', and adjust buffer counters.
 */
int unpack16(uint16_t * valp, Buf buffer)
{
	uint16_t ns;

	if (remaining_buf(buffer) < sizeof(ns))
		return _ERROR;

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*valp = ns;
	buffer->processed += sizeof(ns);
	return _SUCCESS;
}

/*
 * Given a 8-bit integer in host byte order, convert to network byte order
 * store in buffer, and adjust buffer counters.
 */
void pack8(uint8_t val, Buf buffer)
{
	if (remaining_buf(buffer) < sizeof(uint8_t)) {
		if (buffer->size > (MAX_BUF_SIZE - BUF_SIZE)) {
			_error("pack8: buffer size too large");
			return;
		}
		buffer->size += BUF_SIZE;
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &val, sizeof(uint8_t));
	buffer->processed += sizeof(uint8_t);
}

/*
 * Given a buffer containing a network byte order 8-bit integer,
 * store a host integer at 'valp', and adjust buffer counters.
 */
int unpack8(uint8_t * valp, Buf buffer)
{
	if (remaining_buf(buffer) < sizeof(uint8_t))
		return _ERROR;

	memcpy(valp, &buffer->head[buffer->processed], sizeof(uint8_t));
	buffer->processed += sizeof(uint8_t);
	return _SUCCESS;
}

void packdata(void *data, size_t size, Buf buffer)
{
    debug(DEBUG_PACK, "%lu", *((uint64_t*)data));

    switch(size)
    {
	case 1 :
	    pack8(*(uint8_t*)data, buffer);
	    break;
	case 2 :
	    pack16(*(uint16_t*)data, buffer);
	    break;
	case 4 :
	    pack32(*(uint32_t*)data, buffer);
	    break;
	case 8 :
	    pack64(*(uint64_t*)data, buffer);
	    break;
	default :
	    error("Unsupport data size, size = %ld", size);
    }
}
int unpackdata(void *data, size_t size, Buf buffer)
{
    switch(size)
    {
	case 1 :
	    unpack8((uint8_t*)data, buffer);
	    break;
	case 2 :
	    unpack16((uint16_t*)data, buffer);
	    break;
	case 4 :
	    unpack32((uint32_t*)data, buffer);
	    break;
	case 8 :
	    unpack64((uint64_t*)data, buffer);
	    break;
	default :
	    error("Unsupport data size, size = %ld", size);
    }
    return 0;
}

void packdata_array(const void *valp, int len, size_t size, Buf buffer)
{
    int i = 0;

    packdata(&len, sizeof(int), buffer);
	
    for(i = 0; i < len; i ++)
    {
	switch(size)
	{
	    case 1 :
		pack8(*((uint8_t*)valp + i), buffer);
		break;
	    case 2 :
		pack16(*((uint16_t*)valp + i), buffer);
		break;
	    case 4 :
		pack32(*((uint32_t*)valp + i), buffer);
//		debug(DEBUG_PACK, "valp[%d] = %f", 
//				*((float *)((uint32_t*)valp + i)));
		break;
	    case 8 :
		pack64(*((uint64_t*)valp + i), buffer);
		break;
	    default :
		error("Unsupport data size, size = %ld", size);
	}
    }
}
int unpackdata_array(void **valp, int *len, size_t size, Buf buffer)
{
    int i = 0;
    float *f_p;

    unpackdata((void*)len, sizeof(int), buffer);

    f_p = (float*)&buffer->head[buffer->processed];
    debug(DEBUG_PACK, "valp[0] = %f", f_p[0]);
    if(0 == *len)
    {
	*valp = NULL;
    }else
    {
//	*valp = &buffer->head[buffer->processed];
	*valp = f_p;
	buffer->processed += (*len) * size;
    }

    return _SUCCESS;
}
/*
 * Given a pointer to memory (valp) and a size (size_val), convert
 * size_val to network byte order and store at buffer followed by
 * the data at valp. Adjust buffer counters.
 */
void packmem(const char *valp, uint32_t size_val, Buf buffer)
{
	uint32_t ns = htonl(size_val);

	if (remaining_buf(buffer) < (sizeof(ns) + size_val)) {
		if (buffer->size > (MAX_BUF_SIZE -  size_val - BUF_SIZE)) {
			_error("packmem: buffer size too large");
			return;
		}
		buffer->size += (size_val + BUF_SIZE);
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &ns, sizeof(ns));
	buffer->processed += sizeof(ns);

	if (size_val) {
		memcpy(&buffer->head[buffer->processed], valp, size_val);
		buffer->processed += size_val;
	}
}


/*
 * Given a buffer containing a network byte order 16-bit integer,
 * and an arbitrary data string, return a pointer to the
 * data string in 'valp'.  Also return the sizes of 'valp' in bytes.
 * Adjust buffer counters.
 * NOTE: valp is set to point into the buffer bufp, a copy of
 *	the data is not made
 */
int unpackmem_ptr(char **valp, uint32_t * size_valp, Buf buffer)
{
	uint32_t ns;

	if (remaining_buf(buffer) < sizeof(ns))
		return _ERROR;

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*size_valp = ntohl(ns);
	buffer->processed += sizeof(ns);

	if (*size_valp > MAX_PACK_MEM_LEN)
		return _ERROR;
	else if (*size_valp > 0) {
		if (remaining_buf(buffer) < *size_valp)
			return _ERROR;
		*valp = &buffer->head[buffer->processed];
		buffer->processed += *size_valp;
	} else
		*valp = NULL;
	return _SUCCESS;
}


/*
 * Given a buffer containing a network byte order 16-bit integer,
 * and an arbitrary data string, copy the data string into the location
 * specified by valp.  Also return the sizes of 'valp' in bytes.
 * Adjust buffer counters.
 * NOTE: The caller is responsible for the management of valp and
 * insuring it has sufficient size
 */
int unpackmem(char *valp, uint32_t * size_valp, Buf buffer)
{
	uint32_t ns;

	if (remaining_buf(buffer) < sizeof(ns))
		return _ERROR;

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*size_valp = ntohl(ns);
	buffer->processed += sizeof(ns);

	if (*size_valp > MAX_PACK_MEM_LEN)
		return _ERROR;
	else if (*size_valp > 0) {
		if (remaining_buf(buffer) < *size_valp)
			return _ERROR;
		memcpy(valp, &buffer->head[buffer->processed], *size_valp);
		buffer->processed += *size_valp;
	} else
		*valp = 0;
	return _SUCCESS;
}

/*
 * Given a buffer containing a network byte order 16-bit integer,
 * and an arbitrary data string, copy the data string into the location
 * specified by valp.  Also return the sizes of 'valp' in bytes.
 * Adjust buffer counters.
 * NOTE: valp is set to point into a newly created buffer,
 *	the caller is responsible for calling free() on *valp
 *	if non-NULL (set to NULL on zero size buffer value)
 */
int unpackmem_xmalloc(char **valp, uint32_t * size_valp, Buf buffer)
{
	uint32_t ns;

	if (remaining_buf(buffer) < sizeof(ns))
		return _ERROR;

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*size_valp = ntohl(ns);
	buffer->processed += sizeof(ns);

	if (*size_valp > MAX_PACK_STR_LEN)
		return _ERROR;
	else if (*size_valp > 0) {
		if (remaining_buf(buffer) < *size_valp)
			return _ERROR;
		*valp = malloc(*size_valp);
		memcpy(*valp, &buffer->head[buffer->processed],
		       *size_valp);
		buffer->processed += *size_valp;
	} else
		*valp = NULL;
	return _SUCCESS;
}

/*
 * Given a buffer containing a network byte order 16-bit integer,
 * and an arbitrary data string, copy the data string into the location
 * specified by valp.  Also return the sizes of 'valp' in bytes.
 * Adjust buffer counters.
 * NOTE: valp is set to point into a newly created buffer,
 *	the caller is responsible for calling free() on *valp
 *	if non-NULL (set to NULL on zero size buffer value)
 */
int unpackmem_malloc(char **valp, uint32_t * size_valp, Buf buffer)
{
	uint32_t ns;

	if (remaining_buf(buffer) < sizeof(ns))
		return _ERROR;

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*size_valp = ntohl(ns);
	buffer->processed += sizeof(ns);
	if (*size_valp > MAX_PACK_STR_LEN)
		return _ERROR;
	else if (*size_valp > 0) {
		if (remaining_buf(buffer) < *size_valp)
			return _ERROR;
		*valp = malloc(*size_valp);
		memcpy(*valp, &buffer->head[buffer->processed],
		       *size_valp);
		buffer->processed += *size_valp;
	} else
		*valp = NULL;
	return _SUCCESS;
}

/*
 * Given a pointer to array of char * (char ** or char *[] ) and a size
 * (size_val), convert size_val to network byte order and store in the
 * buffer followed by the data at valp. Adjust buffer counters.
 */
void packstr_array(char **valp, uint32_t size_val, Buf buffer)
{
	int i;
	uint32_t ns = htonl(size_val);

	if (remaining_buf(buffer) < sizeof(ns)) {
		if (buffer->size > (MAX_BUF_SIZE - BUF_SIZE)) {
			_error("packstr_array: buffer size too large");
			return;
		}
		buffer->size += BUF_SIZE;
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &ns, sizeof(ns));
	buffer->processed += sizeof(ns);

	for (i = 0; i < size_val; i++) {
		packstr(valp[i], buffer);
	}

}

/*
 * Given 'buffer' pointing to a network byte order 16-bit integer
 * (size) and a array of strings  store the number of strings in
 * 'size_valp' and the array of strings in valp
 * NOTE: valp is set to point into a newly created buffer,
 *	the caller is responsible for calling free on *valp
 *	if non-NULL (set to NULL on zero size buffer value)
 */
int unpackstr_array(char ***valp, uint32_t * size_valp, Buf buffer)
{
	int i;
	uint32_t ns;
	uint32_t uint32_tmp;

	if (remaining_buf(buffer) < sizeof(ns))
		return _ERROR;

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*size_valp = ntohl(ns);
	buffer->processed += sizeof(ns);

	if (*size_valp > MAX_PACK_ARRAY_LEN)
		return _ERROR;
	else if (*size_valp > 0) {
		*valp = malloc(sizeof(char *) * (*size_valp + 1));
		for (i = 0; i < *size_valp; i++) {
			if (unpackmem_xmalloc(&(*valp)[i], &uint32_tmp, buffer))
				return _ERROR;
		}
		(*valp)[i] = NULL;	/* NULL terminated array so that execle */
		/*    can detect end of array */
	} else
		*valp = NULL;
	return _SUCCESS;
}

/*
 * Given a pointer to memory (valp), size (size_val), and buffer,
 * store the memory contents into the buffer
 */
void packmem_array(char *valp, uint32_t size_val, Buf buffer)
{
	if (remaining_buf(buffer) < size_val) {
		if (buffer->size > (MAX_BUF_SIZE - size_val - BUF_SIZE)) {
			_error("packmem_array: buffer size too large");
			return;
		}
		buffer->size += (size_val + BUF_SIZE);
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], valp, size_val);
	buffer->processed += size_val;
}

/*
 * Given a pointer to memory (valp), size (size_val), and buffer,
 * store the buffer contents into memory
 */
int unpackmem_array(char *valp, uint32_t size_valp, Buf buffer)
{
	if (remaining_buf(buffer) >= size_valp) {
		memcpy(valp, &buffer->head[buffer->processed], size_valp);
		buffer->processed += size_valp;
		return _SUCCESS;
	} else {
		*valp = 0;
		return _ERROR;
	}
}

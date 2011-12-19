/****************************************************************************\
 *  pack.c - lowest level un/pack functions
 *  NOTE: The memory buffer will expand as needed using realloc()
 *****************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>,
 *             Morris Jette <jette1@llnl.gov>, et. al.
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <https://computing.llnl.gov/linux/slurm/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\****************************************************************************/

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "pack.h"

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

/* xfer_buf_data - return a pointer to the buffer's data and release the
 * buffer's structure */
void *xfer_buf_data(Buf my_buf)
{
	void *data_ptr;

	assert(my_buf->magic == BUF_MAGIC);
	data_ptr = (void *) my_buf->head;
	free(my_buf);
	return data_ptr;
}

/*
 * Given a time_t in host byte order, promote it to int64_t, convert to
 * network byte order, store in buffer and adjust buffer acc'd'ngly
 */
void pack_time(time_t val, Buf buffer)
{
	int64_t n64 = HTON_int64((int64_t) val);

	if (remaining_buf(buffer) < sizeof(n64)) {
		if (buffer->size > (MAX_BUF_SIZE - BUF_SIZE)) {
			_error("pack_time: buffer size too large");
			return;
		}
		buffer->size += BUF_SIZE;
		buffer->head = realloc(buffer->head, buffer->size);
	}

	memcpy(&buffer->head[buffer->processed], &n64, sizeof(n64));
	buffer->processed += sizeof(n64);
}

int unpack_time(time_t * valp, Buf buffer)
{
	int64_t n64;

	if (remaining_buf(buffer) < sizeof(n64))
		return _ERROR;

	memcpy(&n64, &buffer->head[buffer->processed], sizeof(n64));
	buffer->processed += sizeof(n64);
	*valp = (time_t) NTOH_int64(n64);
	return _SUCCESS;
}


/*
 * Given a double, multiple by FLOAT_MULT and then
 * typecast to a uint64_t in host byte order, convert to network byte order
 * store in buffer, and adjust buffer counters.
 */
void packdouble(double val, Buf buffer)
{
	double nl1 =  (val * FLOAT_MULT) + .5; /* the .5 is here to
						  round off.  We have
						  found on systems
						  going out more than
						  15 decimals will
						  mess things up so
						  this is here to
						  correct it. */
	uint64_t nl =  HTON_uint64(nl1);

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
 * typecast as double, and  divide by FLOAT_MULT
 * store a host double at 'valp', and adjust buffer counters.
 */
int	unpackdouble(double *valp, Buf buffer)
{
	uint64_t nl;
	if (remaining_buf(buffer) < sizeof(nl))
		return _ERROR;

	memcpy(&nl, &buffer->head[buffer->processed], sizeof(nl));

	*valp = (double)NTOH_uint64(nl) / (double)FLOAT_MULT;
	buffer->processed += sizeof(nl);
	return _SUCCESS;
}

/*
 * Given a 64-bit integer in host byte order, convert to network byte order
 * store in buffer, and adjust buffer counters.
 */
void pack64(uint64_t val, Buf buffer)
{
	uint64_t nl =  HTON_uint64(val);

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
	*valp = NTOH_uint64(nl);
	buffer->processed += sizeof(nl);
	return _SUCCESS;
}

/*
 * Given a 32-bit integer in host byte order, convert to network byte order
 * store in buffer, and adjust buffer counters.
 */
void pack32(uint32_t val, Buf buffer)
{
	uint32_t nl = htonl(val);

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
	*valp = ntohl(nl);
	buffer->processed += sizeof(nl);
	return _SUCCESS;
}

/* Given a *uint16_t, it will pack an array of size_val */
void pack16_array(const uint16_t * valp, uint32_t size_val, Buf buffer)
{
	uint32_t i = 0;

	pack32(size_val, buffer);

	for (i = 0; i < size_val; i++) {
		pack16(*(valp + i), buffer);
	}
}

/* Given a int ptr, it will unpack an array of size_val
 */
int unpack16_array(uint16_t ** valp, uint32_t * size_val, Buf buffer)
{
	uint32_t i = 0;

	if (unpack32(size_val, buffer))
		return _ERROR;

	*valp = malloc((*size_val) * sizeof(uint16_t));
	for (i = 0; i < *size_val; i++) {
		if (unpack16((*valp) + i, buffer))
			return _ERROR;
	}
	return _SUCCESS;
}

/* Given a *uint32_t, it will pack an array of size_val */
void pack32_array(const uint32_t * valp, uint32_t size_val, Buf buffer)
{
	uint32_t i = 0;

	pack32(size_val, buffer);

	for (i = 0; i < size_val; i++) {
		pack32(*(valp + i), buffer);
	}
}

/* Given a int ptr, it will unpack an array of size_val
 */
int unpack32_array(uint32_t ** valp, uint32_t * size_val, Buf buffer)
{
	uint32_t i = 0;

	if (unpack32(size_val, buffer))
		return _ERROR;

	*valp = malloc((*size_val) * sizeof(uint32_t));
	for (i = 0; i < *size_val; i++) {
		if (unpack32((*valp) + i, buffer))
			return _ERROR;
	}
	return _SUCCESS;
}

/*
 * Given a 16-bit integer in host byte order, convert to network byte order,
 * store in buffer and adjust buffer counters.
 */
void pack16(uint16_t val, Buf buffer)
{
	uint16_t ns = htons(val);

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
	*valp = ntohs(ns);
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

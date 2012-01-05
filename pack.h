/****************************************************************************\
	debug("%d", data_len);
 *  pack.h - definitions for lowest level un/pack functions. all functions
 *	utilize a Buf structure. Call init_buf, un/pack, and free_buf
 *****************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Kevin Tew <tew1@llnl.gov>, Morris Jette <jette1@llnl.gov>, et. al.
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

#ifndef _PACK_INCLUDED
#define _PACK_INCLUDED

#if HAVE_CONFIG_H
#  include "config.h"
#if HAVE_INTTYPES_H
#  include <inttypes.h>
#else  /* !HAVE_INTTYPES_H */
#  if HAVE_STDINT_H
#    include <stdint.h>
#  endif
#endif  /* HAVE_INTTYPES_H */
#else   /* !HAVE_CONFIG_H */
#include <stdint.h>
#endif  /* HAVE_CONFIG_H */

#include <assert.h>
#include <time.h>
#include <string.h>

#define BUF_MAGIC 0x42554545
#define BUF_SIZE 1024
#define MAX_BUF_SIZE ((uint32_t) 0xffff0000)	/* avoid going over 32-bits */
#define FLOAT_MULT 1000000

//if Bigendian
# define HTON_int64(x)	  ((int64_t)  (x))
# define NTOH_int64(x)	  ((int64_t)  (x))
# define HTON_uint64(x)	  ((uint64_t) (x))
# define NTOH_uint64(x)	  ((uint64_t) (x))

struct yuga_buf {
	uint32_t magic;
	char *head;
	uint32_t size;
	uint32_t processed;
};

typedef struct yuga_buf* Buf;
typedef struct yuga_buf iofw_buf_t;

#define get_buf_data(__buf)		(__buf->head)
#define get_buf_offset(__buf)		(__buf->processed)
#define set_buf_offset(__buf,__val)	(__buf->processed = __val)
#define remaining_buf(__buf)		(__buf->size - __buf->processed)
#define size_buf(__buf)			(__buf->size)

Buf	create_buf (char *data, int size);
void	free_buf(Buf my_buf);
Buf	init_buf(int size);
void    grow_buf (Buf my_buf, int size);
void	*xfer_buf_data(Buf my_buf);

void	packdata(void *data, size_t size, Buf buffer);
int	unpackdata(void *data, size_t size, Buf buffer);

void	packdata_array(const void *valp, int len, size_t size, Buf buffer);
int	unpackdata_array(void **valp, int *len, size_t size, Buf buffer);

void	pack64_array(const uint64_t *valp, uint64_t size_val, Buf buffer);
int	unpack64_array(uint64_t **valp, uint64_t* size_val, Buf buffer);

void	packmem(const char *valp, uint32_t size_val, Buf buffer);
int	unpackmem(char *valp, uint32_t *size_valp, Buf buffer);
int	unpackmem_ptr(char **valp, uint32_t *size_valp, Buf buffer);
int	unpackmem_malloc(char **valp, uint32_t *size_valp, Buf buffer);

void	packstr_array(char **valp, uint32_t size_val, Buf buffer);
int	unpackstr_array(char ***valp, uint32_t* size_val, Buf buffer);

void	packmem_array(char *valp, uint32_t size_val, Buf buffer);
int	unpackmem_array(char *valp, uint32_t size_valp, Buf buffer);

#define packstr(str,buf) do {				\
	uint32_t _size = 0;				\
	if((char *)str != NULL)				\
		_size = (uint32_t)strlen(str)+1;	\
        assert(_size == 0 || str != NULL);             	\
	assert(_size <= 0xffffffff);			\
	assert(buf->magic == BUF_MAGIC);		\
	packmem(str,(uint32_t)_size,buf);		\
} while (0)

#define unpackstr_ptr		                        \
        unpackmem_ptr

#define unpackstr_malloc	                        \
        unpackmem_malloc

#endif /* _PACK_INCLUDED */

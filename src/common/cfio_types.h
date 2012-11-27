/****************************************************************************
 *       Filename:  cfio_types.h
 *
 *    Description:  define data types in cfio
 *
 *        Version:  1.0
 *        Created:  08/20/2012 03:30:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include "netcdf.h"

#define IOFW_BYTE   NC_BYTE
#define IOFW_CHAR   NC_CHAR
#define IOFW_SHORT  NC_SHORT
#define IOFW_INT    NC_INT
#define IOFW_FLOAT  NC_FLOAT
#define IOFW_DOUBLE NC_DOUBLE

#define cfio_types_size(size, type) \
    do{				    \
    switch(type) {		    \
	case IOFW_BYTE :	    \
	    size = 1;		    \
	    break;		    \
	case IOFW_CHAR :	    \
	    size = 1;		    \
	    break;		    \
	case IOFW_SHORT :	    \
	    size = sizeof(short);   \
	    break;		    \
	case IOFW_INT :		    \
	    size = sizeof(int);	    \
	    break;		    \
	case IOFW_FLOAT :	    \
	    size = sizeof(float);   \
	    break;		    \
	case IOFW_DOUBLE :	    \
	    size = sizeof(double);  \
	    break;		    \
    }} while(0)

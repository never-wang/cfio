/****************************************************************************
 *       Filename:  debug.h
 *
 *    Description:  define for debug mask
 *
 *        Version:  1.0
 *        Created:  12/28/2011 03:24:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _DEBUG_H
#define _DEBUG_H

#define ENABLE_DEBUG

#define DEBUG_NONE ((uint32_t)0)
#define DEBUG_USER ((uint32_t)1 << 0)
#define DEBUG_IO ((uint32_t)1 << 1)

extern int debug_mask;

#ifdef ENABLE_DEBUG
/* try to avoid function call overhead by checking masks in macro */
#define debug(mask, format, f...)                  \
    if (debug_mask & mask)                         \
    {                                                     \
        printf("[%s, %d]: " format, __FILE__ , __LINE__ , ##f); \
    }                                                     
#else
#define debug(mask, format, f...) \
    do {} while(0)
#endif

#define set_debug_mask(mask) debug_mask = mask
#define add_debug_mask(mask) debug_mask |= mask

#define debug_mark(mask) debug(mask, "MARK\n")

#endif

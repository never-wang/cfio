/****************************************************************************
 *       Filename:  msg.c
 *
 *    Description:  implement for msg between IO process adn IO forwardinging process
 *
 *        Version:  1.0
 *        Created:  12/13/2011 03:11:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "msg.h"
#include "debug.h"
#include "times.h"
#include "map.h"
#include "cfio_error.h"
#include "define.h"

cfio_msg_t *cfio_msg_create()
{
    cfio_msg_t *msg;
    msg = malloc(sizeof(cfio_msg_t));
    if(NULL == msg)
    {
	return NULL;
    }
    memset(msg, 0, sizeof(cfio_msg_t));

    return msg;
}

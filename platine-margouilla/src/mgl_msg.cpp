/**********************************************************************
**
** Margouilla Runtime Library
**
**
** Copyright (C) 2002-2003 CQ-Software.  All rights reserved.
**
**
** This file is distributed under the terms of the GNU Library 
** General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.
**
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.margouilla.com
**********************************************************************/
/* $Id: mgl_msg.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else 
//#include <socket.h>
#include <netinet/in.h>
#endif

#include "mgl_msg.h"
#include "mgl_event.h"
 
long mgl_marshaller_to_buf_long(char *op_buf, long i_len, long i_val)
{
	long *lp_ptr;
	if (op_buf) {
		if (i_len>=4) {
			lp_ptr = (long *)op_buf;
			(*lp_ptr)=htonl(i_val);
		}
	}
	return 4;
}

long mgl_marshaller_to_struct_long(long *op_val, char *ip_buf, long i_len)
{
	long *lp_ptr;
	if (ip_buf) {
		if (i_len>=4) {
			lp_ptr = (long *)ip_buf;
			(*op_val)=ntohl((*lp_ptr));
		}
	}
	return 4;
}

long mgl_marshaller_to_buf_buffer(char *op_buf, long i_len, char *ip_buf, long i_buflen)
{
	if (op_buf) {
		if (i_len>=i_buflen) {
			memcpy(op_buf, ip_buf, i_buflen);
		}
	}
	return i_buflen;
}

long mgl_marshaller_to_struct_buffer(char *op_buf, long i_len, char *ip_buf, long i_buflen)
{
	if (op_buf) {
		if (i_len>=i_buflen) {
			memcpy(op_buf, ip_buf, i_buflen);
		}
	}
	return i_buflen;
}

long mgl_msginfo_struct_to_buf(char *op_buf, long *op_len, mgl_msginfo *ip_msginfo, mgl_marshaller_struct_to_buf *ip_fct_marshaller)
{
	char *lp_buf;
	long l_len;

	if (!(ip_msginfo&&op_buf&&op_len)) {
		return 0;
	}
	if (!(ip_msginfo->ptr)) {
		return 0;
	}
	if ((ip_msginfo->ptr->len>0)&&(ip_msginfo->ptr->pBuf==NULL)) {
		printf("Pb, non coherent msg: buf len=%ld, ptrlen=NULL\n", ip_msginfo->ptr->len);
		return 0;
	}

	// msg internal header
	l_len = 6*sizeof(long); 

	// msg marshaller
	if (ip_fct_marshaller) {
		l_len += ip_fct_marshaller(NULL, 0, ip_msginfo); // only get length
	}

	if ((*op_len)<l_len) {
		printf("Buffer too small %ld provided, %ld required\n", *op_len, l_len);
		return mgl_ko;
	}
	
	// Header
	lp_buf = op_buf;
	lp_buf += mgl_marshaller_to_buf_long(lp_buf, 4, ip_msginfo->srcBloc);
	lp_buf += mgl_marshaller_to_buf_long(lp_buf, 4, ip_msginfo->srcPort);
	lp_buf += mgl_marshaller_to_buf_long(lp_buf, 4, ip_msginfo->dstBloc);
	lp_buf += mgl_marshaller_to_buf_long(lp_buf, 4, ip_msginfo->dstPort);
	lp_buf += mgl_marshaller_to_buf_long(lp_buf, 4, ip_msginfo->ptr->type);
	lp_buf += mgl_marshaller_to_buf_long(lp_buf, 4, ip_msginfo->ptr->len);


	// Buffer
	if (ip_fct_marshaller) {
		lp_buf += ip_fct_marshaller(lp_buf, *op_len, ip_msginfo->ptr->pBuf); 
	} else {
		lp_buf += mgl_marshaller_to_buf_buffer(lp_buf, *op_len, (char *)ip_msginfo->ptr->pBuf, ip_msginfo->ptr->len); 
	}

	return l_len;
}

mgl_event *mgl_event_msginfo_buf_to_struct(char *ip_buf, long ip_len, mgl_marshaller_buf_to_struct *ip_fct_marshaller)
{ 
	mgl_event *lp_event;
	mgl_msg *lp_msg;
	long l_len;
	char *lp_buf;

	if (!ip_buf) { return NULL; }

	// msg internal header
	l_len = 6*sizeof(long); 
	if (ip_len<6) { return NULL; }

	// Allocate vent & msg
	lp_event = (mgl_event *)malloc(sizeof(mgl_event));
	lp_msg = (mgl_msg *)malloc(sizeof(mgl_msg));
	lp_event->type = mgl_event_type_msg;
	lp_event->event.msg.ptr=lp_msg;

	// Get fields
	lp_buf = ip_buf;
	lp_buf += mgl_marshaller_to_struct_long(&(lp_event->event.msg.srcBloc), lp_buf, ip_len); 
	lp_buf += mgl_marshaller_to_struct_long(&(lp_event->event.msg.srcPort), lp_buf, ip_len); 
	lp_buf += mgl_marshaller_to_struct_long(&(lp_event->event.msg.dstBloc), lp_buf, ip_len); 
	lp_buf += mgl_marshaller_to_struct_long(&(lp_event->event.msg.dstPort), lp_buf, ip_len); 
	lp_buf += mgl_marshaller_to_struct_long(&(lp_event->event.msg.ptr->type), lp_buf, ip_len); 
	lp_buf += mgl_marshaller_to_struct_long(&(lp_event->event.msg.ptr->len), lp_buf, ip_len); 

	// Buffer
	if (lp_event->event.msg.ptr->len==0) {
		lp_event->event.msg.ptr->pBuf=NULL;
	} else {
		lp_event->event.msg.ptr->pBuf = (char *)malloc(lp_event->event.msg.ptr->len);
		if (ip_fct_marshaller) {
			// Warning: size should be checked !
			lp_buf += ip_fct_marshaller((char *)lp_event->event.msg.ptr->pBuf, lp_buf, lp_event->event.msg.ptr->len); 
		} else {

			lp_buf += mgl_marshaller_to_struct_buffer((char *)lp_event->event.msg.ptr->pBuf, lp_event->event.msg.ptr->len, lp_buf, lp_event->event.msg.ptr->len); 
		}
	}
	return lp_event;
}





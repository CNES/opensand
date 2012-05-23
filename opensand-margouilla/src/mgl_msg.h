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
/* $Id: mgl_msg.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_MSG_H
#define MGL_MSG_H


#include "mgl_type.h"
#include "mgl_marshall.h"

// Description of the message: Its name and body length
// Stored once in the Msg definition file
// Used to dynamically allocated the correct body size
typedef struct {
	char *name;
	mgl_bool is_msgset;
	long buf_len;
	mgl_id msgset_id;
	mgl_marshaller_fct *encode;
	mgl_marshaller_fct *decode;
} mgl_msgdesc;



// The message: An Id and a body
typedef struct {
	long type;
	void *pBuf;
	long len;
	// Set freeBody to 1 if the eventMgr allocate and free it
	// Set freeBody to 0 if the bloc allocate and free it
	long freeBody;
} mgl_msg;

typedef struct {
	mgl_msg *ptr;
	mgl_id srcBloc;
	mgl_id srcPort;
	mgl_id dstBloc;
	mgl_id dstPort;
	long time_in;
	long time_out;
	long id;
} mgl_msginfo;


typedef long mgl_marshaller_struct_to_buf(char *op_buf, long i_len, void *ip_struct);

typedef long mgl_marshaller_buf_to_struct(char *op_struct, char *ip_buf, long ip_len);

long mgl_msginfo_struct_to_buf(char *op_buf, long *op_len, mgl_msginfo *ip_struct, mgl_marshaller_struct_to_buf *ip_fct_marshaller);

typedef long mgl_msg_id;


#endif

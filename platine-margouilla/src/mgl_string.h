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
/* $Id: mgl_string.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef MGL_STRING_H
#define MGL_STRING_H


/****** System Includes ******/
/****** Application Includes ******/
#include "mgl_type.h"


class mgl_string {
public:
	char *ptrBuf;
	long strLen;
	long bufLen;
	mgl_string();
	mgl_string(const char *ip_string);
	~mgl_string();

	long set(const char *ip_string);
	long append(const char *ip_string);
	long append(long i_val);
	mgl_string& operator  = (const char *ip_string);
	mgl_string& operator  = (mgl_string &ip_string);
	int cmp(char *ip_string);
	int cmp(mgl_string &ip_string);
	const char *get();
	char get(long i_index);
	long getLength();
};



#endif



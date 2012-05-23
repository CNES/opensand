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
/* $Id: mgl_filename.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


/****** System Includes ******/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****** Application Includes ******/
#include "mgl_filename.h"
#include "mgl_debug.h"


mgl_status mgl_filename::getFileName(char *op_name, long i_len)
{
	long l_len;
	long l_cpt;

	// Set name to ""
	op_name[0]=0;

	// Check pointers
	if ((!ptrBuf)||(!op_name)) { 
		return mgl_ko; 
	}
	// Find last / 
	for (l_cpt=strLen-1; l_cpt>=0; l_cpt--) {
		if ((ptrBuf[l_cpt]=='/')||(ptrBuf[l_cpt]=='\\')) {
			l_cpt++;
			break;
		}
	}
	if (l_cpt<0) { l_cpt=0; }
	// Check dest buffer size
	l_len = strLen-l_cpt;
	if (l_len>=i_len) {
		return mgl_ko; 
	}
	// Copy file
	strcpy(op_name, ptrBuf+l_cpt);
	return mgl_ok; 
}


mgl_status mgl_filename::getFilePath(char *op_path, long i_len)
{
	long l_len;
	long l_start;
	long l_size;

	// Check pointers
	if ((!ptrBuf)||(!op_path)) { 
		return mgl_ko; 
	}

	// find start of file name
	l_len = strLen;
	for (l_start=l_len; l_start>0; l_start--) {
		if ((ptrBuf[l_start]=='/')||(ptrBuf[l_start]=='\\')) {
			l_start++;
			break;
		}
	}

	// Check dest buffer size
	l_size = l_start;
	if (l_size>=i_len) {
		MGL_WARNING(MGL_CTX, "Warning, truncating string\n");
		l_size=i_len; 
	}

	// Copy file
	strncpy(op_path, ptrBuf, l_size);
	op_path[l_size]=0;
	return mgl_ok; 
}

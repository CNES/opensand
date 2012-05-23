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
/* $Id: mgl_filename.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef MGL_FILENAME_H
#define MGL_FILENAME_H


/****** System Includes ******/
/****** Application Includes ******/
#include "mgl_string.h"


class mgl_filename : public mgl_string {
public:
	mgl_status getFileName(char *op_path, long i_len);
	mgl_status getFilePath(char *op_path, long i_len);
	mgl_status deleteFileExtension() {
		long l_len;
		long l_start;

		// Check pointers
		if (!ptrBuf) { 
			return mgl_ko; 
		}

		// find start of extension
		l_len = strlen(ptrBuf);
		for (l_start=l_len; l_start>0; l_start--) {
			if (ptrBuf[l_start]=='.') {
				ptrBuf[l_start]=0;
				break;
			}
		}
		return mgl_ok; 
	};
	char *getExtension() {
		long l_len;
		long l_start;
		static char noext[]="";

		// Check pointers
		if (!ptrBuf) { 
			return NULL; 
		}

		// find start of extension
		l_len = strlen(ptrBuf);
		for (l_start=l_len; l_start>0; l_start--) {
			if (ptrBuf[l_start]=='.') {
				return (ptrBuf+l_start+1);
			}
		}
		return noext; 
	}
	mgl_bool isCSource() {
		char *lp_ext;
		lp_ext = getExtension();
		if (!lp_ext) {
			return mgl_false;
		}
		if (strcmp(lp_ext, "c")==0) { return mgl_true; }
		if (strcmp(lp_ext, "cpp")==0) { return mgl_true; }
		if (strcmp(lp_ext, "c++")==0) { return mgl_true; }
		if (strcmp(lp_ext, "cxx")==0) { return mgl_true; }
		return mgl_false;
	}
	mgl_bool isCHeader() {
		char *lp_ext;
		lp_ext = getExtension();
		if (!lp_ext) {
			return mgl_false;
		}
		if (strcmp(lp_ext, "h")==0) { return mgl_true; }
		if (strcmp(lp_ext, "hpp")==0) { return mgl_true; }
		if (strcmp(lp_ext, "h++")==0) { return mgl_true; }
		if (strcmp(lp_ext, "hxx")==0) { return mgl_true; }
		return mgl_false;
	}
};



#endif



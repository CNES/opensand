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
/* $Id: mgl_type.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_TYPE_H
#define MGL_TYPE_H

#include <string.h>
#include <stdio.h>

typedef long mgl_id;

typedef enum {
	mgl_ko,
	mgl_ok
} mgl_status;

typedef enum {
	mgl_false,
	mgl_true
} mgl_bool;

class mgl_rgb {
public:
	char _buf[64];
	long r;
	long g;
	long b;
	mgl_rgb() { r=0; g=0; b=0; }
	mgl_rgb(long i_r, long i_g, long i_b) { r=i_r; g=i_g; b=i_b; }
	void set(long i_r, long i_g, long i_b) { r=i_r; g=i_g; b=i_b; }
	long set(char *ip_buf) { 
		//SEb: Please: upgrade !!!!
		sscanf(ip_buf, "%ld,%ld,%ld", &r, &g, &b); 
		return 0; 
	}
	char *get() { sprintf(_buf, "%ld,%ld,%ld", r, g, b); return _buf; }
	long getR() { return r; }
	long getG() { return g; }
	long getB() { return b; }
};



typedef char mgl_string128[128];
mgl_status mgl_str128cpy(char *op_dest, const char *ip_src);

typedef char mgl_string1024[1024];
mgl_status mgl_str1024cpy(char *op_dest, const char *ip_src);

long mgl_min(long i_a, long i_b);
long mgl_max(long i_a, long i_b);


#endif


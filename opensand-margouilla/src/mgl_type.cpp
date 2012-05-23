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
/* $Id: mgl_type.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#include "mgl_type.h"

mgl_status mgl_str128cpy(char *op_dest, const char *ip_src) 
{
		if (!op_dest) { 
			return mgl_ko; 
		} 
		if (!ip_src) { 
			return mgl_ko; 
		} 
		strncpy(op_dest, ip_src, 127); 
		return mgl_ok; 
}

mgl_status mgl_str1024cpy(char *op_dest, const char *ip_src) 
{
		if (!op_dest) { 
			return mgl_ko; 
		} 
		if (!ip_src) { 
			return mgl_ko; 
		} 
		strncpy(op_dest, ip_src, 1023); 
		return mgl_ok; 
}


long mgl_min(long i_a, long i_b) 
{ 
	if (i_a<i_b) return i_a; else return i_b; 
}


long mgl_max(long i_a, long i_b) 
{ 
	if (i_a>i_b) return i_a; else return i_b; 
}




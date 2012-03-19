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
/* $Id: mgl_font.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


#ifndef MGL_FONT_H
#define MGL_FONT_H


/****** System Includes ******/
/****** Application Includes ******/
#include "mgl_string.h"


class mgl_font {
public:
	char _buf[1024];
	mgl_string _family;
	long _size;
	long _weight;
	mgl_bool _italic;

	mgl_font() { _family="Arial"; _size=10; _weight=50; _italic=mgl_false; }
	mgl_font(char *ip_family, long i_size=10, long i_weight=50, mgl_bool i_italic=mgl_false) 
		{ _family=ip_family; _size=i_size; _weight=i_weight; _italic=i_italic; }
	void set(char *ip_family, long i_size, long i_weight, mgl_bool i_italic)
		{ _family=ip_family; _size=i_size; _weight=i_weight; _italic=i_italic; }
	long set(char *ip_buf) { 
		char l_buf[1024]="";
		// Seb: To upgrade
		sscanf(ip_buf, "%d,%d,%d,%s", &_size, &_weight, &_italic, l_buf);
		_family.set(l_buf);
		return 0; 
	}
	char *get() { sprintf(_buf, "%d,%d,%d,%s", _size, _weight, _italic, _family.get()); return _buf; }
	char *getFamily() { return _family.get(); }
	long getSize() { return _size; }
	long getWeight() { return _weight; }
	mgl_bool getItalic() { return _italic; }
};



#endif



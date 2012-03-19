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
/* $Id: mgl_obj.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_OBJ_H
#define MGL_OBJ_H

#include <string.h>



class mgl_obj {
public:
	mgl_string128 _name;
	mgl_id _id;

	mgl_obj() { _id=-1; _name[0]=0; }
	mgl_status setName(const char *ip_name) { return mgl_str128cpy(_name, ip_name); };
	const char *getName() { return _name; };
	mgl_status setId(mgl_id i_id) {_id = i_id; return mgl_ok; };
	mgl_id getId() { return _id; };
};

#endif


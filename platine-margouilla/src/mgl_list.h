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
/* $Id: mgl_list.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */





#ifndef MGL_LIST_H
#define MGL_LIST_H

#include "mgl_type.h"


typedef mgl_bool mgl_sort_fct(void *ip_data1, void *ip_data2);

class mgl_list {
public:
	char *ptr;
	long size;
	long count;
	long increment;
	mgl_list();
	~mgl_list();

	void init();
	mgl_status adjustSize(long i_index);
	long append(void *ip_ptr_data);
	long appendSorted(void *ip_ptr_data, mgl_sort_fct *fct);
	long prepend(void *ip_ptr_data);
	long insert(long i_pos, void *ip_ptr_data);
	void *removeByPtr(void *ip_ptr_data);
	void *remove(long i_pos);
	void *get(long i_pos);
	long getIndexByPtr(void *ip_ptr_data);
	long getCount();
	mgl_status clear();

	mgl_status swap(long i_pos1, long i_pos2);
	mgl_status sort(mgl_sort_fct *fct);

};

#endif


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
/* $Id: mgl_fifo.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */





#ifndef MGL_FIFO_H
#define MGL_FIFO_H

#include "mgl_type.h"

typedef mgl_bool mgl_fifo_sort_fct(void *ip_data1, void *ip_data2);


class mgl_fifo {
public:
	void **ptr;
	void **ptr_end;
	void **first;
	void **last;
	long size;
	long count;
	mgl_fifo();
	~mgl_fifo();

	int init(long i_size);
	long append(void *ip_ptr_data);
	long appendSorted(void *ip_ptr_data, mgl_fifo_sort_fct *fct);
	void *get(); // first element
	void *get(long i_index); 
	void *remove();
	long getCount();
	mgl_status set(long i_index, void *ip_ptr_data);

	mgl_status swap(long i_pos1, long i_pos2);
	mgl_status sort(mgl_fifo_sort_fct *fct);

	void quick_sort(mgl_fifo_sort_fct *fct, int i_start_index, int i_nb);
	int  quick_sort_partition(mgl_fifo_sort_fct *fct, int i_start_index, int i_nb);
	
};

#endif


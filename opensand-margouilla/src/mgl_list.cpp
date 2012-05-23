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
/* $Id: mgl_list.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


/* Margouilla Lib: list definition */



#include "mgl_list.h"
#include <stdlib.h>


#define mgl_list_element_size	sizeof(void *)
mgl_list::mgl_list()
{
	init();
}

void mgl_list::init()
{
	ptr=NULL;
	size=0;
	count=0;
	increment=32;
}
mgl_list::~mgl_list()
{
	if ((size>0)&&(ptr!=NULL)) {
		free(ptr);
	}
}

mgl_status mgl_list::adjustSize(long i_index)
{
	long l_newsize;
	char *lp_ptr;
	if (i_index<=size) {
			return mgl_ok;
	}
	// allocate new memory
	l_newsize = (i_index/increment+1)*increment;
	lp_ptr = (char *)malloc(l_newsize*mgl_list_element_size);
	if (lp_ptr) {
		if (ptr) {
			memcpy(lp_ptr, ptr, size*mgl_list_element_size);
			free(ptr);
		}
		ptr = lp_ptr;
		size = l_newsize;
		return mgl_ok;
	}
	return mgl_ko;
}

long mgl_list::append(void *ip_ptr_data)
{
	mgl_status l_ret;
	void **lp_ptr;
	l_ret = adjustSize(count+1);
	if (l_ret==mgl_ko) { return -1; }
	lp_ptr = (void **)(ptr+(count*mgl_list_element_size));
	(*lp_ptr)=ip_ptr_data;
	count++;
	return count;
}


long mgl_list::appendSorted(void *ip_ptr_data, mgl_sort_fct *fct)
{
	long l_ret;
	l_ret = append(ip_ptr_data);
	sort(fct);
	return l_ret;
}


long mgl_list::prepend(void *ip_ptr_data)
{
	return ++count;
}


long mgl_list::insert(long i_pos, void *ip_ptr_data)
{
	return ++count;
}

void *mgl_list::removeByPtr(void *ip_ptr_data)
{
	long l_cpt;
	void *lp_ptr_data;
	for (l_cpt=0; l_cpt<count; l_cpt++) {
		lp_ptr_data = get(l_cpt);
		if (lp_ptr_data==ip_ptr_data) {
			return remove(l_cpt);
		}
	}
	return NULL;
}

void *mgl_list::remove(long i_pos)
{
	void **lp_ptr;
	void **lp_ptr_next;
	void *lp_ptr_data;
	long l_cpt;

	if (count==0) { return NULL; }
	if (i_pos>count) { return NULL; }
	if (i_pos<0) { return NULL; }
	lp_ptr = (void **)(ptr+(i_pos*mgl_list_element_size));
	lp_ptr_data = (*lp_ptr);
	lp_ptr_next = lp_ptr; lp_ptr_next++;
	for (l_cpt=0; l_cpt<(count-1-i_pos); l_cpt++) {
		(*lp_ptr) = (*lp_ptr_next);
		lp_ptr++;
		lp_ptr_next++;
	}
	(*lp_ptr)=NULL;
	count--;
	return lp_ptr_data;
}

void *mgl_list::get(long i_pos)
{
	void **lp_ptr;

	if (i_pos>=count) { return NULL; }
	if (i_pos<0) { return NULL; }
	lp_ptr = (void **)(ptr+(i_pos*mgl_list_element_size));
	return (*lp_ptr);
}

long mgl_list::getIndexByPtr(void *ip_ptr)
{
	long l_cpt;
	void *lp_ptr;

	for (l_cpt=0; l_cpt<count; l_cpt++) {
		lp_ptr = get(l_cpt);
		if (lp_ptr==ip_ptr) {
			return l_cpt;
		}
	}
	return -1;
}


long mgl_list::getCount() {
	return count;
}

mgl_status mgl_list::clear()
{
	count=0;
	return mgl_ok;
}


mgl_status mgl_list::swap(long i_pos1, long i_pos2)
{
	void **lp_ptr1;
	void **lp_ptr2;
	void *lp_ptr;

	if (i_pos1<0) { return mgl_ko; }
	if (i_pos2<0) { return mgl_ko; }
	if (i_pos1>=count) { return mgl_ko; }
	if (i_pos2>=count) { return mgl_ko; }
	lp_ptr1 = (void **)(ptr+(i_pos1*mgl_list_element_size));
	lp_ptr2 = (void **)(ptr+(i_pos2*mgl_list_element_size));
	lp_ptr = (*lp_ptr2);
	(*lp_ptr2) = (*lp_ptr1);
	(*lp_ptr1) = lp_ptr;
	return mgl_ok;
}


mgl_status mgl_list::sort(mgl_sort_fct *fct)
{
	long l_cpt1;
	long l_cpt2;
	long l_nb;
	void *lp_data1;
	void *lp_data2;

	l_nb = getCount();
	for (l_cpt1=0; l_cpt1<l_nb; l_cpt1++) {
		lp_data1 = get(l_cpt1);
		for (l_cpt2=l_cpt1+1; l_cpt2<l_nb; l_cpt2++) {
			lp_data2 = get(l_cpt2);
			if ((*fct)(lp_data1, lp_data2)) {
				swap(l_cpt1, l_cpt2);
				lp_data1 = lp_data2;
			}
		}
	}
	return mgl_ok;
}


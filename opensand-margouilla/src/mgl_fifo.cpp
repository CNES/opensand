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
/* $Id: mgl_fifo.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

/* Margouilla Lib: fifo definition */



#include "mgl_fifo.h"
#include <stdlib.h>


#define mgl_fifo_element_size	sizeof(void *)


mgl_fifo::mgl_fifo()
{
	ptr     = NULL;
	ptr_end	= NULL;
	first = NULL;
	last  = NULL;
	count = 0;
	size  = 0;
}


mgl_fifo::~mgl_fifo()
{
	if (ptr) { free(ptr); }
}


int mgl_fifo::init(long i_size)
{
	if (ptr) { free(ptr); }
	ptr     = (void **)malloc(mgl_fifo_element_size*i_size);
	memset(ptr, 0, mgl_fifo_element_size*i_size);
	ptr_end	= (void **)(ptr+i_size-1);
	first = (void **)ptr;
	last  = (void **)ptr;
	count = 0;
	size  = i_size;
	return 0;
}

long mgl_fifo::append(void *ip_ptr_data)
{
	if (!ip_ptr_data) { 
		return -1; 
	}
	if (count==size) {
		return -1; // Pb, fifo full
	}
	if (count>0) {
		last++;
		if (last>ptr_end) { last=ptr; }
	}
	(*last) = ip_ptr_data;
	count++;
	return count;
}

long mgl_fifo::appendSorted(void *ip_ptr_data, mgl_fifo_sort_fct *fct)
{
	long l_ret;
	l_ret = append(ip_ptr_data);
	sort(fct);
	return l_ret;
}

void *mgl_fifo::get()
{
	void *l_ptr;
	if (count<=0) {
		return NULL; // Pb, fifo empty
	}
	l_ptr = (*first);
	return l_ptr;
}

void *mgl_fifo::get(long i_index)
{
	void **lp_ref;
	void *l_ptr;
	long l_dist;

	if ((count<=0)||(i_index<0)||(i_index>=count)) {
		return NULL; // Pb,
	}
	// Distance between first and end of circular buffer
	l_dist= (ptr_end-first)+1;
	if (i_index<l_dist) {
		lp_ref = first+i_index;
	} else {
		lp_ref = ptr+(i_index-l_dist);
	}
	l_ptr = (*lp_ref);
	return l_ptr;
}

mgl_status mgl_fifo::set(long i_index, void *ip_ptr_data)
{
	void **lp_ref;
	long l_dist;

	if ((count<=0)||(i_index<0)||(i_index>=count)) {
		return mgl_ko; // Pb,
	}
	// Distance between first and end of circular buffer
	l_dist= (ptr_end-first)+1;
	if (i_index<l_dist) {
		lp_ref = first+i_index;
	} else {
		lp_ref = ptr+(i_index-l_dist);
	}
	(*lp_ref) = ip_ptr_data;
	return mgl_ok;;
}


void *mgl_fifo::remove()
{
	void *l_ptr;
	if (count<=0) {
		return NULL; // Pb, fifo empty
	}
	l_ptr = (*first);
	(*first)=0;
	if (count>1) {
		first++;
		if (first>ptr_end) { first=ptr; }
	}
	count--;
	return l_ptr;
}


long mgl_fifo::getCount()
{
	return count;
}




mgl_status mgl_fifo::swap(long i_pos1, long i_pos2)
{
	void *lp_ptr1;
	void *lp_ptr2;

	if (i_pos1<0) { return mgl_ko; }
	if (i_pos2<0) { return mgl_ko; }
	if (i_pos1>=count) { return mgl_ko; }
	if (i_pos2>=count) { return mgl_ko; }

	lp_ptr1 = get(i_pos1);
	lp_ptr2 = get(i_pos2);
	set(i_pos1, lp_ptr2);
	set(i_pos2, lp_ptr1);

	return mgl_ok;
}


void dump(mgl_fifo &fifo) 
{
	long l_cpt;
	long l_nb;
	printf("\n");
	l_nb = fifo.getCount();
	for (l_cpt=0; l_cpt<l_nb; l_cpt++) {
		printf("%d, ", (int)*(fifo.ptr+l_cpt));
	}
	printf("\n");

}

void mgl_fifo::quick_sort(mgl_fifo_sort_fct *ip_fct, int i_start_index, int i_nb)
{       
	int pivotpos;
	if(i_nb<=1) { return; }
	pivotpos=quick_sort_partition(ip_fct, i_start_index, i_nb);
	quick_sort(ip_fct, i_start_index,pivotpos);
	quick_sort(ip_fct, i_start_index+pivotpos+1,i_nb-pivotpos-1);
}


int mgl_fifo::quick_sort_partition(mgl_fifo_sort_fct *ip_fct, int i_start_index, int i_nb)
{       
	void *pivot;
	int left,right;
	pivot=get(i_start_index); // First element is pivot

	left=i_start_index; right=i_start_index+i_nb-1;
	for( ; ; )
	 {
		 while((left<right) && ((*ip_fct)(get(left), pivot)==mgl_false)) { left++; }
		 while((left<right) && ((*ip_fct)(get(right),pivot)==mgl_true)) { right--; }

		 if(left==right)
		 {
			 if(((*ip_fct)(get(right),pivot)==mgl_true)) { left=left-1; }
			 break;
		 }
		 swap(left, right);
	 }
	swap(i_start_index, left);
	dump(*this);
    return left;
}


mgl_status mgl_fifo::sort(mgl_fifo_sort_fct *ip_fct)
{
	long l_cpt1;
	long l_cpt2;
	long l_nb;
	void *lp_data1;
	void *lp_data2;

	l_nb = getCount();

	// Quick sort
	//quick_sort(ip_fct, 0, l_nb);
	//return mgl_ok;

	// Bubble sort
	for (l_cpt1=0; l_cpt1<l_nb; l_cpt1++) {	
		lp_data1 = get(l_cpt1);
		for (l_cpt2=l_cpt1+1; l_cpt2<l_nb; l_cpt2++) {
			lp_data2 = get(l_cpt2);
			if ((*ip_fct)(lp_data1, lp_data2)) {
				swap(l_cpt1, l_cpt2);
				lp_data1 = lp_data2;
				//dump(*this);
			}
		}
	}
	return mgl_ok;
}


mgl_bool fifo_sort_fct(void *ip_data1, void *ip_data2)
{
	if (ip_data1>=ip_data2) {
		return mgl_true;
	} else {
		return mgl_false;
	}
}


void test_fifo()
{
	mgl_fifo l_fifo;

	l_fifo.init(6);
	l_fifo.append((void *)1);
	l_fifo.append((void *)5);
	l_fifo.append((void *)3);
	l_fifo.remove();
	l_fifo.remove();
	l_fifo.remove();
	l_fifo.append((void *)1);
	l_fifo.append((void *)5);
	l_fifo.append((void *)3);
	l_fifo.append((void *)7);
	l_fifo.append((void *)2);
	l_fifo.append((void *)4);
	l_fifo.sort(&fifo_sort_fct);
	while (l_fifo.getCount()>0) {
		printf("%d\n", (int)l_fifo.remove());
	}
	// Result should be 1, 2, 3, 4, 5, 7
}



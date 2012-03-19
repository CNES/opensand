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
// V2.0
// modif by ASP for Platine 29.10.04 
// --> update release() in order to stop in critical error if count > nb_blocs

/* $Id: mgl_memorypool.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


#include <stdlib.h>
#include <assert.h>


#include "config.h"
#include "mgl_memorypool.h"


mgl_memory_pool::mgl_memory_pool(long i_use_mutex)
{
	_memBlocSize=0;
	_nbBlocs=0;
	_useMutex=i_use_mutex;
	if (i_use_mutex) { mgl_mutex_init(&_mutex); }
#if DEBUG_MEMORY
	this->allocated = 0;
	this->used = 0;
	this->max_used = 0;
#endif
}


mgl_memory_pool::mgl_memory_pool(long i_bloc_size, long i_nb_blocs, const char *ip_name, long i_use_mutex)
{
#if MEMORY_POOL
	_useMutex=i_use_mutex;
	setName(ip_name);
	allocate(i_bloc_size, i_nb_blocs);
 #if DEBUG_MEMORY
	this->allocated = i_nb_blocs;
	this->used = 0;
	this->max_used = 0;
 #endif
#else
	this->_memBlocSize = i_bloc_size;
	this->_nbBlocs = i_nb_blocs;
#endif
}



mgl_memory_pool::~mgl_memory_pool()
{
#if MEMORY_POOL
 #if DEBUG_MEMORY
	std::map<char *,list_string *>::iterator it;
	std::list<std::string>::iterator it_string;
 #endif

	// If needed free already allocated memory
	freeAll();

 #if DEBUG_MEMORY
	// warn about memory leaks
	if(this->_used_memory.size() > 0)
	{
		printf("memory pool '%s' leaked %d memory segments:\n",
		       this->_name, this->_used_memory.size());

		for(it = this->_used_memory.begin(); it != this->_used_memory.end(); it++)
		{
			printf("\tsegment %p was lost:\n", it->first);
			for(it_string = (*it).second->begin();
			    it_string != (*it).second->end(); it_string++)
			{
				printf("\t\tsegment %p was in function '%s'\n",
				       it->first, it_string->c_str());
			}
			delete it->second;
		}
	}
	printf("memory pool '%s' used at max %u of %ld segments\n", this->_name,
	       this->max_used, this->allocated);
 #endif
#endif
}

void mgl_memory_pool::setName(const char *ip_name)
{
	bzero(this->_name, sizeof(this->_name));
	strncpy(this->_name, ip_name, sizeof(this->_name)-1);
}


mgl_status mgl_memory_pool::allocate(long i_bloc_size, long i_nb_blocs)
{
#if MEMORY_POOL
	char *lp_ptr;
	long l_cpt;

	// If needed free already allocated memory
	freeAll();

	mutexLock();
	// Store values
	_memBlocSize=i_bloc_size;
	_nbBlocs=i_nb_blocs;
	
	// Adjust lists length
	_available_memory_list.adjustSize(i_nb_blocs+1);

	// Allocate new memory
	for (l_cpt=0; l_cpt<_nbBlocs; l_cpt++) {
		// Allocate memory
		lp_ptr = (char *)malloc(i_bloc_size+2*4);
		if (!lp_ptr) { 
			mutexUnlock();
			return mgl_ko;
			// Out of memory
		}

		// Set magic number at beginning and end
		lp_ptr[0]='B';
		lp_ptr[1]='B';
		lp_ptr[2]='B';
		lp_ptr[3]='B';
		lp_ptr[4+i_bloc_size+0]='B';
		lp_ptr[4+i_bloc_size+1]='B';
		lp_ptr[4+i_bloc_size+2]='B';
		lp_ptr[4+i_bloc_size+3]='B';

		// Store pointer
		_available_memory_list.append(lp_ptr);
	}
	mutexUnlock();
#else
	this->_memBlocSize = i_bloc_size;
	this->_nbBlocs = i_nb_blocs;
#endif

	return mgl_ok;
}


char *mgl_memory_pool::get(std::string descr, size_t size)
{
 #if DEBUG_MEMORY_MORE
	printf("memory pool '%s' was asked for %u bytes\n", this->_name, size);
 #endif

	return this->get(descr);
}


char *mgl_memory_pool::get(std::string descr)
{
#if MEMORY_POOL
	char *lp_ptr;
	long l_count;
	static long l_min_count = 600000;
	long l_flag=0;
	char **lp_buf_ref;
	char *lp_buf;

	mutexLock();

	l_count = _available_memory_list.getCount();
	if (l_count<=0) { 
		printf("mgl_memory_pool(%s)::get('%s'): Out of memory\n",
		       _name, descr.c_str());
		mutexUnlock();
		return NULL; 
	}
	if (l_count<l_min_count) { l_min_count=l_count; l_flag=1; }
	lp_ptr = (char *)_available_memory_list.remove(l_count-1);
	if (lp_ptr!=0) { 
		this->setMagicAllocated(lp_ptr);
 #if DEBUG_MEMORY
		list_string * _list_string = new std::list<std::string>;
		_list_string->push_front(descr);
		this->_used_memory.insert(std::make_pair(lp_ptr, _list_string));
		this->used++;
		if(this->used > this->max_used)
			this->max_used = this->used;
 #endif
		lp_ptr+=4; 
		bzero(lp_ptr, _memBlocSize);
	} else {
		// Warning out of memory
		lp_buf_ref = (char **)_available_memory_list.ptr;
		lp_buf=lp_buf_ref[l_count];
		printf("memory pool '%s': return NULL pointer for allocation "
		       "request at %s\n", this->_name, descr.c_str());
	}
	mutexUnlock();
	return lp_ptr;
#else
	return (char *)malloc(this->_memBlocSize);
#endif
}



mgl_status mgl_memory_pool::release(char *ip_ptr_data)
{
#if MEMORY_POOL
	char *lp_ptr;
	// ASP for Platine 29.10.04 -- begin
	int count = 0;
	// ASP for Platine 29.10.04 -- end


	if (!ip_ptr_data) { 
		return mgl_ko; 
	}
	mutexLock();

	lp_ptr = ip_ptr_data-4; // Jump over magic number
	if (!checkMagicAllocated(lp_ptr)) {
		// Bad pointer or corrupted memory
 #if DEBUG_MEMORY
		std::map<char *, list_string* >::iterator it;
		it = this->_used_memory.find(lp_ptr);
		if(it != this->_used_memory.end())
			printf("mgl_memory_pool(%s)::release(): segment 0x%x "
			       "allocated at '%s' corrupted\n", _name,
			       (*it).first, (*((*it).second->begin())).c_str());
		else
			printf("mgl_memory_pool(%s)::release(): unknown segment 0x%x\n",
			       _name, lp_ptr);
		assert(0);
 #else
		printf("mgl_memory_pool(%s)::release():  Warning Releasing corrupted memory\n", _name);
 #endif

		mutexUnlock();
		return mgl_ko;
	}
	this->setMagicFreed(lp_ptr);
	// ASP for Platine 29.10.04 -- begin
	//_available_memory_list.append(lp_ptr);
	count = _available_memory_list.append(lp_ptr);
	if (count > _nbBlocs) {
		// Critical: bad management of memory pool
		fprintf(stderr,"\n [mgl_memorypool] Name %s. Nb of available blocs after release (%d) is larger than at init (%ld). See log file\n",
			_name, count, _nbBlocs);
		exit(1);
	}

 #if DEBUG_MEMORY
	std::list<std::string>::iterator it_string;
	std::map<char *, list_string *>::iterator it;
	it = this->_used_memory.find(lp_ptr);
	if(it != this->_used_memory.end())
	{
		printf("pool '%s': segment %p released:\n", this->_name, lp_ptr);
		for(it_string = (*it).second->begin(); it_string != (*it).second->end(); it_string++)
                {
                        printf("\tfunction '%s'\n", it_string->c_str());
                }
                delete (*it).second;
		
		this->_used_memory.erase(it);
		this->used--;
	}
 #endif

	// ASP for Platine 29.10.04 -- end
	mutexUnlock();
#else
	free(ip_ptr_data);
#endif
	return mgl_ok;
}

mgl_status mgl_memory_pool::freeAll()
{
	char *lp_ptr;
	long l_cpt;
	long l_nb;

	mutexLock();
	// Reset values
	_memBlocSize=0;
	_nbBlocs=0;
	
	// Free available memory
	l_nb = _available_memory_list.getCount();
	for (l_cpt=(l_nb-1); l_cpt>=0; l_cpt--) {
		lp_ptr = (char *)_available_memory_list.remove(l_cpt);
		if (lp_ptr) { free(lp_ptr);}
	}
	mutexUnlock();
	return mgl_ok;
}

mgl_status mgl_memory_pool::checkMemory()
{
	char *lp_ptr;
	long l_cpt;

	mutexLock();
	// Check available memory
	for (l_cpt=_available_memory_list.getCount(); l_cpt>=0; l_cpt--) {
		lp_ptr = (char *)_available_memory_list.get(l_cpt);
		if (lp_ptr) { 
			if (!checkMagicAllocated(lp_ptr) &&
			    !checkMagicFreed(lp_ptr)) { 
				mutexUnlock();
				return mgl_ko; 
			}
		}
	}
	mutexUnlock();
	return mgl_ok;
}

mgl_status mgl_memory_pool::setMagicAllocated(char *ip_ptr)
{
	if (!ip_ptr) { return mgl_ko; }

	// set magic number at beginning and end
	ip_ptr[0] = 'A';
	ip_ptr[1] = 'A';
	ip_ptr[2] = 'A';
	ip_ptr[3] = 'A';
	ip_ptr[4+_memBlocSize+0] = 'A';
	ip_ptr[4+_memBlocSize+1] = 'A';
	ip_ptr[4+_memBlocSize+2] = 'A';
	ip_ptr[4+_memBlocSize+3] = 'A';

	return mgl_ok;
}

mgl_status mgl_memory_pool::checkMagicAllocated(char *ip_ptr)
{
	if (!ip_ptr) { return mgl_ko; }

	// Check magic number at beginning and end
	if (
		  	  (ip_ptr[0]=='A')
			&&(ip_ptr[1]=='A')
			&&(ip_ptr[2]=='A')
			&&(ip_ptr[3]=='A')
			&&(ip_ptr[4+_memBlocSize+0]=='A')
			&&(ip_ptr[4+_memBlocSize+1]=='A')
			&&(ip_ptr[4+_memBlocSize+2]=='A')
			&&(ip_ptr[4+_memBlocSize+3]=='A')
		) {
		return mgl_ok;
	}
	return mgl_ko;
}

mgl_status mgl_memory_pool::setMagicFreed(char *ip_ptr)
{
	if (!ip_ptr) { return mgl_ko; }

	// set magic number at beginning and end
	ip_ptr[0] = 'B';
	ip_ptr[1] = 'B';
	ip_ptr[2] = 'B';
	ip_ptr[3] = 'B';
	ip_ptr[4+_memBlocSize+0] = 'B';
	ip_ptr[4+_memBlocSize+1] = 'B';
	ip_ptr[4+_memBlocSize+2] = 'B';
	ip_ptr[4+_memBlocSize+3] = 'B';

	return mgl_ok;
}

mgl_status mgl_memory_pool::checkMagicFreed(char *ip_ptr)
{
	if (!ip_ptr) { return mgl_ko; }

	// Check magic number at beginning and end
	if (
		  	  (ip_ptr[0]=='B')
			&&(ip_ptr[1]=='B')
			&&(ip_ptr[2]=='B')
			&&(ip_ptr[3]=='B')
			&&(ip_ptr[4+_memBlocSize+0]=='B')
			&&(ip_ptr[4+_memBlocSize+1]=='B')
			&&(ip_ptr[4+_memBlocSize+2]=='B')
			&&(ip_ptr[4+_memBlocSize+3]=='B')
		) {
		return mgl_ok;
	}
	return mgl_ko;
}

void mgl_memory_pool::mutexLock()
{
	if (_useMutex) {
		mgl_mutex_lock(&_mutex);
	}
}


void mgl_memory_pool::mutexUnlock()
{
	if (_useMutex) {
		mgl_mutex_unlock(&_mutex);
	}
}

void mgl_memory_pool::add_function(std::string name_function, char *ip_ptr_data)
{
	char *lp_ptr;

#if DEBUG_MEMORY
	if (ip_ptr_data)
	{
		this->mutexLock();

		lp_ptr = ip_ptr_data-4; // Jump over magic number

		std::map<char *, list_string *>::iterator it;
		it = this->_used_memory.find(lp_ptr);
		if(it != this->_used_memory.end())
		{
			(*it).second->push_back(name_function);
		}
		else
		{
		printf("mgl_memory_pool(%s)::add_function(): unknown segment 0x%x\n",
		       _name, lp_ptr);
		}
		this->mutexUnlock();
	}
#endif
}


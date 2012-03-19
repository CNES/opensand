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
/* $Id: mgl_memorypool.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */





#ifndef MGL_MEMORYPOOL_H
#define MGL_MEMORYPOOL_H


#include "mgl_list.h"
#include "mgl_thread.h"

#define DEBUG_MEMORY 0 // set to 1 for memory debugging
#define DEBUG_MEMORY_MORE 0 // set to 1 for even more traces

#if DEBUG_MEMORY
#include <map>
#endif
#include <string>
#include <list>

class mgl_memory_pool {
public:
	mgl_memory_pool(long i_use_mutex=0);
	mgl_memory_pool(long i_bloc_size, long i_nb_blocs, const char *ip_name, long i_use_mutex=0);
	~mgl_memory_pool();

	char _name[64];
	void setName(const char *ip_name);

	mgl_list _available_memory_list;
#if DEBUG_MEMORY
	typedef std::list<std::string> list_string;
	std::map<char *,  list_string *> _used_memory;
	unsigned int allocated;
	unsigned int used;
	unsigned int max_used;
#endif
	mgl_mutex _mutex;
	long _useMutex;
	long _memBlocSize;
	long _nbBlocs;

	mgl_status allocate(long i_bloc_size, long i_nb_blocs);
	char *get(std::string descr, size_t size);
	char *get(std::string descr = "");
	mgl_status release(char *ip_ptr_data);
	mgl_status freeAll();

	mgl_status setMagicAllocated(char *ip_ptr);
	mgl_status checkMagicAllocated(char *ip_ptr);
	mgl_status setMagicFreed(char *ip_ptr);
	mgl_status checkMagicFreed(char *ip_ptr);
	mgl_status checkMemory();

	void mutexLock();
	void mutexUnlock();
	void add_function(std::string name_function, char *ip_ptr_data);
};


#endif



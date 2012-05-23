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
/* $Id: mgl_msgset.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_MSGSET_H
#define MGL_MSGSET_H


#include "mgl_type.h"
#include "mgl_msg.h"
#include "mgl_list.h"

class mgl_msgset {
public:
	mgl_list	_msgIdList;
	mgl_msgset();

	void init();
	void clear();
//	mgl_msgset(mgl_id i_id1);
//	mgl_msgset(mgl_id i_id1, mgl_id i_id2);
//	mgl_msgset(mgl_id i_id1, mgl_id i_id2, mgl_id i_id3);
	mgl_msgset(mgl_id i_id1, mgl_id i_id2=-1, mgl_id i_id3=-1, mgl_id i_id4=-1);
//	mgl_msgset(mgl_id i_id, ...);

	mgl_status msgIdAppend(mgl_id i_id);
	mgl_status msgIdRemove(mgl_id i_id);

	long getCount();
	mgl_id get(long i_index);
	mgl_bool   msgIdIsIn(mgl_id i_id);
	void operator = (mgl_msgset);
	void dump();
};


typedef long mgl_msgset_id;

#define msgsetid_all -1

mgl_status mgl_msgset_dump(mgl_id i_msgsetid);
mgl_bool mgl_msg_is_in_msgset(mgl_id i_msgid, mgl_id i_setid);

mgl_status mgl_set_msgset_list(mgl_msgset **ip_msgset_list);


/************** Some usefull Macro *************/
#define MGL_DECLARE_MSG(msgtype, varptr, next) \
			if (ip_event->type == mgl_event_type_msg) { \
				if (ip_event->event.msg.ptr->type== msgid_ ## msgtype) {\
					varptr = ( msgtype *)(ip_event->event.msg.ptr->pBuf);\
					onEvent_ ## next (ip_event);\
					return mgl_ok;\
				}\
			}





#endif

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
/* $Id: mgl_link.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_LINK_H
#define MGL_LINK_H


#include "mgl_type.h"
#include "mgl_obj.h"
#include "mgl_msgset.h"

typedef long mgl_linkid;

class mgl_link {
public:
	mgl_id srcBloc;
	mgl_id srcPort;
	mgl_id dstBloc;
	mgl_id dstPort;
	mgl_msgset msgset;
	long delay;
	long bandwidth;
	long bandwidth_out_time;
	FILE *stats_fd; // file descriptor
	long stats_period;  //ms

	mgl_link() {
		srcBloc=0;
		srcPort=0;
		dstBloc=0;
		dstPort=0;
		msgset=0;
		delay=0;
		// Bandwidth limitations
		bandwidth=0; // 0, or -1, no limitation
		bandwidth_out_time=0;
		// Statistics
		stats_fd=NULL;
		stats_period=1000;  //ms
	}
} ;

#endif


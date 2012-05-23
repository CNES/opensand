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
/* $Id: mgl_channel.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef MGL_CHANNEL_H
#define MGL_CHANNEL_H


#include "mgl_type.h"
#include "mgl_link.h"
#include "mgl_socket.h"
#include "mgl_string.h"

typedef long mgl_channelid;

class mgl_channel: public mgl_link {
public:
	mgl_channel():mgl_link() {}
};


class mgl_channeldesc 
{
public:
	mgl_string ip;
	long port;
	mgl_list bloc_list;
	mgl_multicast_channel socket;

	mgl_channeldesc(char *ip_ip, long i_port) {
		ip = ip_ip;
		port = i_port;
	}
};

class mgl_channelrcv_info {
public:
	mgl_id bloc;
	mgl_id port;
	mgl_msgset msgset;
};


#endif


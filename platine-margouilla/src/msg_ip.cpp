/**********************************************************************
**
** Margouilla Common Blocs
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
/* $Id: msg_ip.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */
/** 
	This files define message type numbers for IP related packets

*/

/****** Library Includes ******/
#include "msg_ip.h"
#include "lib_ip_udp_cx.h"
#include "lib_mac.h"


mgl_memory_pool g_memory_pool_ip(msg_ip_max_len, 1000, "g_memory_pool_ip");


mgl_memory_pool g_memory_pool_ip_mac(sizeof(mgl_ip_mac), 1000, "g_memory_pool_ip_mac");

mgl_memory_pool g_memory_pool_ip_qos(sizeof(mgl_ip_qos), 1000, "g_memory_pool_ip_qos");

mgl_memory_pool g_memory_pool_ip_mac_qos(sizeof(mgl_ip_mac_qos), 1000, "g_memory_pool_ip_mac_qos");

mgl_memory_pool g_memory_pool_arp(sizeof(mgl_arp), 1000, "g_memory_pool_arp");




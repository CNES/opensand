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
/* $Id: msg_ip.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

/** 
	This files defines message type numbers for IP related packets

*/

#ifndef MSG_IP_H
#define MSG_IP_H


/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Library Includes ******/
#include "mgl_memorypool.h"

/****** Library Includes ******/
#include "lib_mac.h"

const long msg_ip=100; // IP packet, could be v4 or v6: check the version field
const long msg_ipv4=101; // IPv4 packet
const long msg_ipv6=102; // IPv6 packet
const long msg_ip_max_len=2000;

extern mgl_memory_pool g_memory_pool_ip;


// Messages used between the IP layer and the network interfaces
const long msg_arp=150;    // ARP messages
const long msg_ip_mac=151; // IP packet plus MAC address
const long msg_ip_qos=152; // IP packet plus QoS flag
const long msg_ip_mac_qos=153; // IP packet plus MAC address plus QoS flag


typedef struct {
	// Pointer to the IP packet
	char *ptrIp;
	unsigned long lenIp;
	// MAC address
	mgl_mac_addr MAC;
} mgl_ip_mac;

typedef struct {
	// Pointer to the IP packet
	char *ptrIp;
	unsigned long lenIp;
	// QoS
	int qos;
} mgl_ip_qos;

typedef struct {
	// Pointer to the IP packet
	char *ptrIp;
	unsigned long lenIp;
	// MAC address
	mgl_mac_addr MAC;
	// QoS
	int qos;
} mgl_ip_mac_qos;

extern mgl_memory_pool g_memory_pool_ip_mac;
extern mgl_memory_pool g_memory_pool_ip_qos;
extern mgl_memory_pool g_memory_pool_ip_mac_qos;
extern mgl_memory_pool g_memory_pool_arp;

#endif


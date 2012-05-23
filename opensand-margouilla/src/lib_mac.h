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
/* $Id: lib_mac.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef LIB_MAC_H
#define LIB_MAC_H

/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

typedef unsigned long mgl_mac_addr;

typedef unsigned char mgl_arp[48];


#define BLOC_MAC_ARP_OPERATION_ARP_REQUEST		1
#define BLOC_MAC_ARP_OPERATION_ARP_RESPONSE		2
#define BLOC_MAC_ARP_OPERATION_RARP_REQUEST		3
#define BLOC_MAC_ARP_OPERATION_RARP_RESPONSE	4
#define BLOC_MAC_ARP_OPERATION_DYN_RARP_REQUEST		5
#define BLOC_MAC_ARP_OPERATION_DYN_RARP_RESPONSE	6
#define BLOC_MAC_ARP_OPERATION_DYN_RARP_ERROR		7
#define BLOC_MAC_ARP_OPERATION_IN_ARP_REQUEST		8
#define BLOC_MAC_ARP_OPERATION_IN_ARP_RESPONSE		9

#endif


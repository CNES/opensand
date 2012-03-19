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
/* $Id: lib_mac.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

/****** System Includes ******/
#include <stdio.h>
#include <stdlib.h>

/****** Application Includes ******/
#include "lib_mac.h"


/*
MAC address

ARP packet

1 |<   Hardware   |
2 |      type    >|
3 |<    proto     |
4 |      type    >|
5 |<     HLEN    >|
6 |<     PLEN    >|
7 |<   operation  |
8 |              >|
9 |<    sender    |
. |    hardware   |
. |               |
. |              >|
. |<    sender    |
. |    protocol   |
. |               |
. |              >|
. |<    target    |
. |    hardware   |
. |               |
. |              >|
. |<    target    |
. |    protocol   |
. |               |
. |              >|


*/




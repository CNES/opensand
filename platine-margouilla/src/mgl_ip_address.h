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
/* $Id: mgl_ip_address.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#ifndef MGL_IP_ADDRESS_H
#define MGL_IP_ADDRESS_H

/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Runtime Includes ******/
#include "mgl_string.h"

class mgl_ip_addr
{ 
protected:
	unsigned long _version;
	union {
		unsigned long w;
		unsigned char b[4];
	} _v4;
	union {
		unsigned long w[4];
		unsigned char b[16];
	} _v6;

public:
	// Constructor
	mgl_ip_addr();
	mgl_ip_addr(unsigned long i_v4);
	mgl_ip_addr(unsigned long i_a, unsigned long i_b, unsigned long i_c, unsigned long i_d);
	mgl_ip_addr(unsigned long i_a, unsigned long i_b, unsigned long i_c, unsigned long i_d,
				unsigned long i_e, unsigned long i_f, unsigned long i_g, unsigned long i_h,
				unsigned long i_i, unsigned long i_j, unsigned long i_k, unsigned long i_l,
				unsigned long i_m, unsigned long i_n, unsigned long i_o, unsigned long i_p);
	mgl_ip_addr(char *ip_dot_notation);

	// Copy
	mgl_ip_addr & operator=(const unsigned long i_ipv4);
	mgl_ip_addr & operator=(const mgl_ip_addr & i_ip);
	void setIPV6FromBuf(char *);
	
	// Comparison
    unsigned long operator==(const mgl_ip_addr & i_addr) const;
    unsigned long operator!=(const mgl_ip_addr & i_addr) const;

	// Get bytes
    unsigned char operator[](unsigned long i_index) const;
	unsigned long getV4();
	unsigned long getV6W(int index);

	/// Format an address as a string
    mgl_string AsString() const;

};


#endif




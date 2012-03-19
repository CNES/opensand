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
/* $Id: mgl_ip_address.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Runtime Includes ******/
#include "mgl_ip_address.h"


mgl_ip_addr::mgl_ip_addr()
{
	_version=4;
	_v4.w=0;
	memset(_v6.b, 0, 16);
}

mgl_ip_addr::mgl_ip_addr(unsigned long i_a, unsigned long i_b, unsigned long i_c, unsigned long i_d)
{
	_version=4;
	_v4.b[0] = (char)((i_a)& 0xff);
	_v4.b[1] = (char)((i_b)& 0xff);
	_v4.b[2] = (char)((i_c)& 0xff);
	_v4.b[3] = (char)((i_d)& 0xff);
	memset(_v6.b, 0, 16);
}

mgl_ip_addr::mgl_ip_addr(unsigned long i_ipv4)
{
	(*this)=i_ipv4;
}

mgl_ip_addr::mgl_ip_addr(unsigned long i_a, unsigned long i_b, unsigned long i_c, unsigned long i_d,
		unsigned long i_e, unsigned long i_f, unsigned long i_g, unsigned long i_h,
		unsigned long i_i, unsigned long i_j, unsigned long i_k, unsigned long i_l,
		unsigned long i_m, unsigned long i_n, unsigned long i_o, unsigned long i_p)
{
	_version=6;
	_v6.b[0] = (char)((i_a)& 0xff);
	_v6.b[1] = (char)((i_b)& 0xff);
	_v6.b[2] = (char)((i_c)& 0xff);
	_v6.b[3] = (char)((i_d)& 0xff);
	_v6.b[4] = (char)((i_e)& 0xff);
	_v6.b[5] = (char)((i_f)& 0xff);
	_v6.b[6] = (char)((i_g)& 0xff);
	_v6.b[7] = (char)((i_h)& 0xff);
	_v6.b[8] = (char)((i_i)& 0xff);
	_v6.b[9] = (char)((i_j)& 0xff);
	_v6.b[10] = (char)((i_k)& 0xff);
	_v6.b[11] = (char)((i_l)& 0xff);
	_v6.b[12] = (char)((i_m)& 0xff);
	_v6.b[13] = (char)((i_n)& 0xff);
	_v6.b[14] = (char)((i_o)& 0xff);
	_v6.b[15] = (char)((i_p)& 0xff);
	memset(_v4.b, 0, 4);
}


mgl_ip_addr::mgl_ip_addr(char *ip_dot_notation)
{
//inet_pton
}


mgl_ip_addr & mgl_ip_addr::operator=(const unsigned long i_ipv4)
{
	_version=4;
	_v4.w=i_ipv4;
	memset(_v6.b, 0, 16);
	return *this;
}

mgl_ip_addr & mgl_ip_addr::operator=(const mgl_ip_addr & i_ip)
{
	_version=i_ip._version;
	_v4.w=i_ip._v4.w;
	memcpy(_v6.b, i_ip._v6.b, 16);
	return *this;
}

void mgl_ip_addr::setIPV6FromBuf(char *ip_buf)
{
	_version=6;
	memcpy(_v6.b, ip_buf, 16);
	memset(_v4.b, 0, 4);
}

// Get bytes
unsigned char mgl_ip_addr::operator[](unsigned long i_index) const
{
	if (i_index<0) { return 0; }
	if (_version == 6) {
		if (i_index>15) { return 0; }
		return _v6.b[i_index];
	} 
	if (_version == 4) {
		if (i_index>3) { return 0; }
		return _v4.b[i_index];
	} 
	return 0;	
}

/// Format an address as a string
mgl_string mgl_ip_addr::AsString() const
{
	mgl_string l_string;
	long l_cpt;
	char l_buf[64];

	l_string="";
	if (_version == 6) {
		for (l_cpt=0; l_cpt<16; l_cpt++) {
			sprintf(l_buf, "%lx", ((unsigned long)_v6.b[l_cpt])&0xff);
			l_string.append(l_buf);
			if (l_cpt<15) { l_string.append(":"); }
		}
	} 
	if (_version == 4) {
		for (l_cpt=0; l_cpt<4; l_cpt++) {
			sprintf(l_buf, "%lx", ((unsigned long)_v4.b[l_cpt])&0xff);
			l_string.append(l_buf);
			if (l_cpt<3) { l_string.append("."); }
		}
	} 
	return l_string;
}


// Comparison
unsigned long mgl_ip_addr::operator==(const mgl_ip_addr & i_addr) const
{
	if (_version!=i_addr._version) { return 0; }
	if (_version==4) { 
		if (_v4.w==i_addr._v4.w) { 
			return 1; 
		} else { 
			return 0;
		}
	}
	if (_version==6) { 
		if (memcmp(_v6.b, i_addr._v6.b, 16)==0) { 
			return 1; 
		} else { 
			return 0;
		}
	}
	return 0;
}


unsigned long mgl_ip_addr::operator!=(const mgl_ip_addr & i_addr) const
{
	return !((*this)==i_addr);
}


unsigned long mgl_ip_addr::getV4()
{
	return _v4.w;

}

unsigned long mgl_ip_addr::getV6W(int i_index)
{
	return _v6.w[i_index];
}


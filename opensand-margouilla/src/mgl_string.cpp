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
/* $Id: mgl_string.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Application Includes ******/
#include "mgl_string.h"


mgl_string::mgl_string(const char *ip_string)
{
	int i_len;
	char l_buf[]="";
	char *lp_ptr;

	if (!ip_string) {
		i_len=0;
		lp_ptr = l_buf;
	} else {
		i_len = strlen(ip_string);
		lp_ptr = (char*)ip_string;
	}
	bufLen = i_len+64; 
	strLen = i_len;
	
	ptrBuf = (char *)malloc(bufLen);
	if(ptrBuf != NULL)
	{
		bzero(ptrBuf, sizeof(ptrBuf));
		strncpy(ptrBuf, lp_ptr, sizeof(ptrBuf)-1);
	}
	else
	{
		bufLen = 0;
		strLen = 0;
	}
}

mgl_string::mgl_string()
{
	bufLen = 64; 
	strLen = 0;
	
	ptrBuf = (char *)malloc(bufLen);
	if(ptrBuf != NULL)
		bzero(ptrBuf, sizeof(ptrBuf));
	else
		bufLen = 0;
}

mgl_string::~mgl_string()
{
	if(ptrBuf)
		free(ptrBuf);
}

long mgl_string::set(const char *ip_string)
{
	int i_len;

	if (!ip_string) { return mgl_ko; }
	i_len = strlen(ip_string);
	if (i_len >= bufLen) {
		bufLen = i_len+32; 
		if (ptrBuf) {
			free(ptrBuf);
		}
		ptrBuf = (char *)malloc(bufLen);
		if(this->ptrBuf == NULL)
		{
			this->bufLen = 0;
			this->strLen = 0;
			return 0;
		}
	}
	strLen = i_len;
	bzero(ptrBuf, sizeof(ptrBuf));
	strncpy(ptrBuf, ip_string, sizeof(ptrBuf)-1);
	return strLen;
}

long mgl_string::append(const char *ip_string)
{
	int i_len;
	long newBufLen;
	char *lp_ptr;

	if (!ip_string) { return strLen; }
	i_len = strlen(ip_string);

	if((strLen + i_len) > (bufLen - 1))
	{
		newBufLen = strLen + i_len + 32; 

		lp_ptr = (char *) malloc(newBufLen);
		if(lp_ptr == NULL)
			return this->strLen; 

		bzero(lp_ptr, sizeof(lp_ptr));
		strncpy(lp_ptr, ptrBuf, sizeof(lp_ptr)-1);

		free(ptrBuf);
		ptrBuf=lp_ptr;
		this->bufLen = newBufLen;
	}

	strncat(this->ptrBuf + this->strLen, ip_string,
	        sizeof(this->ptrBuf) - this->strLen - 1);
	strLen += i_len;

	return strLen;
}


long mgl_string::append(long i_val)
{
	char l_buf[128];

	snprintf(l_buf, sizeof(l_buf)-1, "%ld", i_val);
	l_buf[sizeof(l_buf)-1] = '\0';

	return append(l_buf);
}


const char *mgl_string::get()
{
	return ptrBuf;
}

char mgl_string::get(long i_index)
{
	if ((i_index>=0)&&(i_index<strLen)) {
		return ptrBuf[i_index];
	} else {
		return 0;
	}
}

long mgl_string::getLength()
{
	return strLen;
}



mgl_string& mgl_string::operator  = (const char *ip_string)
{
	set(ip_string);
	return *this;
}

mgl_string& mgl_string::operator  = (mgl_string &ip_string)
{
	if (&ip_string==this) {return *this;}
	set(ip_string.get());
	return *this;
}

int mgl_string::cmp(char *ip_string)
{
	return strcmp(ptrBuf, ip_string);
}

int mgl_string::cmp(mgl_string &ip_string)
{
	return strcmp(ptrBuf, ip_string.get());
}



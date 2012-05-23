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
/* $Id: mgl_marshall.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

/* Margouilla Editor Lib: Project definition */


#ifndef MGL_MARSHALL_H
#define MGL_MARSHALL_H


#include <stdio.h>
#include <stdlib.h>


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <netinet/in.h>
#endif

// Basic marshallers
template<typename type> extern long marshal_buffer_encode(char *_buffer,long _length,type &_value);
template<typename type> extern long marshal_buffer_decode(char *_buffer,long _length,type &_value);

template<> inline long marshal_buffer_encode<char>(char *_buffer, long _length, char &_value)
  {
    if (_buffer)
      *_buffer = _value;
    return sizeof(char);
  }

template<> inline long marshal_buffer_decode<char>(char *_buffer,long _length,char &_value)
  {
    if (_buffer)
      _value = *_buffer;
    return sizeof(char);
  };

template<> inline long marshal_buffer_encode<long>(char *_buffer,long _length,long &_value)
  {
	  if (_buffer)
      *((long *)_buffer) = htonl(_value);
	  return sizeof(long);
  };


template<> inline long marshal_buffer_decode<long>(char *_buffer,long _length,long &_value)
  {
	  if (_buffer)
      _value = htonl(*((long *)_buffer));
	  return sizeof(long);
  };

template<> inline long marshal_buffer_encode<int>(char *_buffer,long _length,int &_value)
  {
	  if (_buffer)
      *((int *)_buffer) = htonl(_value);
	  return sizeof(int);
  };


template<> inline long marshal_buffer_decode<int>(char *_buffer,long _length,int &_value)
  {
	  if (_buffer)
      _value = htonl(*((int *)_buffer));
	  return sizeof(int);
  };


// Marshaller funtion pointer
typedef long mgl_marshaller_fct (char *_buffer,long _length,void *);

template< typename type > extern inline mgl_marshaller_fct *marshal_buffer_pointer (long (*function) (char *,long,type))
  {
	  return (mgl_marshaller_fct *)function;
  }



#endif

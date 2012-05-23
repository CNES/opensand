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
/* $Id: mgl_config_file.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

#ifndef MGL_CONFIG_FILE_H
#define MGL_CONFIG_FILE_H

#include "mgl_type.h"

long mgl_get_param_string_from_file(const char *ip_filename,
                                    const char *ip_chapter,
                                    const char *ip_param,
                                    char *op_buf,
                                    long i_size);

long mgl_get_param_int_from_file(const char *ip_filename,
                                 const char *ip_chapter,
                                 const char *ip_param,
                                 long *op_val);

#endif


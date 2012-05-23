/**********************************************************************
**
** Margouilla Runtime Test
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
/* $Id: test_config_file.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */


 
/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Application Includes ******/
#include "mgl_config_file.h"


#define TEST_FILENAME "test_config_file.cfg"

int main()
{
  char l_buf[500];
  long l_val;

  if (mgl_get_param_string_from_file(TEST_FILENAME, "Chapter1", "Name", l_buf, 500)) {
	  printf("Name=%s\n", l_buf);
  } else {
	  printf("Can't get name\n");
  }

  if (mgl_get_param_int_from_file(TEST_FILENAME, "Chapter1", "Duration", &l_val)) {
	  printf("Duration=%ld\n", l_val);
  } else {
	  printf("Can't get duration\n");
  }
  return 0;
}



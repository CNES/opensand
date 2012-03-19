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
/* $Id: mgl_config_file.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

/* Margouilla Lib: config file definition */

#include <stdlib.h>		/* errno */
#include <string.h>		/* strcat */
#include <sys/stat.h>		/* stat */

#ifdef WIN32
#include <windows.h>  
#endif

#include "mgl_config_file.h"

#define MGL_CHAPTER_BEGIN "["
#define MGL_CHAPTER_END   "]"
#define MGL_CAR_SPC                 ' '
#define MGL_CAR_TAB                 9
#define MGL_MAX_LINE_LENGTH	1000

long mgl_is_separator(char i_car)
{ 
  long l_ret;

  switch(i_car)
  {
    case MGL_CAR_SPC:  l_ret=1;  break;
    case MGL_CAR_TAB:  l_ret=1;  break;
    default:           l_ret=0; break;
  }
  return l_ret;
}


long mgl_get_param_string_from_file(const char *ip_filename, const char *ip_chapter,
                                    const char *ip_param,
                                    char *op_buf, long i_size)
{
  FILE  *lv_fh;
  char  lv_buf_line[MGL_MAX_LINE_LENGTH]; 
  char  *lv_ptr_buf_line;
  char  lv_buf_chapitre[MGL_MAX_LINE_LENGTH];
  long  lv_flag_chapitre_not_find;
  long  lv_flag_param_not_find;
  char  *lv_ptr_param;
  char  *lv_ptr_param_fin;
  long  lv_taille_param;
  long  l_len;

  // Valid pointers ? 
  if ((!ip_filename)||(!ip_chapter)||(!ip_param)) return 0;

  // Open the file 
  if(!(lv_fh=fopen(ip_filename,"r"))) return 0;

  // Build chapter title 
  strcpy(lv_buf_chapitre, MGL_CHAPTER_BEGIN);
  strcat(lv_buf_chapitre, ip_chapter);
  strcat(lv_buf_chapitre, MGL_CHAPTER_END);

  // Recherche du chapitre 
  lv_flag_chapitre_not_find=1;
  while(lv_flag_chapitre_not_find
        &&fgets(lv_buf_line, MGL_MAX_LINE_LENGTH, lv_fh))
  {
     lv_buf_line[MGL_MAX_LINE_LENGTH-1]=0; // Avoid problem
     if (lv_buf_line[0]!='#')
      if (strstr(lv_buf_line, lv_buf_chapitre))
        lv_flag_chapitre_not_find=0;
  }
  if (lv_flag_chapitre_not_find) {
	  fclose(lv_fh);
	  return 0;
  }

  // Search for the parameter
  lv_flag_param_not_find=1;
  lv_taille_param=strlen(ip_param);
  while(lv_flag_param_not_find
        &&fgets(lv_buf_line, MGL_MAX_LINE_LENGTH, lv_fh))
    {
    if (lv_buf_line[0]!='#'){
      lv_ptr_buf_line=lv_buf_line;
      while (mgl_is_separator(*lv_ptr_buf_line)) 
        lv_ptr_buf_line++;
      if (!strncmp(lv_ptr_buf_line, ip_param, lv_taille_param)) {
        lv_ptr_param=lv_ptr_buf_line;
        if (mgl_is_separator(*(lv_ptr_param+lv_taille_param)))
          lv_flag_param_not_find=0;
      }
    }
  }
  if (lv_flag_param_not_find) {
	  fclose(lv_fh);
	  return 0;
  }
  // Set pointer on value 

  lv_ptr_param+=lv_taille_param;
  
  // Jump over space and separators
  while (mgl_is_separator(*lv_ptr_param)) 
    lv_ptr_param++;

  // Delete carrie return (CR)
  lv_ptr_param_fin=lv_ptr_param;
  while ((*lv_ptr_param_fin)&&(*lv_ptr_param_fin!='\n'))
    lv_ptr_param_fin++;
  if (*lv_ptr_param_fin) *lv_ptr_param_fin=0;

  // Close file
  fclose(lv_fh);

  // Copy string to buffer
  strncpy(op_buf,lv_ptr_param, i_size); 
  l_len = strlen(op_buf);
  return l_len;
}






long mgl_get_param_int_from_file(const char *ip_filename, const char *ip_chapter,
                                 const char *ip_param, long *op_val)
{
  char l_buf[100];
  long l_len;

  // Null pointers ?
  if ((!ip_filename)||(!ip_chapter)||(!ip_param)) return 0;

  // Search value as string
  l_len = mgl_get_param_string_from_file(ip_filename, ip_chapter, ip_param, (char *)l_buf, 100);

  // Value found ?
  if (!l_len) return 0;
  
  (*op_val) = atoi(l_buf);
  return 1;
}


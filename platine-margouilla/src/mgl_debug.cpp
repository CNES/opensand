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
/* $Id: mgl_debug.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */




#include <stdarg.h> // ANSI format
#include <ctype.h> // ANSI format

#include "mgl_debug.h"

/**************************************************
*
* Trace & Debug functions
*
**************************************************/
long g_mgl_trace_level =  MGL_TRACE_DEFAULT|MGL_TRACE_ROUTING|MGL_TRACE_MSG; //|MGL_TRACE_CHANNEL;

void MGL_TRACE_SET_LEVEL(long i_flag)
{
	g_mgl_trace_level = i_flag;
}

void MGL_TRACE_SET_FLAG(long i_flag)
{
	g_mgl_trace_level= (g_mgl_trace_level|=i_flag);
}

int MGL_NEED_TRACE(long i_level) 
{
	return (i_level&g_mgl_trace_level);
}

void MGL_TRACE(const char *ip_file, long i_line, long i_level, const char *ip_format,...)
{
	char l_buf[1024];
	va_list l_va;

	if (!(i_level&g_mgl_trace_level)) { return; }
	va_start( l_va, ip_format );     
	vsprintf( l_buf, ip_format, l_va );
	va_end( l_va );              
	printf("%s", l_buf);
}

void MGL_TRACE_BUF(const char *ip_file, long i_line, long i_level,
                   char *ip_header, char *ip_buf, long i_buflen)
{
	mgl_trace_screen l_trace;
  
	if (!(i_level&g_mgl_trace_level)) { return; }
	l_trace.print(ip_header);
	l_trace.trace_buf(i_line, ip_file, 0, ip_buf, ip_header, i_buflen);

}

void MGL_WARNING(const char *ip_file, long i_line, const char *ip_format,...)
{
	char l_buf[1024];
	va_list l_va;

	if (!(MGL_TRACE_WARNING&g_mgl_trace_level)) { return; }
	va_start( l_va, ip_format );     
	vsprintf( l_buf, ip_format, l_va );
	va_end( l_va );              
	printf("[Warning] %s\n", l_buf);
}

void MGL_CRITICAL(const char *ip_file, long i_line, const char *ip_format,...)
{
	char l_buf[1024];
	va_list l_va;

	if (!(MGL_TRACE_CRITICAL&g_mgl_trace_level)) { return; }
	va_start( l_va, ip_format );     
	vsprintf( l_buf, ip_format, l_va );
	va_end( l_va );              
	printf("[Critical] %s\n", l_buf);
}



/**************************************************
*
* mgl_trace classe
*
**************************************************/

mgl_trace::~mgl_trace()
{
}

void mgl_trace::print(char *ip_buf) 
{
	// Empty funtion
}

void mgl_trace::close() 
{
	// Empty funtion
}

void mgl_trace::trace(const char *ip_format,...)
{
	char l_buf[1024];
	va_list l_va;

	va_start( l_va, ip_format );     
	vsprintf( l_buf, ip_format, l_va );
	va_end( l_va );              
	print(l_buf);
}


/************************************/
/* trace_buf
/* Dump a buffer trace
/* IN:
/* i_sourceLine			: source line to be added in header
/* ip_sourceFilename	: source file name to be added in header
/* i_seuil_trace		: trace level
/* *ip_buf				: buffer to dump
/* *ip_label			: title of the dump
/* i_taillebuf			: size of buf to dump
/************************************/
void mgl_trace::trace_buf(int i_sourceLine,
		      const char *ip_sourceFilename,
		      int  i_seuil_trace,
		      char *ip_buf,
		      char *ip_label,
		      int  i_taillebuf)
{
    static  int	 lv_size;
    static  char lv_char[] = "0123456789ABCDEF";
    static  char lv_message[1024];
    static  int	 lv_position;
    static  char *lv_octet_ptr;
    static  int	 lv_cpt1;
    static  int  lv_taillebuf;

    const   int  MGL_DUMP_OFFSET_SIZE=	7;
    const   int  MGL_DUMP_LINE_SIZE=	74;
    const   int  MGL_DUMP_SEP_POS=		55;
    const	int  MGL_DUMP_ASC_POS=	57;
    const	int  MGL_BYTESPERLINE=	 16;

    lv_taillebuf=i_taillebuf;

	if ((i_taillebuf<0)) { 
		printf("Trace Error : taille buf <0\n");
		return;
	}
	if ((i_taillebuf>100)) { 
		printf("Trace taille buf %d>100, dump limite a 64\n", i_taillebuf);
		lv_taillebuf=64;
	}

     /**** get entete */
      sprintf(lv_message, "[%s:%d]", ip_sourceFilename, i_sourceLine);
      lv_size=strlen(lv_message);

      /**** raw buffer */
      lv_size+=sprintf (&lv_message[lv_size]," : Buffer lg= %d (0x%x) : %s", lv_taillebuf, lv_taillebuf, ip_label);	 
      lv_message[lv_size] = '\n';
      lv_message[lv_size+1] = 0;
      
      /* dump in file */
	  /*
      if (fwrite(lv_message, lv_size+1, 1, l_fh) != 1)
			return;
	  */
      /* dump at screen  */
      printf("%s", lv_message);


      /**** hexa dump */
      if (lv_taillebuf != 0)
      {
		 for (lv_cpt1=0,
			  lv_octet_ptr = ip_buf;
			  lv_taillebuf; lv_taillebuf--, lv_cpt1++, lv_octet_ptr++)
		 {
			/* built a line */
			if ((lv_position = lv_cpt1 % MGL_BYTESPERLINE) == 0)
			{
			   /* built an empty line */
			   sprintf(lv_message,"%05x: ",lv_cpt1);
			   memset(&lv_message[MGL_DUMP_OFFSET_SIZE],' ',MGL_DUMP_LINE_SIZE);
			   lv_message[MGL_DUMP_SEP_POS] = '|';
			   lv_message[MGL_DUMP_LINE_SIZE] = '\n';
			   lv_message[MGL_DUMP_LINE_SIZE+1] = '\0'; 
			}
			
			lv_message[lv_position*3+MGL_DUMP_OFFSET_SIZE] =
			   lv_char[(*lv_octet_ptr >> 4) & 0x0F];
			lv_message[lv_position*3+MGL_DUMP_OFFSET_SIZE+1] =
			   lv_char[(*lv_octet_ptr) & 0x0F];
			if (isprint(*lv_octet_ptr))
			   lv_message[MGL_DUMP_ASC_POS+lv_position] = (*lv_octet_ptr);
			else
			   lv_message[MGL_DUMP_ASC_POS+lv_position] = '.';
			
			/* Test: Size of line */
			if (lv_position == MGL_BYTESPERLINE - 1)
			{
			   /* Write the line */
				/*
			   if (fwrite(lv_message, MGL_DUMP_LINE_SIZE+1, 1, iop_contexte->fh) != 1)
					return;		
				*/
			   /* Write the line */
				lv_message[MGL_DUMP_LINE_SIZE+1]=0;
			   printf("%s", lv_message);
			} /* if */
		 } /* for */
		 
		 if (lv_position != MGL_BYTESPERLINE - 1)
		 {
			   /* Write the line */
			 /*
			if (fwrite(lv_message, MGL_DUMP_LINE_SIZE+1, 1, iop_contexte->fh) != 1)
				return;  
			*/
			lv_message[MGL_DUMP_LINE_SIZE+1]=0;
			printf("%s", lv_message);
		 }
      }
 
	  /***** update trace context ****/
	  /*
	  iop_contexte->nb_ecritures++;
	  if (iop_contexte->nb_ecritures>iop_contexte->frequence_scan)
		check_size_file_trace(iop_contexte);
	  */
/*	} */
}

void mgl_trace_screen::print(char *ip_buf)
{ 
	if (ip_buf) printf("%s", ip_buf); 
};

mgl_trace_file::mgl_trace_file()
{
	_fd=NULL;
}

mgl_status mgl_trace_file::open(char *ip_file)
{
	_fd = fopen(ip_file, "w");
	if (_fd) {
		return mgl_ok;
	} else {
		return mgl_ko;
	}
}

void mgl_trace_file::print(char *ip_buf) 
{ 
	if (_fd&&ip_buf) {
		fprintf(_fd, "%s", ip_buf); 
		fflush(_fd);
	}
};


void mgl_trace_file::close()
{
	fclose(_fd);
}


/************** Trace into a file with Nam format *****/

void mgl_trace_file_nam::init_node(mgl_id i_id)
{
	trace("n -t * -s %d -S UP -v circle -c black\n", i_id); 
}

void mgl_trace_file_nam::init_link(mgl_id i_src_id, mgl_id i_dst_id)
{
	trace("l -t * -s %d -d %d -S UP -r 10000000 -D 0.01 -o\n", i_src_id, i_dst_id); 
}

void mgl_trace_file_nam::init_queue(mgl_id i_src_id, mgl_id i_dst_id)
{
	trace("q -t * -s %d -d %d -a 0.5\n", i_src_id, i_dst_id); 
}

void mgl_trace_file_nam::send(long i_time_ms, mgl_id i_src, mgl_id i_dst, const char *ip_desc, long i_length, long i_pkt_id)
{
	float l_f=((float)i_time_ms)/1000;
	trace("h -t %f -s %d -d %d -p %s -e %d -c 0 -i %d -a 0\n", 
		l_f, i_src, i_dst, ip_desc, i_length, i_pkt_id); 
}

void mgl_trace_file_nam::receive(long i_time_ms, mgl_id i_src, mgl_id i_dst, const char *ip_desc, long i_length, long i_pkt_id)
{
	float l_f=((float)i_time_ms)/1000;
	trace("r -t %f -s %d -d %d -p %s -e %d -c 0 -i %d -a 0\n", 
		l_f, i_src, i_dst, ip_desc, i_length, i_pkt_id); 
}

void mgl_trace_file_nam::drop(long i_time_ms, mgl_id i_src, mgl_id i_dst, char *ip_desc, long i_length, long i_pkt_id)
{
	float l_f=((float)i_time_ms)/1000;
	trace("d -t %f -s %d -d %d -p %s -e %d -c 0 -i %d -a 0\n", 
		l_f, i_src, i_dst, ip_desc, i_length, i_pkt_id); 
}


void mgl_trace_file_nam::enqueue(long i_time_ms, mgl_id i_src, mgl_id i_dst, char *ip_desc, long i_length, long i_pkt_id)
{
	float l_f=((float)i_time_ms)/1000;
	trace("+ -t %f -s %d -d %d -p %s -e %d -c 0 -i %d -a 0\n", 
		l_f, i_src, i_dst, ip_desc, i_length, i_pkt_id); 
}

void mgl_trace_file_nam::dequeue(long i_time_ms, mgl_id i_src, mgl_id i_dst, char *ip_desc, long i_length, long i_pkt_id)
{
	float l_f=((float)i_time_ms)/1000;
	trace("- -t %f -s %d -d %d -p %s -e %d -c 0 -i %d -a 0\n", 
		l_f, i_src, i_dst, ip_desc, i_length, i_pkt_id); 
}




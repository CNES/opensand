/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/*
 * @file sat_emulator_err.cpp
 * @brief  Emulate satellite errors
 * @author Viveris Technologies
 */

//
// System includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#   include <io.h>
#   include <time.h>
#else /*  */
#   include <errno.h>
#   include <dirent.h>
#endif /*  */

//
// Libraries includes
//
/*
  #include "../links/ll_sock.h"
  #include "../links/port_uxnt.h"
  #include "sat_emulator.h"
*/
#include "sat_emulator_err.h"

// Logging configuration
//
#define DBG_PACKAGE PKG_DVB_RCS_SAT
#include "opensand_conf/uti_debug.h"

//
// Global variables
//

// Use to generate an error burst
int SE_g_burstError_toDo = 0;
int g_no_error = 0;

// Internal state of the error generator
long SE_g_nbOctets_between_burstError = 100000000; // calculated Value, depend on ber+mean+amp...
long SE_g_nbOctets_next_error_change = 10000000; // counter decremented at each frame
int SE_g_nbOctets_burstError_state = 1; //1(no error), -1(error)

// 0: default statistic generator
// >0: precalculated data arrays index
int SE_g_error_generator = 0;

// Precalculated error distributions
#define MAX_error_generator 10
TS_error_generator *SE_gp_tab_error_generator[MAX_error_generator];
int SE_g_nb_generator = 0;
int SE_g_trace_level = 0;

/**************************
 * clean_string
 * Replace CR/LF last char by NULL
 **************************/
void clean_string(char *iop_string)
{
	int l_len;
	int l_cpt;
	l_len = strlen(iop_string);
	for(l_cpt = 0; (l_cpt < 3) && (l_len > 0); l_cpt++)
	{
		if(*(iop_string + l_len) == 10)
		{
			*(iop_string + l_len) = 0;
		}
		l_len--;
	}
}


/**************************
 * init_error_generator_1
 * inititalise a set of precalculated values
 **************************/
void init_error_generator_1()
{
	int l_cpt;
	static TS_error_generator s_error_generator_1;
	sprintf(s_error_generator_1.name, "Generateur Test");
	sprintf(s_error_generator_1.desc, "BER 10-3");
	s_error_generator_1.nb_measurements = 12;
	s_error_generator_1.current_measurement = 0;
	for(l_cpt = 0; l_cpt < 12; l_cpt++)
	{
		s_error_generator_1.lenght_before_burst[l_cpt] = 1000;
		s_error_generator_1.burst_length[l_cpt] = 1;
	}
	SE_g_nb_generator++; /* Must be 1,2,3.... (0 is default generator) */
	SE_gp_tab_error_generator[SE_g_nb_generator] = &s_error_generator_1;
}


/**************************
 * init_error_generator_from_file
 * inititalise a set of precalculated values
 **************************/
int SE_init_error_generator_from_file(char *ip_filename)
{
	const char *FUNCNAME = "[SE_init_error_generator_from_file]";
	TS_error_generator *lp_error_generator;
	FILE *lp_f;
	int l_ret;
	char *lp_char;
	int l_cpt;
	int l_val1, l_val2;

	/* Test file */
	lp_f = fopen(ip_filename, "r");
	if(!lp_f)
	{
		UTI_INFO("%s Pb, Cant open file <%s>\n", FUNCNAME, ip_filename);
		return 0;
	}
	l_ret = fscanf(lp_f, "Brahms Error Generator file, v%d\n", &l_cpt);
	if(l_ret <= 0)
	{
		UTI_INFO("%s Not an Error Generator File <%s>\n", FUNCNAME, ip_filename);
		return 0;
	}

	/* Allocate memory */
	UTI_INFO("%s File <%s>\n", FUNCNAME, ip_filename);
	lp_error_generator =
		(TS_error_generator *) malloc(sizeof(TS_error_generator));

	/* Load values */
	lp_error_generator->name[0] = 0;

	//l_ret = fscanf(lp_f,"Name=%120s\n", &(lp_error_generator->name));
	l_ret = fscanf(lp_f, "Name=");
	lp_char = fgets((char *) &(lp_error_generator->name), 120, lp_f);
	if (lp_char == NULL)
	{
		UTI_INFO("%s Error getting name", FUNCNAME);
		return 0;	
	}
			
	clean_string(lp_error_generator->name);
	UTI_INFO("%s Error generator name <%s>\n", FUNCNAME, lp_error_generator->name);
	lp_error_generator->desc[0] = 0;
	l_ret = fscanf(lp_f, "Description=");
	lp_char= fgets((char *) &(lp_error_generator->desc), 498, lp_f);
	if (lp_char == NULL)
	{
		UTI_INFO("%s Error getting desc",FUNCNAME);
	}
	clean_string(lp_error_generator->desc);
	UTI_INFO("%s Error generator description <%s>\n", FUNCNAME,
	         lp_error_generator->desc);
	l_ret =
		fscanf(lp_f, "Nb measurements=%d\n",
				 &(lp_error_generator->nb_measurements));
	lp_error_generator->current_measurement = 0;
	UTI_INFO("%s Nb measurements <%d>\n", FUNCNAME,
	         lp_error_generator->nb_measurements);
	for(l_cpt = 0; l_cpt < lp_error_generator->nb_measurements; l_cpt++)
	{
		l_ret = fscanf(lp_f, "%d, %d,\n", &l_val1, &l_val2);
		lp_error_generator->lenght_before_burst[l_cpt] = l_val1;
		lp_error_generator->burst_length[l_cpt] = l_val2;
		if(l_cpt < 20)
		{
			UTI_INFO("<%ld,%ld> ",
			         lp_error_generator->lenght_before_burst[l_cpt],
			         lp_error_generator->burst_length[l_cpt]);
		}
	}
	UTI_INFO("\n");
	SE_g_nb_generator++;
	SE_gp_tab_error_generator[SE_g_nb_generator] = lp_error_generator;
	fclose(lp_f);
	return SE_g_nb_generator;
}


/**************************
 * init_error_generator
 * inititalise the precalculated distribution
 **************************/
void SE_init_error_generator()
{

#ifdef WIN32
	struct _finddata_t c_file;
	long hFile;

#else /*  */
	DIR *dir;
	struct dirent *ent;

#endif /*  */
	char l_tab_dir[2][255] = { ".", "beg" };
	char l_buf[255];
	int l_cpt_dir;
	for(l_cpt_dir = 0; l_cpt_dir < 2; l_cpt_dir++)
	{

#ifdef WIN32
		UTI_INFO
			("[Loading error generator]: Scanning directory for *.beg file\n");
		sprintf(l_buf, "%s/*.beg", l_tab_dir[l_cpt_dir]);
		if((hFile = _findfirst(l_buf, &c_file)) == -1L)
			printf("No *.beg files in directory <%s>\n", l_tab_dir[l_cpt_dir]);

		else
		{
			do
			{
				sprintf(l_buf, "%s/%s", l_tab_dir[l_cpt_dir], c_file.name);
				SE_init_error_generator_from_file(l_buf);
			}
			while(_findnext(hFile, &c_file) == 0);
			_findclose(hFile);
		}

#else /*  */
		if(!(dir = opendir(l_tab_dir[l_cpt_dir])))
		{
			UTI_INFO("Can't open directory <%s>\n", l_tab_dir[l_cpt_dir]);
			return;
		}
		while((ent = readdir(dir)))
		{
			if(strstr(ent->d_name, ".beg"))
			{
				sprintf(l_buf, "%s/%s", l_tab_dir[l_cpt_dir], ent->d_name);
				SE_init_error_generator_from_file(l_buf);
			}
		}
		closedir(dir);

#endif /*  */
	}
}
struct _SE_g_val_cmd
{
	int MeanErrLen;
	int AmpErrLen;
} SE_g_val_cmd;

/**************************
 * SE_get_next_error_change
 * return length of this error/no erro state
 **************************/
long SE_get_next_error_change(int SE_g_nbOctets_burstError_state)
{
	long l_len = 0;
	if(SE_g_nbOctets_burstError_state < 0)
	{
		// State no error
		if(SE_g_error_generator == 0)
		{
			// statistic generator
			l_len = SE_g_nbOctets_between_burstError;
		}
		else
		{ // Real measurement generator
			l_len =
				(long) SE_gp_tab_error_generator[SE_g_error_generator]->
				lenght_before_burst[SE_gp_tab_error_generator
				                   [SE_g_error_generator]->current_measurement];
			l_len = (l_len / 8) + 1; // Measurement in bits
		} return l_len;
	}
	else
	{  // State Error
		if(SE_g_error_generator == 0)
		{
			// statistic generator
			l_len = SE_g_val_cmd.MeanErrLen
				- SE_g_val_cmd.AmpErrLen / 2 + (rand() % (SE_g_val_cmd.AmpErrLen));
		}
		else
		{
			// Real measurement generator
			l_len =
				(long) SE_gp_tab_error_generator[SE_g_error_generator]->
				burst_length[SE_gp_tab_error_generator[SE_g_error_generator]->
								 current_measurement];
			l_len = (l_len / 8) + 1; // Measurement in bits
			SE_gp_tab_error_generator[SE_g_error_generator]->current_measurement++;
			if(SE_gp_tab_error_generator[SE_g_error_generator]->
				current_measurement >=
				SE_gp_tab_error_generator[SE_g_error_generator]->nb_measurements)
			{
				SE_gp_tab_error_generator[SE_g_error_generator]->
					current_measurement = 0;
			}
		}
		return l_len;
	}
	return l_len;
}


/**************************
 * SE_errors_buf
 * Add errors to message
 * return : 0 : No error introduced
 *          1 : Error introduced
 **************************/
int SE_errors_buf(char *iop_buf, int i_len)
{
	char *lp_buf;
	int l_cpt;
	int l_buf_size_pending;
	int l_len;
	if(g_no_error)
		return 0;

	// Error Burst command
	// Whole paquet corrupted
	if(SE_g_burstError_toDo > 0)
	{
		lp_buf = iop_buf;
		for(l_cpt = 0; l_cpt < i_len; l_cpt++)
		{
			(*lp_buf) = (char) (rand() % 255);
			lp_buf++;
		} SE_g_burstError_toDo--;
		return 1;
	}

	// BER error
	l_buf_size_pending = i_len;
	if(SE_g_trace_level)
		UTI_INFO("Packet : [");

SE_errors_go_state:
	//do
	//{
	// State Change position
	lp_buf = iop_buf + i_len - l_buf_size_pending;

	// State length
	if(SE_g_nbOctets_next_error_change < l_buf_size_pending)
	{
		l_len = SE_g_nbOctets_next_error_change;
	}
	else
	{
		l_len = l_buf_size_pending;
	}

	// state
	if(SE_g_nbOctets_burstError_state < 0)
	{
		// errors state
		for(l_cpt = 0; l_cpt < l_len; l_cpt++)
		{
			(*lp_buf) = (char) (rand() % 255);
			lp_buf++;
		} if(SE_g_trace_level)
			UTI_INFO("error (%d octets), ", l_len);
	}
	else
	{

		// no error state
		if(SE_g_trace_level)
			UTI_INFO("no errors (%d octets), ", l_len);
	}

	// state change
	if(SE_g_nbOctets_next_error_change < l_buf_size_pending)
	{
		l_buf_size_pending -= SE_g_nbOctets_next_error_change;
		SE_g_nbOctets_next_error_change =
			SE_get_next_error_change(SE_g_nbOctets_burstError_state);
		SE_g_nbOctets_burstError_state = -SE_g_nbOctets_burstError_state;	// state change
		goto SE_errors_go_state;
	}
	else
	{
		SE_g_nbOctets_next_error_change -= l_len;
	}

	//} while(SE_g_nbOctets_next_error_change<l_buf_size_pending);
	if(SE_g_trace_level)
		UTI_INFO("], next change %ld\n", SE_g_nbOctets_next_error_change);
	return 0;
}


/**************************
 * SE_power
 * return : i_a power i_b
 **************************/
float SE_power(int i_a, int i_b)
{
	int l_cpt;
	float l_res;
	l_res = 1;
	for(l_cpt = 0; l_cpt < i_b; l_cpt++)
	{
		l_res *= i_a;
	}
	return l_res;
}


/**************************
 * SE_set_err_param
 * Used to configure the homogene distribution generator
 **************************/
void SE_set_err_param(int i_ber, int i_mean, int i_amp)
{
	SE_g_nbOctets_between_burstError =
		(long) (SE_power(10, i_ber) / 8) * (i_mean);
	SE_g_nbOctets_next_error_change = 0;
	SE_g_val_cmd.MeanErrLen = i_mean;
	SE_g_val_cmd.AmpErrLen = i_amp;
	UTI_INFO
		("[ErrCtrl] Set BER=10-%d, burst length=%d, %ld octets between burst errors\n",
		 i_ber, i_mean, SE_g_nbOctets_between_burstError);
}

/**************************
 * SE_set_err_param
 * Used to configure the homogene distribution generator
 * return :  0: OK
 * return : -1: KO
 **************************/
int SE_set_error_generator(int i_err_gen)
{
	UTI_INFO("[ErrCtrl] Set Error generator num=%d :", i_err_gen);
	if((i_err_gen < 0) || (i_err_gen > SE_g_nb_generator))
	{
		UTI_INFO("Bad value.\n");
		return -1;
	}
	SE_g_error_generator = i_err_gen;
	SE_g_nbOctets_next_error_change = 0;
	if(i_err_gen == 0)
	{
		UTI_INFO("[Default] Homogene distribution generator.\n");
		return 0;
	}
	else
	{
		UTI_INFO("[%s] %s.\n", SE_gp_tab_error_generator[i_err_gen]->name,
		         SE_gp_tab_error_generator[i_err_gen]->desc);
		return 0;
	}
}

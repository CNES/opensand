/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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

/***************************************************/
/*                                                 */
/* BRAHMS IST Project Demonstrator                 */
/* -------------------------------                 */
/*                                                 */
/* Satellite emulator : Emulate a satellite.       */
/*                      Send and receive packets.  */
/*                                                 */
/*                              S. Josset          */
/*                              Alcatel Space 2000 */
/*               sebastien.josset@space.alcatel.fr */
/*                                                 */
/*                                                 */
/***************************************************/

#ifndef SAT_EMULATOR_ERR_H
#   define SAT_EMULATOR_ERR_H

//
// System includes
//
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>

//
// Libraries includes
//
/*
#include "../links/ll_sock.h"
#include "../links/port_uxnt.h"
*/


//
// Global variables
//

extern int SE_g_burstError_toDo;

extern long SE_g_nbOctets_between_burstError; // Value
extern long SE_g_nbOctets_next_error_change;  // counter decremented at each frame
extern int SE_g_nbOctets_burstError_state;    //1(no error), -1(error)

// 0: default statistic generator
// >0: precalculated data arrays index
extern int SE_g_error_generator;

//
// lenght_before_burst[0],burst_length[0],lenght_before_burst[1],burst_length[1],....
// lenght_before_burst[nbmeasurements-1],burst_length[nbmeasurements-1]
#   define MAX_measurements 10000
typedef struct
{
	char name[128];
	char desc[500];
	int nb_measurements;
	int current_measurement;
	long lenght_before_burst[MAX_measurements]; // length in bit
	long burst_length[MAX_measurements]; // length in bit
} TS_error_generator;

#   define MAX_error_generator 10
extern TS_error_generator *SE_gp_tab_error_generator[MAX_error_generator];
extern int SE_g_nb_generator;

void SE_init_error_generator();
int SE_init_error_generator_from_file(char *ip_filename);
void SE_set_err_param(int i_ber, int i_mean, int i_amp);
int SE_set_error_generator(int i_err_gen);
int SE_errors_buf(char *iop_buf, int i_len);


#endif

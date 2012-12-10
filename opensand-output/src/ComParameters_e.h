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

/**
 * @file ComParameters_e.h
 * @author TAS
 * @brief The ComParameters class implements the reading of communication
 *        parameters configuration file
 */

#ifndef ComParameters_e
#   define ComParameters_e

#   include <sys/socket.h>

/*********************/
/* PROJECT RESOURCES */
/*********************/
#   include "EnumCouple_e.h"
#   include "Error_e.h"
#   include "DominoConstants_e.h"
#   include "IPAddr_e.h"
#   include "FileReader_e.h"

#   define C_NB_DISPLAY_PORTS   3
										  /* Number of ports used for display */
#   define C_MAX_HOSTNAME_SIZE 50
										  /* Hostname maximum size */

/********************/
/* ENUM DEFINITIONS */
/********************/
typedef enum
{
	C_INET = AF_INET,				  // value 1
	C_UNIX = AF_UNIX,				  // value 2
	C_PORT_FAMILY_MAX = 3		  // value 3
} T_PORT_FAMILY_NUMBERS;


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/
typedef struct
{										  /* LEVEL 2 or LEVEL 3 */
	T_PORT_FAMILY_NUMBERS _Family;
	T_IP_ADDR _IpAddress;
} T_COM_STRUCT;


typedef struct
{										  /* LEVEL 1 */
	T_COM_STRUCT _ErrorController;
	T_COM_STRUCT _EventController;
	T_COM_STRUCT _ProbeController;
} T_CONTROLLERS_PORTS;


typedef struct
{
	T_COM_STRUCT _EventDisplay;
	T_COM_STRUCT _ErrorDisplay;
	T_COM_STRUCT _ProbeDisplay;
} T_DISPLAY_PORTS;

typedef struct
{										  /* LEVEL 0 */
	T_CONTROLLERS_PORTS _ControllersPorts;
	T_DISPLAY_PORTS _DisplayPorts;
	T_ENUM_COUPLE C_PORT_FAMILY_choices[C_PORT_FAMILY_MAX];
} T_COM_PARAMETERS;


T_ERROR COM_PARAMETERS_ReadConfigFile(
													 /* INOUT */ T_COM_PARAMETERS *
													 ptr_this);


T_ERROR COM_PARAMETERS_PrintConfigFile(
													  /* INOUT */ T_COM_PARAMETERS *
													  ptr_this);


#endif /* ComParameters_e */

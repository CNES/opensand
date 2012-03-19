/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file FilePath_e.h
 * @author TAS
 * @brief The FilePath class implements method to get exec path,
 *        scenario path and run path for curent simulation
 */

#ifndef FilePath_e
#   define FilePath_e

#   include "Error_e.h"

#   define C_FILE_PATH_MAX_CARS 256


typedef T_CHAR T_FILE_PATH[C_FILE_PATH_MAX_CARS];


/* Gets the simulation run path. 
    This directory will contain all files needed and produced by a simulation run.  */

T_ERROR FILE_PATH_InitClass();


T_ERROR FILE_PATH_GetConfPath(
										  /*   OUT */ T_STRING ptr_this);

T_ERROR FILE_PATH_GetScenarioPath(
												/*   OUT */ T_STRING ptr_this,
												/* IN    */ T_UINT16 reference);

T_ERROR FILE_PATH_GetRunPath(
										 /*   OUT */ T_STRING ptr_this,
										 /* IN    */ T_UINT16 reference,
										 /* IN    */ T_UINT16 run);

T_ERROR FILE_PATH_GetOutputPath(
											 /*   OUT */ T_STRING ptr_this,
											 /* IN    */ T_UINT16 reference,
											 /* IN    */ T_UINT16 run);

T_ERROR FILE_PATH_Concat(
									/*   OUT */ T_STRING ptr_this,
									/* IN    */ T_STRING file_name);

#endif

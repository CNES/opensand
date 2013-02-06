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
 * @file ErrorDef_e.h
 * @author TAS
 * @brief The ErrorDef class implements the error definition
 *        configuration file reading
 */

#ifndef ErrorDef_e
#   define ErrorDef_e

#   include "Error_e.h"

/*********************/
/* MACRO DEFINITIONS */
/*********************/
/* All these limits shall be reconsidered at integration-time  */
#   define C_ERR_DEF_MAX_CAR_NAME     64
													/* maximum number of characters for error Name */
#   define C_ERR_DEF_MAX_CAR_IDX_SIGN 32
													/* maximum number of characters for index signification */
#   define C_ERR_DEF_MAX_CAR_VAL_SIGN 32
													/* maximum number of characters for value signification */
#   define C_ERR_DEF_MAX_CAR_UNIT     32
													/* maximum number of characters for Unit */
#   define C_INDEX_DEF_MAX_CAR        32
													/* maximum number of characters for Index value */
#   define C_INDEX_DEF_MAX_NB         48
													/* maximum number of Index for one type */
#   define C_ERR_DEF_MAX_ERRORS       100


/********************/
/* ENUM DEFINITIONS */
/********************/
typedef enum
{
	C_ERROR_LABEL_COMMAND = 0,
	C_ERROR_LABEL_CRITICAL,
	C_ERROR_LABEL_MINOR,

	C_ERROR_LABEL_MAX_NB
} T_ERROR_LABEL;


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/
typedef T_CHAR T_INDEX_VALUE[C_INDEX_DEF_MAX_CAR];


typedef struct
{										  /* LEVEL 2 */
	T_UINT32 _nbIndex;
	T_INDEX_VALUE _IndexValues[C_INDEX_DEF_MAX_NB];
} T_INDEX_TAB;


typedef struct
{										  /* LEVEL 1 */

	T_INT32 _ErrorId;
	T_INT32 _Category;
	T_CHAR _Name[C_ERR_DEF_MAX_CAR_NAME];
	T_CHAR _IndexSignification[C_ERR_DEF_MAX_CAR_IDX_SIGN];
	T_CHAR _ValueSignification[C_ERR_DEF_MAX_CAR_VAL_SIGN];
	T_CHAR _Unit[C_ERR_DEF_MAX_CAR_UNIT];

	T_INDEX_TAB _IndexTab;
} T_ERROR_DEF;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _nbError;
	T_ERROR_DEF _Error[C_ERR_DEF_MAX_ERRORS];
} T_ERRORS_DEF;


T_ERROR ERROR_DEF_ReadConfigFile(
											  /* INOUT */ T_ERRORS_DEF * ptr_this);


#endif /* ErrorDef_e */

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
 * @file SizedBuffer.c
 * @author TAS
 * @brief The SizedBuffer class creates a buffer with a variable size
 */

/* SYSTEM RESOURCES */
#include <stdlib.h>
#include <string.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "SizedBuffer_e.h"


/*  @ROLE    : This function initialises the buffer
    @RETURN  : Error code */
T_ERROR SIZED_BUFFER_Init(
									 /* INOUT */ T_SIZED_BUFFER * ptr_this,
									 /* IN    */ T_UINT32 eltSize,
									 /* IN    */ T_UINT32 eltNumberMax)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_SIZED_BUFFER));

	if((eltSize != 0) && (eltNumberMax != 0))
	{
		ptr_this->_buffer = (T_BUFFER) malloc(eltSize * eltNumberMax);
		if(ptr_this->_buffer == NULL)
		{
			JUMP_TRACE(FIN, rid, C_ERROR_ALLOC,
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
							C_TRACE_ERROR, "SIZED_BUFFER_Init() malloc failed"));
		}
		memset(ptr_this->_buffer, 0, eltSize * eltNumberMax);
	}
	else
		ptr_this->_buffer = NULL;

	ptr_this->_eltSize = eltSize;
	ptr_this->_eltNumberMax = eltNumberMax;
	ptr_this->_eltNumber = 0;

 FIN:
	return rid;
}


/*  @ROLE    : This function deletes the buffer
    @RETURN  : Error code */
T_ERROR SIZED_BUFFER_Terminate(
											/* INOUT */ T_SIZED_BUFFER * ptr_this)
{
	free(ptr_this->_buffer);
	return C_ERROR_OK;
}

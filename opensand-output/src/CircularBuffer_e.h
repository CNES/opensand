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
 * @file CircularBuffer_e.h
 * @author TAS
 * @brief The CircularBuffer class implements the circular buffer services
 */

#ifndef CircularBuffer_e
#   define CircularBuffer_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "SizedBuffer_e.h"

typedef struct
{
	T_UINT32 _writeIndex;		  /* The buffer write index */
	T_UINT32 _readIndex;			  /* The buffer read index */
	T_SIZED_BUFFER _circularBuf; /* The circular Buffer */
} T_CIRCULAR_BUFFER;


/*  @ROLE    : This function initialises the buffer
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_Init(
												  /* INOUT */ T_CIRCULAR_BUFFER * ptr_this,
												  /* IN    */ T_UINT32 eltSize,
												  /* IN    */ T_UINT32 eltNumber);


/*  @ROLE    : This function deletes the buffer
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_Terminate(
														 /* INOUT */ T_CIRCULAR_BUFFER *
														 ptr_this);


/*  @ROLE    : This function reset the buffer contains
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_ResetBuffer(
															/* INOUT */ T_CIRCULAR_BUFFER *
															ptr_this);


/*  @ROLE    : This function returns the buffer pointer to the element to write
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_GetWriteBuffer(
																/* INOUT */ T_CIRCULAR_BUFFER *
																ptr_this,
																/*   OUT */
																T_BUFFER * ptr_buffer);


/*  @ROLE    : This function returns the buffer pointer to the next element to write
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_GetWriteBufferWithoutPublish(
																				  /* INOUT */
																				  T_CIRCULAR_BUFFER
																				  * ptr_this,
																				  /*   OUT */
																				  T_BUFFER *
																				  ptr_buffer);


/*  @ROLE    : This function returns the buffer pointer to the element to read
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_GetPrevReadBuffer(
																	/* INOUT */ T_CIRCULAR_BUFFER
																	* ptr_this,
																														/* IN    */ T_UINT32 prevElmtNumber,
																														/* 0 -> the last previous elemt */
																	/*   OUT */
																	T_BUFFER * ptr_buffer);


/*  @ROLE    : This function returns the buffer pointer to the first element to read
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_GetFirstReadBuffer(
																	 /* INOUT */
																	 T_CIRCULAR_BUFFER *
																	 ptr_this,
																	 /*   OUT */
																	 T_BUFFER * ptr_buffer);


/*  @ROLE    : This function returns the buffer pointer to the first written element
               and remove it
    @RETURN  : Error code */
extern T_ERROR CIRCULAR_BUFFER_GetAndRemoveReadBuffer(
																		  /* INOUT */
																		  T_CIRCULAR_BUFFER *
																		  ptr_this,
																		  /*   OUT */
																		  T_BUFFER * ptr_buffer);


/*  @ROLE    : check if the buffer is empty */
#   define CIRCULAR_BUFFER_IsEmpty(ptr_this) \
  SIZED_BUFFER_IsEmpty(&((ptr_this)->_circularBuf))


/*  @ROLE    : check if the buffer is full */
#   define CIRCULAR_BUFFER_IsFull(ptr_this) \
  SIZED_BUFFER_IsFull((&(ptr_this)->_circularBuf))


/*  @ROLE    : returns the number of element in buffer */
#   define CIRCULAR_BUFFER_GetEltNumber(ptr_this) \
  SIZED_BUFFER_GetEltNumber(&((ptr_this)->_circularBuf))

/*  @ROLE    : returns the size of an element in buffer */
#   define CIRCULAR_BUFFER_GetEltSize(ptr_this) \
  SIZED_BUFFER_GetEltSize(&((ptr_this)->_circularBuf))


/*  @ROLE    : returns the remainted number of element in buffer */
#   define CIRCULAR_BUFFER_GetRemaintedEltNumber(ptr_this) \
  SIZED_BUFFER_GetRemaintedEltNumber(&((ptr_this)->_circularBuf))


#endif /* CircularBuffer_e */

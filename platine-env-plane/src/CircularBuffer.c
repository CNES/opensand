/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The CircularBuffer class implements the circular buffer services
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/
/* SYSTEM RESOURCES */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "SizedBuffer_e.h"
#include "CircularBuffer_e.h"


/*  @ROLE    : This function initialises the buffer
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_Init(
										 /* INOUT */ T_CIRCULAR_BUFFER * ptr_this,
										 /* IN    */ T_UINT32 eltSize,
										 /* IN    */ T_UINT32 eltNumber)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_CIRCULAR_BUFFER));

	/* Create the sized buffer */
	JUMP_ERROR(FIN, rid,
				  SIZED_BUFFER_Init(&(ptr_this->_circularBuf), eltSize, eltNumber));

	/* data inits */
	ptr_this->_writeIndex = 0;
	ptr_this->_readIndex = 0;

 FIN:
	return rid;
}


/*  @ROLE    : This function deletes the buffer
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_Terminate(
												/* INOUT */ T_CIRCULAR_BUFFER * ptr_this)
{
	return (SIZED_BUFFER_Terminate(&(ptr_this->_circularBuf)));
}


/*  @ROLE    : This function reset the buffer contains
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_ResetBuffer(
												  /* INOUT */ T_CIRCULAR_BUFFER * ptr_this)
{
	ptr_this->_writeIndex = 0;
	ptr_this->_readIndex = 0;
	ptr_this->_circularBuf._eltNumber = 0;

	return C_ERROR_OK;
}


/*  @ROLE    : This function returns the buffer pointer to the element to write
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_GetWriteBuffer(
													  /* INOUT */ T_CIRCULAR_BUFFER *
													  ptr_this,
													  /*   OUT */ T_BUFFER * ptr_buffer)
{
	/* return the buffer pointer */
	*ptr_buffer = SIZED_BUFFER_GetBufferPtr(&(ptr_this->_circularBuf),
														 ptr_this->_writeIndex);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
				  C_TRACE_DEBUG_3,
				  "CIRCULAR_BUFFER_GetWriteBuffer() get buffer index %d",
				  ptr_this->_writeIndex));

	/* increase the buffer index */
	ptr_this->_writeIndex = SIZED_BUFFER_GetNextIndex(&(ptr_this->_circularBuf),
																	  ptr_this->_writeIndex);
	if(CIRCULAR_BUFFER_IsFull(ptr_this))
		ptr_this->_readIndex = ptr_this->_writeIndex;

	SIZED_BUFFER_IncreaseElt(&(ptr_this->_circularBuf));

	return C_ERROR_OK;
}


/*  @ROLE    : This function returns the buffer pointer to the next element to write
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_GetWriteBufferWithoutPublish(
																		 /* INOUT */
																		 T_CIRCULAR_BUFFER *
																		 ptr_this,
																		 /*   OUT */
																		 T_BUFFER * ptr_buffer)
{
	/* return the buffer pointer */
	*ptr_buffer = SIZED_BUFFER_GetBufferPtr(&(ptr_this->_circularBuf),
														 ptr_this->_writeIndex);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
				  C_TRACE_DEBUG_3,
				  "CIRCULAR_BUFFER_GetWriteBufferWithoutPublish() get buffer index %d",
				  ptr_this->_writeIndex));

	return C_ERROR_OK;
}


/*  @ROLE    : This function returns the buffer pointer to the element to read
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_GetPrevReadBuffer(
														  /* INOUT */ T_CIRCULAR_BUFFER *
														  ptr_this,
																											/* IN    */ T_UINT32 prevElmtNumber,
																											/* 0 -> the last written elemt */
														  /*   OUT */ T_BUFFER * ptr_buffer)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i, decrease, eltIndex;

	if(CIRCULAR_BUFFER_IsEmpty(ptr_this))
	{
		*ptr_buffer = NULL;
		JUMP_TRACE(FIN, rid, C_ERROR_BUF_EMPTY,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
						C_TRACE_ERROR,
						"CIRCULAR_BUFFER_GetPrevReadBuffer() buffer is empty"));
	}

	/* check the element number */
	if(prevElmtNumber >= CIRCULAR_BUFFER_GetEltNumber(ptr_this))
	{
		*ptr_buffer = NULL;
		JUMP_TRACE(FIN, rid, C_ERROR_BUF_EMPTY,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
						C_TRACE_ERROR,
						"CIRCULAR_BUFFER_GetPrevReadBuffer() cannot get prevElmtNumber %d (buffer cointains %d)",
						prevElmtNumber, CIRCULAR_BUFFER_GetEltNumber(ptr_this)));
	}

	/* calculate the element index */
	eltIndex = ptr_this->_writeIndex;
	decrease = prevElmtNumber + 1;
	for(i = 0; i < decrease; i++)
	{
		eltIndex = SIZED_BUFFER_GetPrevIndex(&(ptr_this->_circularBuf), eltIndex);
	}

	/* return the buffer pointer */
	*ptr_buffer = SIZED_BUFFER_GetBufferPtr(&(ptr_this->_circularBuf), eltIndex);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY, C_TRACE_VALID,
				  "CIRCULAR_BUFFER_GetPrevReadBuffer() get buffer index %d",
				  eltIndex));

 FIN:
	return rid;
}


/*  @ROLE    : This function returns the buffer pointer to the first element to read
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_GetFirstReadBuffer(
															/* INOUT */ T_CIRCULAR_BUFFER *
															ptr_this,
															/*   OUT */ T_BUFFER * ptr_buffer)
{
	T_ERROR rid = C_ERROR_OK;

	if(CIRCULAR_BUFFER_IsEmpty(ptr_this))
	{
		*ptr_buffer = NULL;
		JUMP_TRACE(FIN, rid, C_ERROR_BUF_EMPTY,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
						C_TRACE_ERROR,
						"CIRCULAR_BUFFER_GetFirstReadBuffer() buffer is empty"));
	}

	/* return the buffer pointer */
	*ptr_buffer = SIZED_BUFFER_GetBufferPtr(&(ptr_this->_circularBuf),
														 ptr_this->_readIndex);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY, C_TRACE_VALID,
				  "CIRCULAR_BUFFER_GetFirstReadBuffer() get buffer index %d",
				  ptr_this->_readIndex));

 FIN:
	return rid;
}


/*  @ROLE    : This function returns the buffer pointer to the first written element
               and remove it
    @RETURN  : Error code */
T_ERROR CIRCULAR_BUFFER_GetAndRemoveReadBuffer(
																 /* INOUT */ T_CIRCULAR_BUFFER *
																 ptr_this,
																 /*   OUT */
																 T_BUFFER * ptr_buffer)
{
	T_ERROR rid = C_ERROR_OK;

	if(CIRCULAR_BUFFER_IsEmpty(ptr_this))
	{
		*ptr_buffer = NULL;
		JUMP_TRACE(FIN, rid, C_ERROR_BUF_EMPTY,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY,
						C_TRACE_VALID,
						"CIRCULAR_BUFFER_GetReadBuffer() buffer is empty"));
	}

	/* return the buffer pointer */
	*ptr_buffer = SIZED_BUFFER_GetBufferPtr(&(ptr_this->_circularBuf),
														 ptr_this->_readIndex);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_SHARED_MEMORY, C_TRACE_VALID,
				  "CIRCULAR_BUFFER_GetReadBuffer() get buffer index %d",
				  ptr_this->_readIndex));

	/* increase the buffer read index */
	ptr_this->_readIndex = SIZED_BUFFER_GetNextIndex(&(ptr_this->_circularBuf),
																	 ptr_this->_readIndex);

	SIZED_BUFFER_DecreaseElt(&(ptr_this->_circularBuf));

 FIN:
	return rid;
}

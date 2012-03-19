/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The SizedBuffer class creates a buffer with a variable size
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/
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

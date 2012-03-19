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
#ifndef SizedBuffer_e
#   define SizedBuffer_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"

typedef struct
{
	T_UINT32 _eltSize;			  /* the bytes size of one element */
	T_UINT32 _eltNumberMax;		  /* number of allocated elements */
	T_UINT32 _eltNumber;			  /* number of elements in buffer */
	T_BUFFER _buffer;				  /* the buffer */
} T_SIZED_BUFFER;

/*  @ROLE    : This function initialises the buffer
    @RETURN  : Error code */
extern T_ERROR SIZED_BUFFER_Init(
											  /* INOUT */ T_SIZED_BUFFER * ptr_this,
											  /* IN    */ T_UINT32 eltSize,
																								/* IN    */ T_UINT32 eltNumberMax);
																								/* in bytes */


/*  @ROLE    : This function deletes the buffer
    @RETURN  : Error code */
extern T_ERROR SIZED_BUFFER_Terminate(
													 /* INOUT */ T_SIZED_BUFFER * ptr_this);


/*  @ROLE    : returns the next buffer index */
#   define SIZED_BUFFER_GetNextIndex(ptr_this,index) \
  ((index+1)%((ptr_this)->_eltNumberMax))


/*  @ROLE    : returns the previous buffer index */
#   define SIZED_BUFFER_GetPrevIndex(ptr_this,index) \
  ((index != 0) ? (index - 1) : ((ptr_this)->_eltNumberMax - 1))

/*  @ROLE    : check if the buffer is empty */
#   define SIZED_BUFFER_IsEmpty(ptr_this) \
  (((ptr_this)->_eltNumber == 0) ? TRUE : FALSE)


/*  @ROLE    : check if the buffer is full */
#   define SIZED_BUFFER_IsFull(ptr_this) \
  (((ptr_this)->_eltNumber == (ptr_this)->_eltNumberMax) ? TRUE : FALSE)


/*  @ROLE    : increase the element number */
#   define SIZED_BUFFER_IncreaseElt(ptr_this) \
{ \
  if((ptr_this)->_eltNumber != (ptr_this)->_eltNumberMax) \
    (ptr_this)->_eltNumber++; \
}


/*  @ROLE    : decrease the element number */
#   define SIZED_BUFFER_DecreaseElt(ptr_this) \
{ \
  if((ptr_this)->_eltNumber != 0) \
     (ptr_this)->_eltNumber--; \
}


/*  @ROLE    : returns the number of element in buffer */
#   define SIZED_BUFFER_GetEltNumber(ptr_this) \
  ((ptr_this)->_eltNumber)

/*  @ROLE    : returns the size of an element in buffer */
#   define SIZED_BUFFER_GetEltSize(ptr_this) \
  ((ptr_this)->_eltSize)

/*  @ROLE    : returns the remainted number of element in buffer */
#   define SIZED_BUFFER_GetRemaintedEltNumber(ptr_this) \
  (((ptr_this)->_eltNumberMax) - ((ptr_this)->_eltNumber))


/*  @ROLE    : returns a pointer to the buffer index */
#   define SIZED_BUFFER_GetBufferPtr(ptr_this,index) \
  (((T_BYTE*)(ptr_this)->_buffer) + (index * (ptr_this)->_eltSize))


#endif /* SizedBuffer_e */

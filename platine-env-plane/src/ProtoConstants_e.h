/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : defines the commun protocol constants
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/
#ifndef ProtoConstants_e
#   define ProtoConstants_e

#   include "Types_e.h"

/* MSB 4 bits for component type, LSB 4 bits for instance id */
#   define MAKE_COMPONENT_ID(componentType,instanceId) \
  ((T_UINT8)(((T_UINT8)componentType & 0xF) << 4) | ((T_UINT8)instanceId & 0xF))

#   define EXTRACT_COMPONENT_ID(componentId,componentType,instanceId) \
{ \
  componentType = (componentId & 0xF0) >> 4; \
  instanceId = (componentId & 0x0F); \
}

#   define MEMCOPY_16_BITS(ptr_destination,ptr_source) \
{ \
  *(((T_UINT8*)(ptr_destination)) + 0) = *(((T_UINT8*)(ptr_source)) + 0); \
  *(((T_UINT8*)(ptr_destination)) + 1) = *(((T_UINT8*)(ptr_source)) + 1); \
}

#   define MEMCOPY_32_BITS(ptr_destination,ptr_source) \
{ \
  MEMCOPY_16_BITS(ptr_destination,ptr_source); \
  MEMCOPY_16_BITS((((T_UINT8*)(ptr_destination))+2),(((T_UINT8*)(ptr_source))+2)); \
}

#   define MEMCOPY_48_BITS(ptr_destination,ptr_source) \
{ \
  MEMCOPY_32_BITS(ptr_destination,ptr_source); \
  MEMCOPY_16_BITS((((T_UINT8*)(ptr_destination))+4),(((T_UINT8*)(ptr_source))+4)); \
}

#   define MEMCOPY_64_BITS(ptr_destination,ptr_source) \
{ \
  MEMCOPY_32_BITS(ptr_destination,ptr_source); \
  MEMCOPY_32_BITS((((T_UINT8*)(ptr_destination))+4),(((T_UINT8*)(ptr_source))+4)); \
}

#   define ALIGNED_SIZE(size,type) ((((size)+sizeof(type)-1)/sizeof(type))*sizeof(type))

#endif /* ProtoConstants_e */

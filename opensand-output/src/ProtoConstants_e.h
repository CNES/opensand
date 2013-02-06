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
 * @file ProtoConstants_e.h
 * @author TAS
 * @brief defines the commun protocol constants
 */

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

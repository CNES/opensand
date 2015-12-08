/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file UleExtPadding.h
 * @brief Optional Padding ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_EXT_PADDING_H
#define ULE_EXT_PADDING_H

#include <UleExt.h>


/**
 * @class UleExtPadding
 * @brief Optional Padding ULE extension
 */
class UleExtPadding: public UleExt
{
 public:

	/**
	 * Build a Padding ULE extension
	 */
	UleExtPadding();

	/**
	 * Destroy the Padding ULE extension
	 */
	~UleExtPadding();

	// implementation of virtual functions
	ule_ext_status build(uint16_t ptype, Data payload);
	ule_ext_status decode(uint8_t hlen, Data payload);
};

#endif

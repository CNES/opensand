/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file RleIdentifier.h
 * @brief RLE identifier (unique index given by the association of
 *        the Tal Id and Mac Id)
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef RLE_IDENT_H
#define RLE_IDENT_H

#include <stdint.h>


/**
 * @class RleIdentifier
 * @brief RLE identifier (unique index given by the association of
 *        the Tal Id and Mac Id)
 */
class RleIdentifier
{
 private:

	/// The source Tal Id value
	uint8_t src_tal_id;
	/// The destination Tal Id value
	uint8_t dst_tal_id;

 public:

	/**
	 * Build an identifier
	 *
	 * @param src_tal_id the source Tal Id
	 * @param dst_tal_id the destination Tal Id
	 */
	RleIdentifier(uint8_t src_tal_id, uint8_t dst_tal_id);

	/**
	 * Destroy the identifier
	 */
	~RleIdentifier();

	/**
	 * Get the source Tal Id
	 *
	 * @return the source Tal Id
	 */
	uint8_t getSrcTalId();

	/**
	 * Get the destination Tal Id
	 *
	 * @return the destination Tal Id
	 */
	uint8_t getDstTalId();
};

/**
 * @brief Operator to compare two RLE identifiers
 */
struct ltRleIdentifier
{
	/**
	 * Operator to test if one RLE identifier is lesser than another
	 * RLE identifier
	 *
	 * @param ai1 the first RLE identifier
	 * @param ai2 the second RLE identifier
	 * @return true if first RLE identifier is lesser than the second,
	 *         false otherwise
	 */
	bool operator() (RleIdentifier * ai1, RleIdentifier * ai2) const
	{
		if(ai1->getSrcTalId() == ai2->getSrcTalId())
		{
			return (ai1->getDstTalId() < ai2->getDstTalId());
		}
		return (ai1->getSrcTalId() < ai2->getSrcTalId());
	}
};

#endif

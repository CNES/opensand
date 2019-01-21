/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file GseIdentifier.h
 * @brief GSE identifier (unique index given by the association of
 *        the Tal Id and Mac Id and QoS of the packets)
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_IDENT_H
#define GSE_IDENT_H

#include <stdint.h>


/**
 * @class GseIdentifier
 * @brief GSE identifier (unique index given by the association of
 *        the Tal Id and Mac Id and QoS of the packets)
 */
class GseIdentifier
{
 private:

	/// The source Tal Id value
	uint8_t src_tal_id;
	/// The destination Tal Id value
	uint8_t dst_tal_id;
	/// the QoS value
	uint8_t qos;

 public:

	/**
	 * Build an GSE identifier
	 *
	 * @param src_tal_id the source Tal Id
	 * @param dst_tal_id the destination Tal Id
	 * @param qos        the Qos
	 */
	GseIdentifier(uint8_t src_tal_id, uint8_t dst_tal_id, uint8_t qos);

	/**
	 * Destroy the GSE identifier
	 */
	~GseIdentifier();

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

	/**
	 * Get the QoS
	 *
	 * @return the QoS
	 */
	uint8_t getQos();
};

/**
 * @brief Operator to compare two GSE identifiers
 */
struct ltGseIdentifier
{
	/**
	 * Operator to test if one GSE identifier is lesser than another
	 * GSE identifier
	 *
	 * @param ai1 the first GSE identifier
	 * @param ai2 the second GSE identifier
	 * @return true if first GSE identifier is lesser than the second,
	 *         false otherwise
	 */
	bool operator() (GseIdentifier * ai1, GseIdentifier * ai2) const
	{
		if(ai1->getSrcTalId() == ai2->getSrcTalId())
		{
			if(ai1->getDstTalId() == ai2->getDstTalId())
			{
				return (ai1->getQos() < ai2->getQos());
			}
			else
			{
				return (ai1->getDstTalId() < ai2->getDstTalId());
			}
		}
		else
		{
			return (ai1->getSrcTalId() < ai2->getSrcTalId());
		}
	}
};

#endif

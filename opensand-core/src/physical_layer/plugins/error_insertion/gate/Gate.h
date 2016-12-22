 /*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 CNES
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

/* @file Gate.h
 *  @brief Gate
 *  @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef GATE_ERROR_PLUGIN_H
#define GATE_ERROR_PLUGIN_H

#include "PhysicalLayerPlugin.h"

/**
 * @class Gate
 * @brief Gate
 */
class Gate: public ErrorInsertionPlugin
{

	public:

		/**
		 * @brief Channel ctor
		 */
		Gate();


		/**
		 * @brief Gate dtor
		 */
		~Gate();

		bool init();

		/**
		 * @brief Corrupt a package with error bits 
		 *
		 * @param payload the payload to the frame that should be modified 
		 * @return true if DVB header should be tagged as corrupted,
		 *         false otherwise
		 */
		bool modifyPacket(const Data &payload);

		/**
		 * @brief Determine if a Packet shall be corrupted or not depending on
		 *        the attenuationModel conditions 
		 *
		 * @param cn_total       The total C/N of the link
		 * @param threshold_qef  The minimal C/N of the link
		 *
		 * @return true if it must be corrupted, false otherwise 
		 */
		bool isToBeModifiedPacket(double cn_total,
		                          double threshold_qef);

};

CREATE(Gate, error_plugin, "Gate");

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
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
 * @file BlockPhysicalLayer.h
 * @brief A DVB physical layer block for regenerative satellite
 * @author Santiago PENA  <santiago.penaluque@cnes.fr>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 *
 * This block modifies the DVB frames sent/received on satellite terminals
 * depending on emulated physical conditions of the up and downlink
 *
 */

#ifndef BLOCK_PHYSICAL_LAYER_SAT_H
#define BLOCK_PHYSICAL_LAYER_SAT_H

#include "OpenSandCore.h"
#include "AttenuationHandler.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

#include <string>
#include <map>

using std::string;
using std::map;

/**
 * @class BlockPhysicalLayerSat
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayerSat: public Block
{
 public:
	/**
	 * @class Upward
	 * @brief Upward Physical Layer Channel
	 */
	class Upward: public RtUpward
	{
	 protected:
		/// Logs
		OutputLog *log_channel;
		OutputLog *log_event;

		/// The attenuation process
		AttenuationHandler *attenuation_hdl;

		/**
		 * @brief Forward the frame to the next channel
		 *
		 * @param dvb_frame  the DVB frame to forward
		 *
		 * @return true on success, false otherwise
		 */
		bool forwardPacket(DvbFrame *dvb_frame);

	 public:
		/**
		 * @brief Constructor of the ground upward physical channel
		 *
		 * @param name    the name of the channel
		 */
		Upward(const string &name);

		/**
		 * @brief Destroy the Channel
		 */
		virtual ~Upward();

		/**
		 * @brief Initialize the ground upward physical channel
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool onInit();

		/**
		 * @brief Event processing
		 *
		 * @param event  the event to process
		 *
		 * @return true on success, false otherwise
		 */
		bool onEvent(const RtEvent *const event);
	};

	/**
	 * @class Downward
	 * @brief Downward Physical Layer Channel
	 */
	class Downward : public RtDownward
	{
	 protected:
		/// Logs
		OutputLog *log_event;

	 public:
		/**
		 * @brief Constructor of the ground downward physical channel
		 *
		 * @param name  the name of the channel
		 */
		Downward(const string &name);

		/**
		 * @brief Destroy the Channel
		 */
		virtual ~Downward()
		{
		}

		/**
		 * @brief Event processing
		 *
		 * @param event  the event to process
		 *
		 * @return true on success, false otherwise
		 */
		bool onEvent(const RtEvent *const event);
	};

 public:
	/**
	 * Build a physical layer block for satellite
	 *
	 * @param name            The name of the block
	 */
	BlockPhysicalLayerSat(const string &name):
		Block(name)
	{
	}

	/**
	 * Destroy the PhysicalLayerSat block
	 */
	virtual ~BlockPhysicalLayerSat() {}
	
	// initialization method
	bool onInit()
	{
		return true;
	}
};

#endif



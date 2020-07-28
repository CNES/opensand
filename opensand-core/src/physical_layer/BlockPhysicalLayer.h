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
 * @brief A DVB physical layer block
 * @author Santiago PENA  <santiago.penaluque@cnes.fr>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 *
 * This block modifies the DVB frames sent/received on satellite terminals
 * depending on emulated physical conditions of the up and downlink
 *
 */

#ifndef BLOCK_PHYSICAL_LAYER_H
#define BLOCK_PHYSICAL_LAYER_H

#include "OpenSandCore.h"
#include "GroundPhysicalChannel.h"
#include "AttenuationHandler.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

#include <string>
#include <map>

using std::string;
using std::map;

/**
 * @class BlockPhysicalLayer
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayer: public Block
{
 public:
	/**
	 * @class Upward
	 * @brief Ground Upward Physical Layer Channel
	 */
	class Upward: public GroundPhysicalChannel, public RtUpward
	{
	 private:
		/// Probes
     std::shared_ptr<Probe<float>> probe_total_cn;

	 protected:
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

		/**
		 * @brief Get the C/N fot the current DVB frame
		 *
		 * @param dvb_frame  the current DVB frame
		 *
		 * @return the current C/N
		 */
		double getCn(DvbFrame *dvb_frame) const;

	 public:
		/**
		 * @brief Constructor of the ground upward physical channel
		 *
		 * @param name    the name of the channel
		 * @param mac_id  the id of the ST or of the GW
		 */
		Upward(const string &name, tal_id_t mac_id);

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
	 * @brief Ground Downward Physical Layer Channel
	 */
	class Downward : public GroundPhysicalChannel, public RtDownward
	{
	 private:
		/// Probes
     std::shared_ptr<Probe<int>> probe_delay;

	 protected:
		/// Event
		event_id_t delay_update_timer;

		/**
		 * @brief Update the delay
		 *
		 * @return true on success, false otherwise
		 */
		bool updateDelay();

		/**
		 * @brief Forward the frame to the next channel
		 *
		 * @param dvb_frame  the DVB frame to forward
		 *
		 * @return true on success, false otherwise
		 */
		bool forwardPacket(DvbFrame *dvb_frame);

		/**
		 * @brief Prepare the frame
		 *
		 * @param dvb_frame  the DVB frame to forward
		 */
		void preparePacket(DvbFrame *dvb_frame);

	 public:
		/**
		 * @brief Constructor of the ground downward physical channel
		 *
		 * @param name  the name of the channel
		 * @param mac_id  the id of the ST or of the GW
		 */
		Downward(const string &name, tal_id_t mac_id);

		/**
		 * @brief Destroy the Channel
		 */
		virtual ~Downward()
		{
		}

		/**
		 * @brief Initialize the ground downward physical channel
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

 public:

	/**
	 * Build a physical layer block
	 *
	 * @param name            The name of the block
	 * @param mac_id          The mac id of the terminal
	 */
	BlockPhysicalLayer(const string &name, tal_id_t mac_id);

	/**
	 * Destroy the PhysicalLayer block
	 */
	~BlockPhysicalLayer();

	// initialization method
	bool onInit();

 private:
	/// The terminal mac_id
	tal_id_t mac_id;

	/// The satellite delay for this terminal
	SatDelayPlugin *satdelay;
};

#endif



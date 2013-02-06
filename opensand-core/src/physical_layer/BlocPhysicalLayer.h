/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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
 * @file BlocPhysicalLayer.h
 * @brief A DVB physical layer block
 * @author Santiago PENA  <santiago.penaluque@cnes.fr>
 *
 * This block modifies the DVB frames sent/received on satellite terminals
 * depending on emulated physical conditions of the up and downlink
 *
 */

#ifndef BLOCK_PHYSICAL_LAYER_H
#define BLOCK_PHYSICAL_LAYER_H


#include "Channel.h"
#include "OpenSandCore.h"
#include "PluginUtils.h"

#include <opensand_margouilla/mgl_bloc.h>
#include <opensand_output/Output.h>

#include <map>


/**
 * @class BlocPhysicalLayer
 * @brief Basic DVB PhysicalLayer block
 */
class BlocPhysicalLayer: public mgl_bloc
{
	private:

		// Channel parameters
		mgl_timer channel_timer;
		int granularity;
		Channel *channel_downlink;
		Channel *channel_uplink; 
		component_t component_type; // Terminal type: ST, SAT or GW
		string satellite_type; // Satellite in Regenerative or Transparent mode
		PluginUtils utils;

	public:

		/**
		 * Build a physical layer block
		 *
		 * @param blocmgr  The block manager
		 * @param fatherid The father of the block
		 * @param name     The name of the block
		 * @param type     Type of terminal: ST, SAT, GW
		 * @param utils    The plugins elements
		 */
		BlocPhysicalLayer(mgl_blocmgr *blocmgr,
		                  mgl_id fatherid,
		                  const char *name,
		                  component_t type,
		                  PluginUtils utils);

		/**
		 * Destroy the PhysicalLayer block
		 */
		~BlocPhysicalLayer();

		/**
		 * Handle the events
		 *
		 * @param event    The event to handle
		 * @return Whether the event was successfully handled or not
		 */
		mgl_status onEvent(mgl_event *event);

	private:

		/**
		 * Initiate the bloc
		 *
		 * @return true on success, false otherwise
		 */
		bool onInit();

		/**
		 * @brief Initialize timers
		 */
		bool initTimers();

		/**
		 * Forward a DVB frame to a destination block
		 *
		 * @param dest_block    Destination block to send the DVB frame to
		 * @param dvb_frame     The DVB frame to send
		 * @param dvb_frame_len The length of the DVB frame to send

		 * @return Whether the DVB frame was successfully sent or not
		 */
		mgl_status forwardMetaFrame(mgl_id dest_block,
		                            T_DVB_META *dvb_meta,
		                            long l_len);

		/// whether the block is initialized
		bool init_ok;


		/// output events
		static Event *error_init;
		static Event *init_done;
};

#endif


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file BlockPhysicalLayerSat.h
 * @brief A DVB physical layer block
 * @author Santiago PENA  <santiago.penaluque@cnes.fr>
 *
 * This block modifies the DVB frames sent/received on satellite terminals
 * depending on emulated physical conditions of the up and downlink
 *
 */

#ifndef BLOCK_PHYSICAL_LAYER_SAT_H
#define BLOCK_PHYSICAL_LAYER_SAT_H


#include "BlockPhysicalLayer.h"
#include "PhyChannel.h"
#include "OpenSandCore.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

#include <map>


/**
 * @class BlockPhysicalLayerSat
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayerSat: public Block
{
 public:

	/**
	 * Build a physical layer block
	 *
	 * @param name            The name of the block
	 */
	BlockPhysicalLayerSat(const string &name);

	/**
	 * Destroy the PhysicalLayer block
	 */
	~BlockPhysicalLayerSat();

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

	class PhyUpward: public Chan
	{
	  public:
		PhyUpward(Block &bl):
			Chan(bl, upward_chan)
		{};

		bool onInit(void);

	  protected:
		bool forwardMetaFrame(T_DVB_META *dvb_meta,
		                      size_t len);
	};

	class PhyDownward: public Chan
	{
	  public:
		PhyDownward(Block &bl):
			Chan(bl, downward_chan)
		{};

	  protected:
		bool forwardMetaFrame(T_DVB_META *dvb_meta,
		                      size_t len);
	};

 private:

	/**
	 * @brief Global event function for both upward and downward channels
	 *
	 * @param event  The event
	 * @param chan   The channel
	 */
	bool onEvent(const RtEvent *const event, Chan *chan);

	/// output events
	static Event *error_init;
	static Event *init_done;
};

#endif


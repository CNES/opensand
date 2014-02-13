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
 * @file BlockPhysicalLayer.h
 * @brief A DVB physical layer block
 * @author Santiago PENA  <santiago.penaluque@cnes.fr>
 *
 * This block modifies the DVB frames sent/received on satellite terminals
 * depending on emulated physical conditions of the up and downlink
 *
 */

#ifndef BLOCK_PHYSICAL_LAYER_H
#define BLOCK_PHYSICAL_LAYER_H


#include "PhyChannel.h"
#include "OpenSandCore.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

#include <map>

class BlockPhysicalLayerSat;


class Chan: public RtChannel, PhyChannel
{
	friend class BlockPhysicalLayer;
	friend class BlockPhysicalLayerSat;
 public:
	Chan(Block *const bl, chan_type_t chan_type):
		RtChannel(bl, chan_type),
		PhyChannel()
	{};

 protected:
	virtual bool forwardFrame(DvbFrame *dvb_frame) = 0;

};

/**
 * @class BlockPhysicalLayer
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayer: public Block
{
 public:

	/**
	 * Build a physical layer block
	 *
	 * @param name            The name of the block
	 */
	BlockPhysicalLayer(const string &name);

	/**
	 * Destroy the PhysicalLayer block
	 */
	~BlockPhysicalLayer();

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

	class Upward: public Chan
	{
	  public:
		Upward(Block *const bl):
			Chan(bl, upward_chan)
		{};

		virtual bool onInit(void);

	  protected:
		bool forwardFrame(DvbFrame *dvb_frame);
		/// the expected message type (for filtering on GW)
		uint8_t msg_type;
	};

	class Downward: public Chan
	{
	  public:
		Downward(Block *const bl):
			Chan(bl, downward_chan)
		{};

		virtual bool onInit(void);

	  protected:
		bool forwardFrame(DvbFrame *dvb_frame);
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

/**
 * @class BlockPhysicalLayerSat
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayerSat: public BlockPhysicalLayer
{
 public:
	BlockPhysicalLayerSat(const string &name):
		BlockPhysicalLayer(name)
	{};
	class Upward: public BlockPhysicalLayer::Upward
	{
	  public:
		Upward(Block *const bl):
			BlockPhysicalLayer::Upward(bl)
		{};

		bool onInit(void);
	};

	class Downward: public BlockPhysicalLayer::Downward
	{
	  public:
		Downward(Block *const bl):
			BlockPhysicalLayer::Downward(bl)
		{};

		bool onInit(void);
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



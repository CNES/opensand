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


class Chan: public RtChannel, PhyChannel
{
	friend class BlockPhysicalLayer;
 public:
	Chan(Block &bl):
		RtChannel(bl, upward_chan),
		PhyChannel()
	{};

 protected:
	/// Timer id for attenuation update
	event_id_t att_timer;

	/// Satellite in Regenerative or Transparent mode
	sat_type_t satellite_type;

	/// type of host: ST, SAT or GW
	component_t component_type;

	/**
	 * @brief Common channel initialization part
	 *
	 * @param link  The name of the link
	 * @return true on success, false otherwise
	 */
	bool initChan(const string &link);

	/**
	 * Forward a DVB frame to a destination block
	 *
	 * @param dvb_frame     The DVB frame to send
	 * @param dvb_frame_len The length of the DVB frame to send
	 * @return Whether the DVB frame was successfully sent or not
	 */
	virtual bool forwardMetaFrame(T_DVB_META *dvb_meta,
	                              long l_len) = 0;

};

/**
 * @class BlockPhysicalLayer
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayer: public Block
{
 private:

	/// type of host: ST, SAT or GW
	component_t component_type;

 public:

	/**
	 * Build a physical layer block
	 *
	 * @param name            The name of the block
	 * @param component_type  The type of host
	 */
	BlockPhysicalLayer(const string &name, component_t component_type);

	/**
	 * Destroy the PhysicalLayer block
	 */
	~BlockPhysicalLayer();

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

	/**
	 * @brief Global event function for both upward and downward channels
	 *
	 * @param event  The event
	 * @param chan   The channel
	 */
	bool onEvent(const RtEvent *const event, Chan *chan);

	class PhyUpward: public Chan
	{
	  public:
		PhyUpward(Block &bl):
			Chan(bl)
		{};

		bool onInit(void);

	  protected:
		bool forwardMetaFrame(T_DVB_META *dvb_meta,
		                      long l_len);
	};

	class PhyDownward: public Chan
	{
	  public:
		PhyDownward(Block &bl):
			Chan(bl)
		{};

		bool onInit(void);

	  protected:
		bool forwardMetaFrame(T_DVB_META *dvb_meta,
		                      long l_len);
	};


 private:

	/// output events
	static Event *error_init;
	static Event *init_done;
};

class BlockPhysicalLayerTal: public BlockPhysicalLayer
{
 public:
	BlockPhysicalLayerTal(const string &name):
		BlockPhysicalLayer(name, terminal)
	{};
};

class BlockPhysicalLayerGw: public BlockPhysicalLayer
{
 public:
	BlockPhysicalLayerGw(const string &name):
		BlockPhysicalLayer(name, gateway)
	{};
};

class BlockPhysicalLayerSat: public BlockPhysicalLayer
{
 public:
	BlockPhysicalLayerSat(const string &name):
		BlockPhysicalLayer(name, satellite)
	{};
};

#endif


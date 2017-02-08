/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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

/**
 * @file BlockSatCarrierSat.h
 * @brief This bloc implements a satellite carrier emulation
 * @author AQL (ame)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef BlockSatCarrierSat_H
#define BlockSatCarrierSat_H

#include "sat_carrier_channel_set.h"
#include "SatDelayMap.h"
#include "SatCarrierFifo.h"
#include "DvbFrame.h"
#include "NetContainer.h"

#include <opensand_rt/Rt.h>


struct sc_specific
{
	tal_id_t tal_id;     ///< the terminal id for terminal
	string ip_addr;      ///< the IP address for emulation
	string emu_iface;    ///< the name of the emulation interface
};

/**
 * @class BlockSatCarrierSat
 * @brief This bloc implements a satellite carrier emulation
 */
class BlockSatCarrierSat: public Block
{
 public:

	/**
	 * @brief The satellite carrier block
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockSatCarrierSat(const string &name,
	                struct sc_specific specific);

	~BlockSatCarrierSat();

	class Upward: public RtUpward
	{
	 public:
		Upward(const string &name, struct sc_specific specific):
			RtUpward(name),
			ip_addr(specific.ip_addr),
			interface_name(specific.emu_iface),
			tal_id(specific.tal_id),
			in_channel_set(specific.tal_id),
			sat_delay_map(NULL),
			fifo_timer(-1),
			fifo()
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);
		void setSatDelay(SatDelayMap *sat_delay_map);

	 private:
		/// the IP address for emulation newtork
		string ip_addr;
		/// the interface name for emulation newtork
		string interface_name;
		/// the terminal id for the emulation newtork
		tal_id_t tal_id;
		/// List of input channels
		sat_carrier_channel_set in_channel_set;
		/// the satellite delay map
		SatDelayMap *sat_delay_map;
		/// the timer to check if there's a new ready item in fifo
		event_id_t fifo_timer;
		/// the fifo that implements the delay
		SatCarrierFifo fifo;

		/**
		 * @brief Handle a packt received from carrier
		 *
		 * @param carrier_id  The carrier of the packet
		 * @param data        The data read on socket
		 * @param length      The data length
		 */
		void onReceivePktFromCarrier(uint8_t carrier_id,
		                             spot_id_t spot_id,
		                             unsigned char *data,
		                             size_t length);

		// FIFO timer handler
		bool handleFifoTimer();

		// send Frame to carriers
		bool sendFrame(DvbFrame *dvb_frame);

		// Push Frame in FIFO
		bool pushInFifo(NetContainer *data, time_ms_t delay);
	};

	class Downward: public RtDownward
	{
	 public:
		Downward(const string &name, struct sc_specific specific):
			RtDownward(name),
			ip_addr(specific.ip_addr),
			interface_name(specific.emu_iface),
			tal_id(specific.tal_id),
			out_channel_set(specific.tal_id),
			sat_delay_map(NULL),
			delays_timer(-1),
			fifo_timer(-1),
			fifo()
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);
		void setSatDelay(SatDelayMap *sat_delay_map);

	 private:
		/// the IP address for emulation newtork
		string ip_addr;
		/// the interface name for emulation newtork
		string interface_name;
		/// the terminal id for the emulation newtork
		tal_id_t tal_id;
		/// List of output channels
		sat_carrier_channel_set out_channel_set;
		/// the satellite delay map
		SatDelayMap *sat_delay_map;
		/// the refresh delays timer
		event_id_t delays_timer;
		/// the timer to check if there's a new ready item in fifo
		event_id_t fifo_timer;
		/// the fifo that implements the delay
		SatCarrierFifo fifo;
	
		// FIFO timer handler
		bool handleFifoTimer();

		// send Frame to carriers
		bool sendFrame(DvbFrame *dvb_frame);

		// Push Frame in FIFO
		bool pushInFifo(NetContainer *data, time_ms_t delay);
	};

 protected:

	// initialization method
	bool onInit();

	// the sat delay map
	SatDelayMap sat_delay_map;
};

#endif

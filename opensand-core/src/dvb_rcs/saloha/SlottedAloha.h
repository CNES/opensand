/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file SlottedAloha.h
 * @brief The Slotted Aloha scheduling
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

#ifndef SALOHA_H
#define SALOHA_H

// TODO move in SlottedAlohaNcc
#include "SlottedAlohaMethod.h"
#include "TerminalCategorySaloha.h"
#include "EncapPlugin.h"


#include <opensand_output/Output.h>


/**
 * @class SlottedAloha
 * @brief The Slotted Aloha scheduling
*/
class SlottedAloha
{
 protected:
	/// Number of superframe per Slotted Aloha Frame
	time_sf_t sf_per_saframe;

	/// The frame duration
	time_ms_t frame_duration_ms;

	/// Number of replicas per packet
	uint16_t nb_replicas;

	/// Number of packets received per frame
//	uint16_t nb_packets_received_per_frame;

	/// Number total of packets received since the begining
//	uint64_t nb_packets_received_total;

	/// Check whether the parent is correctly initialized
	bool is_parent_init;

	/// The encap packet handler
	EncapPlugin::EncapPacketHandler *pkt_hdl;

	/// The terminal categories
	TerminalCategories<TerminalCategorySaloha> categories;

	/// The terminal affectation
	TerminalMapping<TerminalCategorySaloha> terminal_affectation;

	/// The default terminal category
	TerminalCategorySaloha *default_category;

 public:
	/**
	 * Build the slotted aloha class
	 */
	SlottedAloha();

	/**
	 * Class destructor
	 */
	~SlottedAloha();

	/**
	 * Init the Slotted Aloha parent class
	 *
	 * @param frame_duration_ms      The frame duration (ms)
	 * @param pkt_hdl                The handler for encap packet
	 * @param categories             The terminal categories
	 * @param terminal_affectation   The terminal affectation
	 * @param default_category       The default terminan category
	 */
	bool initParent(time_ms_t frame_duration_ms,
	                EncapPlugin::EncapPacketHandler *const pkt_hdl,
	                TerminalCategories<TerminalCategorySaloha> &categories,
	                TerminalMapping<TerminalCategorySaloha> terminal_affectation,
	                TerminalCategorySaloha *default_category);
	// TODO affectation and default useful ?????

	/**
	 * Handle a received Slotted Aloha frame
	 *
	 * @param frame  The received frame
	 */
	virtual bool onRcvFrame(DvbFrame *frame) = 0;

 protected:

	/**
	 * Build an <ID, Seq, PDU_nb, QoS> id of a Slotted Aloha data packet
	 *
	 * @param packet  Slotted Aloha data packet to build identifiant
	 * @return The id of the Slotted Aloha data packet given in parameter
	 */
	saloha_id_t buildPacketId(SlottedAlohaPacketData *packet);

	/**
	 * Convert a Slotted Aloha data packet <ID, Seq, PDU_nb, QoS> id to
	 * integers table
	 *
	 * @param id   Slotted Aloha data packet id
	 * @paarm ids  OUT: table containing packet ID elements
	 * @return integers vector
	 */
	void convertPacketId(saloha_id_t id, uint16_t ids[4]);

	/**
	 * Return check if current tick is a Slotted Aloha frame tick
	 *
	 * @param superframe_counter  counter of superfframes since the begining
	 *                            (the current superframe)
	 * @return true if current tick is a Slotted Aloha frame tick, false otherwise
	 */
	bool isSuperFrameTick(time_sf_t superframe_counter);

	/// The slotted aloha logger
	OutputLog *log_saloha;
	/// The init logger
	OutputLog *log_init;
	//TODO probes !! and updateStats function

 public:
//	const char *hexa(Data input);
	// TODO REMOVE DEBUG
/*	void debug(const char *title, SlottedAlohaPacketData *packet);
	void debug(const char *title, SlottedAlohaPacketCtrl *packet);
	virtual void debugFifo(const char *title) = 0;*/
};

#endif


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file BlockLanAdaptation.h
 * @brief Interface between network interfaces and OpenSAND
 *
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Nicol <julien.nicol@b2i-toulouse.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef BLOCK_LAN_ADAPTATION_H
#define BLOCK_LAN_ADAPTATION_H


#include <memory>

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>

#include "OpenSandCore.h"
#include "LanAdaptationPlugin.h"
#include "SarpTable.h"
#include "DelayFifo.h"


class PacketSwitch;
class IslDelayPlugin;


struct la_specific
{
	std::string tap_iface;
	std::shared_ptr<IslDelayPlugin> delay = nullptr;
	tal_id_t connected_satellite = 0;
	bool is_used_for_isl = false;
	std::shared_ptr<PacketSwitch> packet_switch = nullptr;
};


template<>
class Rt::UpwardChannel<class BlockLanAdaptation>: public Channels::Upward<UpwardChannel<BlockLanAdaptation>>
{
 public:
	UpwardChannel(const std::string &name, la_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const TimerEvent& event) override;
	bool onEvent(const MessageEvent& event) override;

	/**
	 * @brief Set the lan adaptation contexts for channels
	 *
	 * @param contexts the lan adaptation contexts
	 */
	void setContexts(const lan_contexts_t &contexts);

	/**
	 * @brief Set the network socket file descriptor
	 *
	 * @param fd  The socket file descriptor
	 */
	void setFd(int fd);

 private:
	/**
	 * @brief Handle a message from lower block
	 *  - build the TAP header with appropriate protocol identifier
	 *  - write TAP header + packet to TAP interface or delay packet before writting
	 *
	 * @param burst  The burst of packets
	 * @return true on success, false otherwise
	 */
	bool onMsgFromDown(Ptr<NetBurst> burst);

	/**
	 * @brief Actually write the TAP header + packet to TAP interface
	 *
	 * @param packet  Data to write on the TAP interface
	 * @return true on success, false otherwise
	 */
	bool writePacket(const Data& packet);

	/// SARP table
	SarpTable sarp_table;

	/// TAP file descriptor
	int fd;

	/// the contexts list from lower to upper context
	lan_contexts_t contexts;

	/// The MAC layer MAC id received through msg_link_up
	tal_id_t tal_id;

	/// State of the satellite link
	SatelliteLinkState state;

	// The Packet Switch including packet forwarding logic and SARP
	std::shared_ptr<PacketSwitch> packet_switch;

	// Delay before writting on the TAP
	std::shared_ptr<IslDelayPlugin> delay;

	// Polling event to implement delay before writting on the TAP
	event_id_t delay_timer;

	// Fifo to implement delay before writting on the TAP
	DelayFifo delay_fifo;
};


template<>
class Rt::DownwardChannel<class BlockLanAdaptation>: public Channels::Downward<DownwardChannel<BlockLanAdaptation>>
{
 public:
	DownwardChannel(const std::string &name, la_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const FileEvent& event) override;
	bool onEvent(const TimerEvent& event) override;
	bool onEvent(const MessageEvent& event) override;

	/**
	 * @brief Set the lan adaptation contexts for channels
	 *
	 * @param contexts the lan adaptation contexts
	 */
	void setContexts(const lan_contexts_t &contexts);

	/**
	 * @brief Set the network socket file descriptor
	 *
	 * @param fd  The socket file descriptor
	 */
	void setFd(int fd);

 private:
	/// statistic timer
	event_id_t stats_timer;

	///  The period for statistics update
	time_ms_t stats_period_ms;

	/// the contexts list from lower to upper context
	lan_contexts_t contexts;

	/// The MAC layer MAC id received through msg_link_up
	tal_id_t tal_id;

	/// State of the satellite link
	SatelliteLinkState state;

	// The Packet Switch including packet forwarding logic and SARP
	std::shared_ptr<PacketSwitch> packet_switch;
};


/**
 * @class BlockLanAdaptation
 * @brief Interface between network interfaces and OpenSAND
 */
class BlockLanAdaptation: public Rt::Block<BlockLanAdaptation, la_specific>
{
public:
	BlockLanAdaptation(const std::string &name, la_specific specific);

	static void generateConfiguration();

	// initialization method
	bool onInit() override;

private:
	/// The TAP interface name
	std::string tap_iface;

	/**
	 * Create or connect to an existing TAP interface
	 *
	 * @param fd  OUT: the file descriptor
	 * @return  true on success, false otherwise
	 */
	bool allocTap(int &fd);
};


#endif

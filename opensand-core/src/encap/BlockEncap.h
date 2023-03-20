/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file BlockEncap.h
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien Delrieu <adelrieu@toulouse.viveris.com>
 */

#ifndef BLOCK_ENCAP_H
#define BLOCK_ENCAP_H

#include "OpenSandCore.h"
#include "EncapPlugin.h"

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>


class LanAdaptationPlugin;


struct EncapConfig
{
	tal_id_t entity_id;
	Component entity_type;
	bool filter_packets;   // if true, the block will drop packets whose destination is not the entity_id
	bool scpc_enabled;     // only used when entity_type == terminal
};


class EncapChannel
{
 protected:
	EncapChannel();

	/// it is the MAC layer group id received through msg_link_up
	group_id_t group_id;

	/// it is the MAC layer MAC id received through msg_link_up
	tal_id_t tal_id;

	/// State of the satellite link
	SatelliteLinkState state;
};


template<>
class Rt::UpwardChannel<class BlockEncap>: public Channels::Upward<UpwardChannel<BlockEncap>>, public EncapChannel
{
 public:
	UpwardChannel(const std::string &name, EncapConfig encap_cfg);

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const MessageEvent &event) override;

	void setContext(const encap_contexts_t &encap_ctx);
	void setSCPCContext(const encap_contexts_t &encap_ctx_scpc);

	void setMacId(tal_id_t id);

 private:
	/// the reception contexts list from upper to lower context
	encap_contexts_t ctx;
	/// the reception contexts list from upper to lower context for SCPC mode
	encap_contexts_t ctx_scpc;

	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;
	Component entity_type;

	/// if true, the block will drop packets whose destination is not the entity_id
	bool filter_packets;

	/// the SCPC encapsulation lower item
	std::string scpc_encap;

 protected:
	/// the MAC ID of the ST (as specified in configuration)
	/**
	 * Handle a burst of encapsulation packets received from the lower-layer
	 * block
	 *
	 * @param burst  The burst received from the lower-layer block
	 * @return       Whether the burst was successful handled or not
	 */
	bool onRcvBurst(Ptr<NetBurst> burst);
};


template<>
class Rt::DownwardChannel<class BlockEncap>: public Channels::Downward<DownwardChannel<BlockEncap>>, public EncapChannel
{
 public:
	DownwardChannel(const std::string &name, EncapConfig encap_cfg);

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const TimerEvent &event) override;
	bool onEvent(const MessageEvent &event) override;

	void setContext(const encap_contexts_t &encap_ctx);

 private:
	/// the emission contexts list from lower to upper context
	encap_contexts_t ctx;

	/// Expiration timers for encapsulation contexts
	std::map<event_id_t, int> timers;

	/**
	 * Handle a burst received from the upper-layer block
	 *
	 * @param burst  The burst received from the upper-layer block
	 * @return        Whether the IP packet was successful handled or not
	 */
	bool onRcvBurst(Ptr<NetBurst> burst);

	/**
	 * Handle the timer event
	 *
	 * @param timer_id  The id of the timer to handle
	 * @return          Whether the timer event was successfully handled or not
	 */
	bool onTimer(event_id_t timer_id);
};


/**
 * @class BlockEncap
 * @brief Generic Encapsulation Bloc
 */
class BlockEncap: public Rt::Block<BlockEncap, EncapConfig>
{
 public:
	/**
	 * Build an encapsulation block
	 *
	 * @param name  The name of the block
	 * @param name  The mac id of the terminal
	 */
	BlockEncap(const std::string &name, EncapConfig encap_cfg);

	static void generateConfiguration();

 protected:
	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;
	Component entity_type;

	bool scpc_enabled;

	/**
	 *
	 * Get the Encapsulation context of the Up/Return or the Down/Forward link
	 *
	 * @param scheme_list   The name of encapsulation scheme list
	 * @param l_plugin      The LAN adaptation plugin
	 * @ctx                 The encapsulation context for return/up or forward/down links
	 * @link_type           The type of link: "return/up" or "forward/down"
	 * @return              Whether the Encapsulation context has been
	 *                      correctly obtained or not
	 */
	bool getEncapContext(EncapSchemeList scheme_list,
	                     LanAdaptationPlugin *l_plugin,
	                     encap_contexts_t &ctx,
	                     const char *link_type);

	/**
	 *
	 * Get the Encapsulation context of the SCPC Return link
	 *
	 * @param l_plugin         The LAN adaptation plugin
	 * @param ctx              The encapsulation context for return link
	 * @param link_type        The type of link: "return/up" or "forward/down"
	 * @return                 Whether the Encapsulation context has been
	 *                         correctly obtained or not
	 */
	bool getSCPCEncapContext(LanAdaptationPlugin *l_plugin,
	                         encap_contexts_t &ctx,
	                         const char *link_type);

	/// initialization method
	bool onInit() override;
};

#endif

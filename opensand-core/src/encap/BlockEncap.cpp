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
 * @file BlockEncap.cpp
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien Delrieu <adelrieu@toulouse.viveris.com>
 */


#include "BlockEncap.h"

#include "Plugin.h"
#include "LanAdaptationPlugin.h"
#include "Ethernet.h"
#include "NetBurst.h"
#include "NetPacket.h"
#include "OpenSandModelConf.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>
#include <opensand_rt/TimerEvent.h>
#include <opensand_rt/MessageEvent.h>

#include <algorithm>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <limits>


/**
 * @brief Check if a file exists
 *
 * @return true if the file is found, false otherwise
 */
inline bool fileExists(const std::string &filename)
{
	if(access(filename.c_str(), R_OK) < 0)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot access '%s' file (%s)\n",
		        filename.c_str(), strerror(errno));
		return false;
	}
	return true;
}


EncapChannel::EncapChannel():
	group_id{std::numeric_limits<decltype(group_id)>::max()},
	tal_id{std::numeric_limits<decltype(tal_id)>::max()},
	state{SatelliteLinkState::DOWN}
{
}


BlockEncap::BlockEncap(const std::string &name, EncapConfig encap_cfg):
	Rt::Block<BlockEncap, EncapConfig>{name, encap_cfg},
	mac_id{encap_cfg.entity_id},
	entity_type{encap_cfg.entity_type},
	scpc_enabled{encap_cfg.scpc_enabled}
{
	// register static log (done in Entity.cpp for now)
	// NetBurst::log_net_burst = Output::Get()->registerLog(LEVEL_WARNING, "NetBurst");
}


Rt::DownwardChannel<BlockEncap>::DownwardChannel(const std::string &name, EncapConfig):
	Channels::Downward<DownwardChannel<BlockEncap>>{name},
	EncapChannel{}
{
}


Rt::UpwardChannel<BlockEncap>::UpwardChannel(const std::string &name, EncapConfig encap_cfg):
	Channels::Upward<UpwardChannel<BlockEncap>>{name},
	EncapChannel{},
	mac_id{encap_cfg.entity_id},
	entity_type{encap_cfg.entity_type},
	filter_packets{encap_cfg.filter_packets},
	scpc_encap{""}
{
}


void BlockEncap::generateConfiguration()
{
	Plugin::generatePluginsConfiguration(nullptr,
	                                     PluginType::Encapsulation,
	                                     "encapsulation_scheme",
	                                     "Encapsulation Scheme");
}


bool Rt::DownwardChannel<BlockEncap>::onEvent(const Event &event)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "unknown event received %s\n",
	    event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<BlockEncap>::onEvent(const TimerEvent &event)
{
	// timer event, flush corresponding encapsulation context
	LOG(this->log_receive, LEVEL_INFO,
	    "Timer received %s\n",
	    event.getName().c_str());
	return this->onTimer(event.getFd());
}


bool Rt::DownwardChannel<BlockEncap>::onEvent(const MessageEvent &event)
{
	// message received from another bloc
	LOG(this->log_receive, LEVEL_INFO,
	    "message received from the upper-layer bloc\n");
	
	InternalMessageType msg_type = to_enum<InternalMessageType>(event.getMessageType());
	if(msg_type == InternalMessageType::link_up)
	{
		// 'link up' message received 
		Ptr<T_LINK_UP> link_up_msg = event.getMessage<T_LINK_UP>();

		// save group id and TAL id sent by MAC layer
		this->group_id = link_up_msg->group_id;
		this->tal_id = link_up_msg->tal_id;
		this->state = SatelliteLinkState::UP;
		return true;
	}

	if (msg_type == InternalMessageType::sig)
	{
		return this->enqueueMessage(event.getMessage<void>(), event.getMessageType());
	}

	return this->onRcvBurst(event.getMessage<NetBurst>());
}


void Rt::DownwardChannel<BlockEncap>::setContext(const std::vector<EncapPlugin::EncapContext *> &encap_ctx)
{
	this->ctx = encap_ctx;
}


bool Rt::UpwardChannel<BlockEncap>::onEvent(const Event &event)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "unknown event received %s\n",
	    event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<BlockEncap>::onEvent(const MessageEvent &event)
{
	LOG(this->log_receive, LEVEL_INFO,
	    "message received from the lower layer\n");

	InternalMessageType msg_type = to_enum<InternalMessageType>(event.getMessageType());
	if(msg_type == InternalMessageType::link_up)
	{
		std::vector<EncapPlugin::EncapContext*>::iterator encap_it;

		// 'link up' message received => forward it to upper layer
		Ptr<T_LINK_UP> link_up_msg = event.getMessage<T_LINK_UP>();
		LOG(this->log_receive, LEVEL_INFO,
		    "'link up' message received (group = %u, tal = %u), forward it\n",
		    link_up_msg->group_id, link_up_msg->tal_id);
		
		if(this->state == SatelliteLinkState::UP)
		{
			LOG(this->log_receive, LEVEL_NOTICE,
			    "duplicate link up msg\n");
			return false;
		}

		// save group id and TAL id sent by MAC layer
		this->group_id = link_up_msg->group_id;
		this->tal_id = link_up_msg->tal_id;
		this->state = SatelliteLinkState::UP;

		// transmit link u to opposite channel
		Ptr<T_LINK_UP> shared_link_up_msg = make_ptr<T_LINK_UP>(nullptr);
		try
		{
			shared_link_up_msg = make_ptr<T_LINK_UP>();
		}
		catch (const std::bad_alloc&)
		{
		     LOG(this->log_receive, LEVEL_ERROR,
		         "failed to allocate a new 'link up' message "
		         "to transmit to opposite channel\n");
		     return false;
		}
		shared_link_up_msg->group_id = link_up_msg->group_id;
		shared_link_up_msg->tal_id = link_up_msg->tal_id;
		if(!this->shareMessage(std::move(shared_link_up_msg),
		                       to_underlying(InternalMessageType::link_up)))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to transmit 'link up' message to "
			    "opposite channel\n");
			return false;
		}

		// send the message to the upper layer
		if(!this->enqueueMessage(std::move(link_up_msg),
		                         to_underlying(InternalMessageType::link_up)))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "cannot forward 'link up' message\n");
			return false;
		}

		LOG(this->log_receive, LEVEL_INFO,
		    "'link up' message sent to the upper layer\n");

		// Set tal_id 'filter' for reception context
		tal_id_t filter_tal_id = this->filter_packets ? this->tal_id : BROADCAST_TAL_ID;

		for(auto &&encap_ctx : this->ctx)
		{
			encap_ctx->setFilterTalId(filter_tal_id);
		}

		for(auto &&encap_ctx : this->ctx_scpc)
		{
			encap_ctx->setFilterTalId(filter_tal_id);
		}

		return true;
	}

	if(msg_type == InternalMessageType::sig) {
		return this->enqueueMessage(event.getMessage<void>(), event.getMessageType());
	}

	// data received
	return this->onRcvBurst(event.getMessage<NetBurst>());
}


bool BlockEncap::onInit()
{
	std::vector <EncapPlugin::EncapContext *> up_return_ctx;
	std::vector <EncapPlugin::EncapContext *> up_return_ctx_scpc;
	std::vector <EncapPlugin::EncapContext *> down_forward_ctx;

	this->upward.setMacId(this->mac_id);
	
	LanAdaptationPlugin *lan_plugin = Ethernet::constructPlugin();
	LOG(this->log_init, LEVEL_NOTICE,
	    "lan adaptation upper layer is %s\n", lan_plugin->getName().c_str());

	if (entity_type == Component::terminal)
	{		
		LOG(this->log_init, LEVEL_INFO,
			"SCPC mode %savailable for ST%d - BlockEncap \n", 
			scpc_enabled ? "" : "not ",
			this->mac_id);

		if (scpc_enabled)
		{
			if (!this->getSCPCEncapContext(lan_plugin, up_return_ctx,
			                               "return/up"))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Cannot get Return Encapsulation context");
				return false;
			}
		}
		else
		{
			if (!this->getEncapContext(EncapSchemeList::RETURN_UP,
			                           lan_plugin, up_return_ctx,
			                           "return/up"))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Cannot get Up/Return Encapsulation context");
				return false;
			}
		}
	}
	else if (entity_type == Component::gateway)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SCPC mode available - BlockEncap");
		if(!this->getSCPCEncapContext(lan_plugin, up_return_ctx_scpc,
		                              "return/up")) 
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Cannot get SCPC Up/Return Encapsulation context");
			return false;
		}

		if(!this->getEncapContext(EncapSchemeList::RETURN_UP, 
		                          lan_plugin, up_return_ctx,
		                          "return/up")) 
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Cannot get Up/Return Encapsulation context");
			return false;
		}
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Unexpected entity type %s (should be terminal or gateway)", getComponentName(entity_type).c_str());
		return false;
	}

	if(!this->getEncapContext(EncapSchemeList::FORWARD_DOWN,
	                          lan_plugin, down_forward_ctx,
	                          "forward/down")) 
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot get Down/Forward Encapsulation context");
		return false;
	}

	LOG(this->log_init, LEVEL_NOTICE, "host type = %s\n",
	    getComponentName(entity_type).c_str());

	if (entity_type == Component::terminal)
	{
		// reorder reception context to get the deencapsulation contexts in the
		// right order
		reverse(down_forward_ctx.begin(), down_forward_ctx.end());
		
		this->downward.setContext(up_return_ctx);
		this->upward.setContext(down_forward_ctx);
	}
	else // gateway (already checked)
	{
		// reorder reception context to get the deencapsulation contexts in the
		// right order
		reverse(up_return_ctx.begin(), up_return_ctx.end());
		reverse(up_return_ctx_scpc.begin(), up_return_ctx_scpc.end());
		
		this->downward.setContext(down_forward_ctx);
		this->upward.setContext(up_return_ctx);
		this->upward.setSCPCContext(up_return_ctx_scpc);
	}

	return true;
}


bool Rt::DownwardChannel<BlockEncap>::onTimer(event_id_t timer_id)
{
	LOG(this->log_receive, LEVEL_INFO,
	    "emission timer received, flush corresponding emission "
	    "context\n");

	// find encapsulation context to flush
	auto it = this->timers.find(timer_id);
	if(it == this->timers.end())
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "timer not found\n");
		return false;
	}

	// context found
	int id = it->second;
	LOG(this->log_receive, LEVEL_INFO,
	    "corresponding emission context found (ID = %d)\n",
	    id);

	// remove emission timer from the list
	this->removeEvent(it->first);
	this->timers.erase(it);

	// flush the last encapsulation contexts
	Ptr<NetBurst> burst = (this->ctx.back())->flush(id);
	if(!burst)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "flushing context %d failed\n", id);
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "%zu encapsulation packets flushed\n",
	    burst->size());

	if(burst->size() <= 0)
	{
		return true;
	}

	// send the message to the lower layer
	if (!this->enqueueMessage(std::move(burst), to_underlying(InternalMessageType::decap_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot send burst to lower layer failed\n");
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	return true;
}


bool Rt::DownwardChannel<BlockEncap>::onRcvBurst(Ptr<NetBurst> burst)
{

	// check packet validity
	if(!burst)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		return false;
	}

	std::map<long, int> time_contexts;
	std::string name = burst->name();
	std::size_t size = burst->size();
	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulate %zu %s packet(s)\n",
	    size, name.c_str());

	// encapsulate packet
	for(auto&& context : this->ctx)
	{
		burst = context->encapsulate(std::move(burst), time_contexts);
		if(!burst)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "encapsulation failed in %s context\n",
			    context->getName().c_str());
			return false;
		}
	}

	// set encapsulate timers if needed
	for(auto&& time_iter : time_contexts)
	{
		// check if there is already a timer armed for the context
		bool found = false;
		for(auto&& it : this->timers)
		{
			if (it.second == time_iter.second)
			{
				found = true;
				break;
			}
		}

		// set a new timer if no timer was found and timer is not null
		if(!found && time_iter.first != 0)
		{
			event_id_t timer;
			std::ostringstream name;

			name << "context_" << time_iter.second;
			timer = this->addTimerEvent(name.str(),
			                            time_iter.first,
			                            false);

			this->timers.emplace(timer, time_iter.second);
			LOG(this->log_receive, LEVEL_INFO,
			    "timer for context ID %d armed with %ld ms\n",
			    time_iter.second, time_iter.first);
		}
		else
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "timer already set for context ID %d\n",
			    time_iter.second);
		}
	}

	// check burst validity
	if(!burst)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "encapsulation failed\n");
		return false;
	}

	if(burst->size() > 0)
	{
		LOG(this->log_receive, LEVEL_INFO,
		    "encapsulation packet of type %s (QoS = %d)\n",
		    burst->front()->getName().c_str(),
		    burst->front()->getQos());
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "%zu %s packet => %zu encapsulation packet(s)\n",
	    size, name.c_str(), burst->size());

	// if no encapsulation packet was created, avoid sending a message
	if(burst->size() <= 0)
	{
		return true;
	}

	// send the message to the lower layer
	if (!this->enqueueMessage(std::move(burst), to_underlying(InternalMessageType::decap_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	// everything is fine
	return true;
}


void Rt::UpwardChannel<BlockEncap>::setContext(const std::vector<EncapPlugin::EncapContext *> &encap_ctx)
{
	this->ctx = encap_ctx;
}


void Rt::UpwardChannel<BlockEncap>::setSCPCContext(const std::vector<EncapPlugin::EncapContext *> &encap_ctx_scpc)
{
	this->ctx_scpc = encap_ctx_scpc;
	if (0 < this->ctx_scpc.size())
	{
		this->scpc_encap = this->ctx_scpc[0]->getName();
	}
	else
	{
		this->scpc_encap = "";
	}
	LOG(this->log_init, LEVEL_DEBUG,
		"SCPC encapsulation lower item: \"%s\"\n",
		this->scpc_encap.c_str());
}


void Rt::UpwardChannel<BlockEncap>::setMacId(tal_id_t id)
{
	this->mac_id = id;
}


bool Rt::UpwardChannel<BlockEncap>::onRcvBurst(Ptr<NetBurst> burst)
{
	// check burst validity
	if(!burst)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		return false;
	}

	auto nb_bursts = burst->size();
	LOG(this->log_receive, LEVEL_INFO,
	    "message contains a burst of %d %s packet(s)\n",
	    nb_bursts, burst->name().c_str());

	bool is_scpc = burst->name() == this->scpc_encap && entity_type == Component::gateway;
	auto &contexts = is_scpc ? this->ctx_scpc : this->ctx;

	// iterate on all the deencapsulation contexts to get the ip packets
	for(auto&& context : contexts)
	{
		burst = context->deencapsulate(std::move(burst));
		if(!burst)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "deencapsulation failed in %s context\n",
			    context->getName().c_str());
			return false;
		}
	}
	LOG(this->log_receive, LEVEL_INFO,
	    "%d %s packet => %zu %s packet(s)\n",
	    nb_bursts, contexts[0]->getName().c_str(),
	    burst->size(), burst->name().c_str());

	if(burst->size() == 0)
	{
		return true;
	}

	// send the burst to the upper layer
	if (!this->enqueueMessage(std::move(burst), to_underlying(InternalMessageType::decap_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to upper layer\n");
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "burst of deencapsulated packets sent to the upper layer\n");

	// everthing is fine
	return true;
}


bool BlockEncap::getEncapContext(EncapSchemeList scheme_list,
                                 LanAdaptationPlugin *l_plugin,
                                 std::vector <EncapPlugin::EncapContext *> &ctx,
                                 const char *link_type)
{
	EncapPlugin *plugin;
	std::vector<std::string> encapsulations;
	switch(scheme_list)
	{
		case EncapSchemeList::RETURN_UP:
			encapsulations.push_back("RLE");
			break;

		case EncapSchemeList::FORWARD_DOWN:
			encapsulations.push_back("GSE");
			break;

		default:
			LOG(this->log_init, LEVEL_ERROR,
			    "Unknown encap schemes link: '%s'\n",
			    scheme_list);
			return false;
	}
	
	StackPlugin *upper_encap = l_plugin;

	// get all the encapsulation to use upper to lower
	for(auto& encap_name : encapsulations)
	{
		EncapPlugin::EncapContext *context;
		
		if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get plugin for %s encapsulation\n",
			    encap_name.c_str());
			return false;
		}

		context = plugin->getContext();
		ctx.push_back(context);
		if(!context->setUpperPacketHandler(
					upper_encap->getPacketHandler()))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "upper encapsulation type %s is not supported "
			    "for %s encapsulation",
			    upper_encap->getName().c_str(),
			    context->getName().c_str());
			return false;
		}
		upper_encap = plugin;
		
		LOG(this->log_init, LEVEL_INFO,
		    "add %s encapsulation layer: %s\n",
		    upper_encap->getName().c_str(), link_type);
	}
	return true;
}


bool BlockEncap::getSCPCEncapContext(LanAdaptationPlugin *l_plugin,
                                     std::vector <EncapPlugin::EncapContext *> &ctx,
                                     const char *link_type)
{

	// Get SCPC encapsulation context
	std::vector<std::string> scpc_encap;
	if (!OpenSandModelConf::Get()->getScpcEncapStack(scpc_encap) ||
		scpc_encap.size() <= 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
			"cannot get SCPC encapsulation names\n");
		return false;
	}

	StackPlugin *upper_encap = l_plugin;

	// get all the encapsulation to use upper to lower
	for(auto &&encap_name : scpc_encap)
	{

		EncapPlugin *plugin;
		if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get plugin for %s encapsulation\n",
			    encap_name.c_str());
			return false;
		}

		EncapPlugin::EncapContext *context = plugin->getContext();
		ctx.push_back(context);
		if(!context->setUpperPacketHandler(
					upper_encap->getPacketHandler()))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "upper encapsulation type %s is not supported "
			    "for %s encapsulation",
			    upper_encap->getName().c_str(),
			    context->getName().c_str());
			return false;
		}
		upper_encap = plugin;
		
		LOG(this->log_init, LEVEL_INFO,
		    "add %s encapsulation layer: %s\n",
		    upper_encap->getName().c_str(), link_type);
	}

	return true;
}

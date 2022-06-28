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
#include "Ethernet.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>
#include <opensand_rt/MessageEvent.h>

#include <algorithm>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>


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


BlockEncap::BlockEncap(const std::string &name, tal_id_t mac_id):
	Block(name),
	mac_id(mac_id)
{
	// register static log (done in Entity.cpp for now)
	// NetBurst::log_net_burst = Output::Get()->registerLog(LEVEL_WARNING, "NetBurst");
}

BlockEncap::~BlockEncap()
{
}

void BlockEncap::generateConfiguration()
{
	Plugin::generatePluginsConfiguration(nullptr,
	                                     PluginType::Encapsulation,
	                                     "encapsulation_scheme",
	                                     "Encapsulation Scheme");
}

bool BlockEncap::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
    case EventType::Timer:
		{
			// timer event, flush corresponding encapsulation context
			LOG(this->log_receive, LEVEL_INFO,
			    "Timer received %s\n", event->getName().c_str());
			return this->onTimer(event->getFd());
		}
		break;

    case EventType::Message:
		{
			// message received from another bloc
			LOG(this->log_receive, LEVEL_INFO,
			    "message received from the upper-layer bloc\n");
			
			auto msg_event = static_cast<const MessageEvent *>(event);
			if(to_enum<InternalMessageType>(msg_event->getMessageType()) == InternalMessageType::msg_link_up)
			{
				// 'link up' message received 
				T_LINK_UP *link_up_msg = static_cast<T_LINK_UP *>(msg_event->getData());

				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = SatelliteLinkState::UP;
				delete link_up_msg;
				break;
			}

			NetBurst *burst = static_cast<NetBurst *>(msg_event->getData());
			return this->onRcvBurst(burst);
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s\n",
			    event->getName().c_str());
			return false;
	}

	return true;
}

void BlockEncap::Downward::setContext(const std::vector<EncapPlugin::EncapContext *> &encap_ctx)
{
	this->ctx = encap_ctx;
}

bool BlockEncap::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
    case EventType::Message:
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "message received from the lower layer\n");

			auto msg_event = static_cast<const MessageEvent *>(event);
			if(to_enum<InternalMessageType>(msg_event->getMessageType()) == InternalMessageType::msg_link_up)
			{
				std::vector<EncapPlugin::EncapContext*>::iterator encap_it;

				// 'link up' message received => forward it to upper layer
				T_LINK_UP *link_up_msg = static_cast<T_LINK_UP *>(msg_event->getData());
				LOG(this->log_receive, LEVEL_INFO,
				    "'link up' message received (group = %u, "
				    "tal = %u), forward it\n", link_up_msg->group_id,
				    link_up_msg->tal_id);
				
				if(this->state == SatelliteLinkState::UP)
				{
					LOG(this->log_receive, LEVEL_NOTICE,
					    "duplicate link up msg\n");
					delete link_up_msg;
					return false;
				}

				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = SatelliteLinkState::UP;

				// transmit link u to opposite channel
				T_LINK_UP *shared_link_up_msg = new T_LINK_UP;
				if(shared_link_up_msg == nullptr)
				{
				     LOG(this->log_receive, LEVEL_ERROR,
				         "failed to allocate a new 'link up' message "
				         "to transmit to opposite channel\n");
				     delete link_up_msg;
				     return false;
				}
				shared_link_up_msg->group_id = link_up_msg->group_id;
				shared_link_up_msg->tal_id = link_up_msg->tal_id;
				if(!this->shareMessage((void **)&shared_link_up_msg,
				                       sizeof(T_LINK_UP),
				                       to_underlying(InternalMessageType::msg_link_up)))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to transmit 'link up' message to "
					    "opposite channel\n");
					delete shared_link_up_msg;
					delete link_up_msg;
					return false;
				}

				// send the message to the upper layer
				if(!this->enqueueMessage((void **)&link_up_msg,
				                         sizeof(T_LINK_UP),
				                         to_underlying(InternalMessageType::msg_link_up)))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot forward 'link up' message\n");
					delete link_up_msg;
					return false;
				}

				LOG(this->log_receive, LEVEL_INFO,
				    "'link up' message sent to the upper layer\n");

				// Set tal_id 'filter' for reception context

				for(encap_it = this->ctx.begin();
				    encap_it != this->ctx.end();
				    ++encap_it)
				{
					(*encap_it)->setFilterTalId(this->tal_id);
				}

				for(encap_it = this->ctx_scpc.begin();
				    encap_it != this->ctx_scpc.end();
				    ++encap_it)
				{
					(*encap_it)->setFilterTalId(this->tal_id);
				}

				break;
			}

			// data received
			NetBurst *burst = static_cast<NetBurst *>(msg_event->getData());
			return this->onRcvBurst(burst);
		}

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s\n",
			    event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockEncap::onInit()
{
	std::vector <EncapPlugin::EncapContext *> up_return_ctx;
	std::vector <EncapPlugin::EncapContext *> up_return_ctx_scpc;
	std::vector <EncapPlugin::EncapContext *> down_forward_ctx;

	static_cast<Upward *>(this->upward)->setMacId(this->mac_id);
	
	LanAdaptationPlugin *lan_plugin = Ethernet::constructPlugin();
	LOG(this->log_init, LEVEL_NOTICE,
	    "lan adaptation upper layer is %s\n", lan_plugin->getName().c_str());

	auto Conf = OpenSandModelConf::Get();
	if (!Conf->isGw(this->mac_id))
	{
		LOG(this->log_init, LEVEL_DEBUG,
		    "Going to check if Tal with id:  %d is in Scpc mode\n",
		    this->mac_id);
		
		auto access = Conf->getProfileData()->getComponent("access");
		auto scpc_enabled = access->getComponent("settings")->getParameter("scpc_enabled");
		bool is_scpc = false;
		OpenSandModelConf::extractParameterData(scpc_enabled, is_scpc);

		LOG(this->log_init, LEVEL_INFO,
			"SCPC mode %savailable for ST%d - BlockEncap \n", 
			is_scpc ? "" : "not ",
			this->mac_id);

		if (!is_scpc)
		{
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
			if(!this->getSCPCEncapContext(lan_plugin, up_return_ctx,
			                              "return/up")) 
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Cannot get Return Encapsulation context");
				return false;
			}
		}
	}
	else
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

	if(!this->getEncapContext(EncapSchemeList::FORWARD_DOWN,
	                          lan_plugin, down_forward_ctx,
	                          "forward/down")) 
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot get Down/Forward Encapsulation context");
		return false;
	}

	// get host type
	auto host = Conf->getComponentType();
	LOG(this->log_init, LEVEL_NOTICE,
	    "host type = %s\n",
	    getComponentName(host).c_str());

	if(host == Component::terminal)
	{
		// reorder reception context to get the deencapsulation contexts in the
		// right order
		reverse(down_forward_ctx.begin(), down_forward_ctx.end());
		
		static_cast<Downward *>(this->downward)->setContext(up_return_ctx);
		static_cast<Upward *>(this->upward)->setContext(down_forward_ctx);
	}
	else
	{
		// reorder reception context to get the deencapsulation contexts in the
		// right order
		reverse(up_return_ctx.begin(), up_return_ctx.end());
		reverse(up_return_ctx_scpc.begin(), up_return_ctx_scpc.end());
		
		static_cast<Downward *>(this->downward)->setContext(down_forward_ctx);
		static_cast<Upward *>(this->upward)->setContext(up_return_ctx);
		static_cast<Upward *>(this->upward)->setSCPCContext(up_return_ctx_scpc);
	}

	return true;
}

bool BlockEncap::Downward::onTimer(event_id_t timer_id)
{
	std::map<event_id_t, int>::iterator it;
	int id;
	NetBurst *burst;
	bool status = false;

	LOG(this->log_receive, LEVEL_INFO,
	    "emission timer received, flush corresponding emission "
	    "context\n");

	// find encapsulation context to flush
	it = this->timers.find(timer_id);
	if(it == this->timers.end())
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "timer not found\n");
		goto error;
	}

	// context found
	id = (*it).second;
	LOG(this->log_receive, LEVEL_INFO,
	    "corresponding emission context found (ID = %d)\n",
	    id);

	// remove emission timer from the list
	this->removeEvent((*it).first);
	this->timers.erase(it);

	// flush the last encapsulation contexts
	burst = (this->ctx.back())->flush(id);
	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "flushing context %d failed\n", id);
		goto error;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "%zu encapsulation packets flushed\n",
	    burst->size());

	if(burst->size() <= 0)
	{
		status = true;
		goto clean;
	}

	// send the message to the lower layer
	if (!this->enqueueMessage((void **)&burst, 0, to_underlying(InternalMessageType::msg_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot send burst to lower layer failed\n");
		goto clean;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	return true;

clean:
	delete burst;
error:
	return status;
}

bool BlockEncap::Downward::onRcvBurst(NetBurst *burst)
{
	std::map<long, int> time_contexts;
	std::vector<EncapPlugin::EncapContext *>::iterator iter;
	std::string name;
	size_t size;
	bool status = false;

	// check packet validity
	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	name = burst->name();
	size = burst->size();
	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulate %zu %s packet(s)\n",
	    size, name.c_str());

	// encapsulate packet
	for(iter = this->ctx.begin(); iter != this->ctx.end();
	    iter++)
	{
		burst = (*iter)->encapsulate(burst, time_contexts);
		if(burst == NULL)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "encapsulation failed in %s context\n",
			    (*iter)->getName().c_str());
			goto error;
		}
	}

	// set encapsulate timers if needed
	for(std::map<long, int>::iterator time_iter = time_contexts.begin();
	    time_iter != time_contexts.end(); time_iter++)
	{
		std::map<event_id_t, int>::iterator it;
		bool found = false;

		// check if there is already a timer armed for the context
		for(it = this->timers.begin(); !found && it != this->timers.end(); it++)
		{
		    found = ((*it).second == (*time_iter).second);
		}

		// set a new timer if no timer was found and timer is not null
		if(!found && (*time_iter).first != 0)
		{
			event_id_t timer;
			std::ostringstream name;

			name << "context_" << (*time_iter).second;
			timer = this->addTimerEvent(name.str(),
			                            (*time_iter).first,
			                            false);

			this->timers.insert(std::make_pair(timer, (*time_iter).second));
			LOG(this->log_receive, LEVEL_INFO,
			    "timer for context ID %d armed with %ld ms\n",
			    (*time_iter).second, (*time_iter).first);
		}
		else
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "timer already set for context ID %d\n",
			    (*time_iter).second);
		}
	}

	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "encapsulation failed\n");
		goto error;
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
		status = true;
		goto clean;
	}


	// send the message to the lower layer
	if (!this->enqueueMessage((void **)&burst, 0, to_underlying(InternalMessageType::msg_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		goto clean;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	// everything is fine
	return true;

clean:
	delete burst;
error:
	return status;
}

void BlockEncap::Upward::setContext(const std::vector<EncapPlugin::EncapContext *> &encap_ctx)
{
	this->ctx = encap_ctx;
}

void BlockEncap::Upward::setSCPCContext(const std::vector<EncapPlugin::EncapContext *> &encap_ctx_scpc)
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

void BlockEncap::Upward::setMacId(tal_id_t id)
{
	this->mac_id = id;
}

bool BlockEncap::Upward::onRcvBurst(NetBurst *burst)
{
	std::vector <EncapPlugin::EncapContext *>::iterator iter;
	unsigned int nb_bursts;


	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	nb_bursts = burst->size();
	LOG(this->log_receive, LEVEL_INFO,
	    "message contains a burst of %d %s packet(s)\n",
	    nb_bursts, burst->name().c_str());

	if(burst->name() == this->scpc_encap &&
	   OpenSandModelConf::Get()->isGw(this->mac_id))
	{
		// SCPC case

		// iterate on all the deencapsulation contexts to get the ip packets
		for(iter = this->ctx_scpc.begin();
		    iter != this->ctx_scpc.end();
		    ++iter)
		{
			burst = (*iter)->deencapsulate(burst);
			if(burst == NULL)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "deencapsulation failed in %s context\n",
				    (*iter)->getName().c_str());
				goto error;
			}
		}
		LOG(this->log_receive, LEVEL_INFO,
		    "%d %s packet => %zu %s packet(s)\n",
		    nb_bursts, this->ctx_scpc[0]->getName().c_str(),
		    burst->size(), burst->name().c_str());
	}
	else
	{
		// iterate on all the deencapsulation contexts to get the ip packets
		for(iter = this->ctx.begin(); 
		    iter != this->ctx.end();
		    ++iter)
		{
			burst = (*iter)->deencapsulate(burst);
			if(burst == NULL)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "deencapsulation failed in %s context\n",
				    (*iter)->getName().c_str());
				goto error;
			}
		}
		LOG(this->log_receive, LEVEL_INFO,
		    "%d %s packet => %zu %s packet(s)\n",
		    nb_bursts, this->ctx[0]->getName().c_str(),
		    burst->size(), burst->name().c_str());
	}

	if(burst->size() == 0)
	{
		delete burst;
		return true;
	}

	// send the burst to the upper layer
	if (!this->enqueueMessage((void **)&burst, 0, to_underlying(InternalMessageType::msg_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to upper layer\n");
		delete burst;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "burst of deencapsulated packets sent to the upper "
	    "layer\n");

	// everthing is fine
	return true;

error:
	return false;
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
	std::vector<std::string> scpc_encap;
	std::vector<std::string>::iterator ite;
	StackPlugin *upper_encap = NULL;
	EncapPlugin *plugin;
	std::string encap_name;

	// Get SCPC encapsulation context
	if (!OpenSandModelConf::Get()->getScpcEncapStack(scpc_encap) ||
		scpc_encap.size() <= 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
			"cannot get SCPC encapsulation names\n");
		goto error;
	}

	upper_encap = l_plugin;

	// get all the encapsulation to use upper to lower
	for(ite = scpc_encap.begin(); ite != scpc_encap.end(); ++ite)
	{
		EncapPlugin::EncapContext *context;
		
		encap_name = *ite;

		if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get plugin for %s encapsulation\n",
			    encap_name.c_str());
			goto error;
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
			goto error;
		}
		upper_encap = plugin;
		
		LOG(this->log_init, LEVEL_INFO,
		    "add %s encapsulation layer: %s\n",
		    upper_encap->getName().c_str(), link_type);
	}

	return true;
	
	error:
		return false;
}


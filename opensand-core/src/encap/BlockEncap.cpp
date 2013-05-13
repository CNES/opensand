/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file BlockEncap.cpp
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "BlockEncap.h"
#include "Plugin.h"

#include <algorithm>
#include <stdint.h>

// debug
#define DBG_PREFIX
#define DBG_PACKAGE PKG_ENCAP
#include <opensand_conf/uti_debug.h>

Event* BlockEncap::error_init = NULL;

/**
 * @brief get the satellite type according to its name
 * 
 * @param type the satellite type name
 * 
 * @return the satellite type enum
 */
sat_type_t strToSatType(std::string sat_type)
{
	if(sat_type == "regenerative")
		return REGENERATIVE;
	else
		return TRANSPARENT;
}

BlockEncap::BlockEncap(const string &name, component_t host):
	Block(name),
	host(host),
	group_id(-1),
	tal_id(-1),
	state(link_down)
{
	this->ip_handler = new IpPacketHandler(*((EncapPlugin *)NULL));
	
	if(error_init == NULL)
	{
		error_init = Output::registerEvent("BlockEncap:init", LEVEL_ERROR);
	}
}

BlockEncap::~BlockEncap()
{
	delete this->ip_handler;
}



bool BlockEncap::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_timer:
		{
			// timer event, flush corresponding encapsulation context
			UTI_DEBUG("Timer received %s\n", event->getName().c_str());
			return this->onTimer(event->getFd());
		}
		break;

		case evt_message:
		{
			// message received from another bloc
			UTI_DEBUG("message received from the upper-layer bloc\n");
			NetPacket *packet;
			packet = (NetPacket *)((MessageEvent *)event)->getData();
			return this->onRcvIpFromUp(packet);
		}
		break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockEncap::onUpwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			UTI_DEBUG("message received from the lower layer\n");

			if(((MessageEvent *)event)->getMessageType() == msg_link_up)
			{
				T_LINK_UP *link_up_msg;
				vector<EncapPlugin::EncapContext*>::iterator encap_it;

				// 'link up' message received => forward it to upper layer
				UTI_DEBUG("'link up' message received, forward it\n");

				link_up_msg = (T_LINK_UP *)((MessageEvent *)event)->getData();
				if(this->state == link_up)
				{
					UTI_INFO("duplicate link up msg\n");
					delete link_up_msg;
					return false;;
				}

				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = link_up;

				// send the message to the upper layer
				if(!this->sendUp((void **)&link_up_msg,
					             sizeof(T_LINK_UP), msg_link_up))
				{
					UTI_ERROR("cannot forward 'link up' message\n");
					delete link_up_msg;
					return false;
				}

				UTI_DEBUG("'link up' message sent to the upper layer\n");

				// Set tal_id 'filter' for reception context
				for(encap_it = this->reception_ctx.begin();
				    encap_it != this->reception_ctx.end();
				    ++encap_it)
				{
					(*encap_it)->setFilterTalId(this->tal_id);
				}
				break;
			}

			// data received
			NetBurst *burst;
			burst = (NetBurst *)((MessageEvent *)event)->getData();
			return this->onRcvBurstFromDown(burst);
		}

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockEncap::onInit()
{
	string up_return_encap_proto;
	string downlink_encap_proto;
	string satellite_type;
	ConfigurationList option_list;
	vector <EncapPlugin::EncapContext *> up_return_ctx;
	vector <EncapPlugin::EncapContext *> down_forward_ctx;
	int i;
	int encap_nbr;
	EncapPlugin *plugin;
	EncapPlugin *upper_option = NULL;
	EncapPlugin *upper_encap = NULL;

	// satellite type: regenerative or transparent ?
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          satellite_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_DEBUG("satellite type = %s\n", satellite_type.c_str());

	// Retrive ip_options
	if(!globalConfig.getListItems(GLOBAL_SECTION, IP_OPTION_LIST, option_list))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n", GLOBAL_SECTION,
		          IP_OPTION_LIST);
		goto error;
	}

	upper_option = NULL;
	// get all the IP options
	for(ConfigurationList::iterator iter = option_list.begin();
	    iter != option_list.end(); ++iter)
	{
		string option_name;
		EncapPlugin::EncapContext *context;

		if(!globalConfig.getAttributeValue(iter, OPTION_NAME, option_name))
		{
			UTI_ERROR("Section %s, invalid value for parameter '%s'\n",
			          GLOBAL_SECTION, OPTION_NAME);
			goto error;
		}
		if(option_name == "NONE")
		{
			continue;
		}

		if(!Plugin::getEncapsulationPlugins(option_name, &plugin))
		{
			UTI_ERROR("missing plugin for %s encapsulation",
			          option_name.c_str());
			goto error;
		}

		context = plugin->getContext();
		up_return_ctx.push_back(context);
		down_forward_ctx.push_back(context);
		if(upper_option == NULL)
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   strToSatType(satellite_type)))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					upper_option->getPacketHandler(),
					strToSatType(satellite_type)))
		{
			UTI_ERROR("%s is not supported for %s IP option",
			          upper_option->getName().c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_option = plugin;
		UTI_INFO("add IP option: %s\n",
		         upper_option->getName().c_str());
	}

	upper_encap = upper_option;
	// get the number of encapsulation context to use for up/return link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("Section %s, %s missing\n", GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	for(i = 0; i < encap_nbr; i++)
	{
		string encap_name;
		EncapPlugin::EncapContext *context;

		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("Section %s, invalid value %d for parameter '%s'\n",
			          GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		if(!Plugin::getEncapsulationPlugins(encap_name, &plugin))
		{
			UTI_ERROR("cannot get plugin for %s encapsulation",
			          encap_name.c_str());
			goto error;
		}

		context = plugin->getContext();
		up_return_ctx.push_back(context);
		if(upper_encap == NULL)
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   strToSatType(satellite_type)))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					upper_encap->getPacketHandler(),
					strToSatType(satellite_type)))
		{
			UTI_ERROR("upper encapsulation type %s is not supported for %s "
			          "encapsulation", upper_encap->getName().c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_encap = plugin;
		UTI_DEBUG("add up/return encapsulation layer: %s\n",
		          upper_encap->getName().c_str());
	}

	// get the number of encapsulation context to use for down/forward link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR(" Section %s, %s missing\n", GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	upper_encap = upper_option;
	for(i = 0; i < encap_nbr; i++)
	{
		string encap_name;
		EncapPlugin::EncapContext *context;

		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("Section %s, invalid value %d for parameter '%s'\n",
			          GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		if(!Plugin::getEncapsulationPlugins(encap_name, &plugin))
		{
			UTI_ERROR("cannot get plugin for %s encapsulation",
			          encap_name.c_str());
			goto error;
		}

		context = plugin->getContext();
		down_forward_ctx.push_back(context);
		if(upper_encap == NULL)
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   strToSatType(satellite_type)))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					upper_encap->getPacketHandler(),
					strToSatType(satellite_type)))
		{
			UTI_ERROR("upper encapsulation type %s is not supported for %s "
			          "encapsulation", upper_encap->getName().c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_encap = plugin;
		UTI_DEBUG("add down/forward encapsulation layer: %s\n",
		          upper_encap->getName().c_str());
	}

	if(this->host == terminal || satellite_type == "regenerative")
	{
		this->emission_ctx = up_return_ctx;
		this->reception_ctx = down_forward_ctx;
	}
	else
	{
		this->reception_ctx = up_return_ctx;
		this->emission_ctx = down_forward_ctx;
	}
	// reorder reception context to get the deencapsulation contexts in the
	// right order
	reverse(this->reception_ctx.begin(), this->reception_ctx.end());

	return true;
error:
	return false;
}

bool BlockEncap::onTimer(event_id_t timer_id)
{
	const char *FUNCNAME = "[BlockEncap::onTimer]";
	std::map<event_id_t, int>::iterator it;
	int id;
	NetBurst *burst;
	bool status = false;

	UTI_DEBUG("%s emission timer received, flush corresponding emission "
	          "context\n", FUNCNAME);

	// find encapsulation context to flush
	it = this->timers.find(timer_id);
	if(it == this->timers.end())
	{
		UTI_ERROR("%s timer not found\n", FUNCNAME);
		goto error;
	}

	// context found
	id = (*it).second;
	UTI_DEBUG("%s corresponding emission context found (ID = %d)\n",
	          FUNCNAME, id);

	// remove emission timer from the list
	this->downward->removeEvent((*it).first);
	this->timers.erase(it);

	// flush the last encapsulation contexts
	burst = (this->emission_ctx.back())->flush(id);
	if(burst == NULL)
	{
		UTI_ERROR("%s flushing context %d failed\n", FUNCNAME, id);
		goto error;
	}

	UTI_DEBUG("%s %d encapsulation packets flushed\n", FUNCNAME, burst->size());

	if(burst->size() <= 0)
	{
		status = true;
		goto clean;
	}

	// send the message to the lower layer
	if(!this->sendDown((void **)&burst, sizeof(burst)))
	{
		UTI_ERROR("%s cannot send burst to lower layer failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	return true;

clean:
	delete burst;
error:
	return status;
}

bool BlockEncap::onRcvIpFromUp(NetPacket *packet)
{
	const char *FUNCNAME = "[BlockEncap::onRcvIpFromUp]";
	NetBurst *burst;
	map<long, int> time_contexts;
	vector<EncapPlugin::EncapContext *>::iterator iter;
	string name = packet->getName();
	bool status = false;

	// check packet validity
	if(packet == NULL)
	{
		UTI_ERROR("%s packet is not valid\n", FUNCNAME);
		goto error;
	}

	// check packet type
	if(packet->getType() != NET_PROTO_IPV4 && packet->getType() != NET_PROTO_IPV6)
	{
		UTI_ERROR("%s packet (type 0x%04x) is not an IP packet\n",
		          FUNCNAME, packet->getType());
		delete packet;
		goto error;
	}

	UTI_DEBUG("%s encapsulate one %s packet (%d bytes, "
	          "Src TAL Id = %u, Dst TAL Id = %u, QoS = %u)\n",
	          FUNCNAME, packet->getName().c_str(), packet->getTotalLength(),
	          packet->getSrcTalId(), packet->getDstTalId(), packet->getQos());

	burst = new NetBurst();
	burst->add(packet);
	// encapsulate packet
	for(iter = this->emission_ctx.begin(); iter != this->emission_ctx.end();
	    iter++)
	{
		burst = (*iter)->encapsulate(burst, time_contexts);
		if(burst == NULL)
		{
			UTI_ERROR("%s encapsulation failed in %s context\n",
			          FUNCNAME, (*iter)->getName().c_str());
			goto error;
		}
	}

	// set encapsulate timers if needed
	for(map<long, int>::iterator time_iter = time_contexts.begin();
	    time_iter != time_contexts.end(); time_iter++)
	{
		std::map<event_id_t, int>::iterator it;
		bool found = false;

		// check if there is already a timer armed for the context
		for(it = this->timers.begin(); !found && it != this->timers.end(); it++)
		    found = ((*it).second == (*time_iter).second);

		// set a new timer if no timer was found and timer is not null
		if(!found && (*time_iter).first != 0)
		{
			event_id_t timer;
			ostringstream name;

			name << "context_" << (*time_iter).second;
			timer = this->downward->addTimerEvent(name.str(),
			                                      (*time_iter).first,
			                                      false);

			this->timers.insert(std::make_pair(timer, (*time_iter).second));
			UTI_DEBUG("%s timer for context ID %d armed with %ld ms\n",
			          FUNCNAME, (*time_iter).second, (*time_iter).first);
		}
		else
		{
			UTI_DEBUG("%s timer already set for context ID %d\n",
			          FUNCNAME, (*time_iter).second);
		}
	}

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s encapsulation failed\n", FUNCNAME);
		goto error;
	}

	if(burst->size() > 0)
	{
		UTI_DEBUG("encapsulation packet of type %s (QoS = %d)\n",
		          burst->front()->getName().c_str(), burst->front()->getQos());
	}

	UTI_DEBUG("1 %s packet => %d encapsulation packet(s)\n",
	          name.c_str(), burst->size());

	// if no encapsulation packet was created, avoid sending a message
	if(burst->size() <= 0)
	{
		status = true;
		goto clean;
	}


	// send the message to the lower layer
	if(!this->sendDown((void **)&burst, sizeof(burst)))
	{
		UTI_ERROR("failed to send burst to lower layer\n");
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	// everything is fine
	return true;

clean:
	delete burst;
error:
	return status;
}

bool BlockEncap::onRcvBurstFromDown(NetBurst *burst)
{
	const char *FUNCNAME = "[BlockEncap::onRcvBurstFromDown]";
	NetBurst *ip_packets;
	NetBurst::iterator ip_pkt_it;
	vector <EncapPlugin::EncapContext *>::iterator iter;
	unsigned int nb_bursts;


	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	nb_bursts = burst->size();
	UTI_DEBUG("%s message contains a burst of %d %s packet(s)\n",
	          FUNCNAME, nb_bursts, burst->name().c_str());

	// iterate on all the deencapsulation contexts to get the ip packets
	ip_packets = burst;
	for(iter = this->reception_ctx.begin(); iter != this->reception_ctx.end();
	    ++iter)
	{
		ip_packets = (*iter)->deencapsulate(ip_packets);
		if(ip_packets == NULL)
		{
			UTI_ERROR("%s deencapsulation failed in %s context\n",
			          FUNCNAME, (*iter)->getName().c_str());
			goto error;
		}
	}

	UTI_DEBUG("%s %d %s packet => %d IP packet(s)\n", FUNCNAME,
	          nb_bursts, this->reception_ctx[0]->getName().c_str(),
	          ip_packets->size());

	// for every desencapsulated IP packet...
	for(ip_pkt_it = ip_packets->begin();
		ip_pkt_it != ip_packets->end(); ip_pkt_it++)
	{
		if((*ip_pkt_it)->getType() != NET_PROTO_IPV4 &&
		   (*ip_pkt_it)->getType() != NET_PROTO_IPV6)
		{
			UTI_ERROR("%s cannot send non-IP packet (0x%04x) to the "
			          "upper-layer block\n", FUNCNAME, (*ip_pkt_it)->getType());
			delete *ip_pkt_it;
			continue;
		}

		// send the message to the upper layer
		if(!this->sendUp((void **)&(*ip_pkt_it)))
		{
			UTI_ERROR("failed to send message to upper layer\n");
			delete *ip_pkt_it;
			continue;
		}

		UTI_DEBUG("%s IP packet sent to the upper layer\n", FUNCNAME);
	}

	// clear the burst of IP packets without deleting the IpPacket
	// objects it contains then delete the burst
	ip_packets->clear();
	delete ip_packets;


	// everthing is fine
	return true;

error:
	return false;
}

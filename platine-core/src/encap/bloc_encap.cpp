/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file bloc_encap.cpp
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "bloc_encap.h"

#include <algorithm>
#include <stdint.h>

// debug
#define DBG_PREFIX
#define DBG_PACKAGE PKG_ENCAP
#include <platine_conf/uti_debug.h>


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

BlocEncap::BlocEncap(mgl_blocmgr * blocmgr, mgl_id fatherid, const char *name,
                     t_component host,
                     std::map<std::string, EncapPlugin *> &encap_plug):
	mgl_bloc(blocmgr, fatherid, name),
	encap_plug(encap_plug)
{
	this->initOk = false;

	// group & TAL id
	this->group_id = -1;
	this->tal_id = -1;
	this->host = host;

	// link state
	this->state = link_down;
	this->ip_handler = new IpPacketHandler(*((EncapPlugin *)NULL));
}

BlocEncap::~BlocEncap()
{
	delete this->ip_handler;
}

mgl_status BlocEncap::onEvent(mgl_event *event)
{
	const char *FUNCNAME = "[BlocEncap::onEvent]";
	mgl_status status = mgl_ko;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->initOk)
		{
			UTI_ERROR("%s bloc has already been initialized, ignore init event\n",
			          FUNCNAME);
		}
		else if(this->onInit() == mgl_ok)
		{
			this->initOk = true;
			status = mgl_ok;
		}
	}
	else if(!this->initOk)
	{
		UTI_ERROR("%s encapsulation bloc not initialized, ignore "
		          "non-init event\n", FUNCNAME);
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		// timer event, flush corresponding encapsulation context
		status = this->onTimer((mgl_timer) event->event.timer.id);
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		// message received from another bloc

		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getUpperLayer())
		{
			UTI_DEBUG("%s message received from the upper-layer bloc\n", FUNCNAME);

			if(MGL_EVENT_MSG_IS_TYPE(event, msg_ip))
			{
				NetPacket *packet;
				packet = (NetPacket *) MGL_EVENT_MSG_GET_BODY(event);
				status = this->onRcvIpFromUp(packet);
			}
			else
			{
				UTI_ERROR("%s message type is unknown\n", FUNCNAME);
			}
		}
		else if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer())
		{
			UTI_DEBUG("%s message received from the lower layer\n", FUNCNAME);

			if(MGL_EVENT_MSG_IS_TYPE(event, msg_link_up))
			{
				mgl_msg *msg; // margouilla message
				T_LINK_UP *link_up_msg;
				vector<EncapPlugin::EncapContext*>::iterator encap_it;

				// 'link up' message received => forward it to upper layer
				UTI_DEBUG("%s 'link up' message received, forward it\n", FUNCNAME);

				link_up_msg = (T_LINK_UP *) MGL_EVENT_MSG_GET_BODY(event);

				if(this->state == link_up)
				{
					UTI_INFO("%s duplicate link up msg\n", FUNCNAME);
					delete link_up_msg;
					goto end_link_up;
				}

				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = link_up;

				// create the Margouilla message
				msg = this->newMsgWithBodyPtr(msg_link_up, link_up_msg,
				                              sizeof(link_up_msg));
				if(!msg)
				{
					UTI_ERROR("%s cannot create 'link up' message\n", FUNCNAME);
					delete link_up_msg;
					goto end_link_up;
				}

				// send the message to the upper layer
				if(this->sendMsgTo(this->getUpperLayer(), msg) == mgl_ko)
				{
					UTI_ERROR("%s cannot forward 'link up' message\n", FUNCNAME);
					delete link_up_msg;
					goto end_link_up;
				}

				UTI_DEBUG("%s 'link up' message sent to the upper layer\n", FUNCNAME);

				// Set tal_id 'filter' for reception context
				for(encap_it = this->reception_ctx.begin();
				    encap_it != this->reception_ctx.end();
				    ++encap_it)
				{
					(*encap_it)->setFilterTalId(this->tal_id);
				}
end_link_up:

				status = mgl_ok;
			}
			else if(MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
			{
				NetBurst *burst;
				burst = (NetBurst *) MGL_EVENT_MSG_GET_BODY(event);
				status = this->onRcvBurstFromDown(burst);
			}
			else
			{
				UTI_ERROR("%s message type is unknown\n", FUNCNAME);
			}
		}
		else
		{
			UTI_ERROR("%s message received from an unknown bloc\n", FUNCNAME);
		}
	}
	else
	{
		UTI_ERROR("%s unknown event (type %ld) received\n", FUNCNAME, event->type);
	}

	return status;
}

mgl_status BlocEncap::onInit()
{
	const char *FUNCNAME = "[BlocEncap::onInit]";
	string up_return_encap_proto;
	string downlink_encap_proto;
	string satellite_type;
	string upper_option;
	ConfigurationList option_list;
	vector <EncapPlugin::EncapContext *> up_return_ctx;
	vector <EncapPlugin::EncapContext *> down_forward_ctx;
	int i;
	int encap_nbr;
	string upper_name;

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

	// TODO factorize
	upper_option = "";
	// get all the IP options
	for(ConfigurationList::iterator iter = option_list.begin();
	    iter != option_list.end(); ++iter)
	{
		string option_name;
		EncapPlugin::EncapContext *context;

		if(!globalConfig.getAttributeValue(iter, OPTION_NAME, option_name))
		{
			UTI_ERROR("%s Section %s, invalid value for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, OPTION_NAME);
			goto error;
		}
		if(option_name == "NONE")
		{
			continue;
		}

		if(this->encap_plug[option_name] == NULL)
		{
			UTI_ERROR("%s missing plugin for %s encapsulation",
			          FUNCNAME, option_name.c_str());
			goto error;
		}

		context = this->encap_plug[option_name]->getContext();
		up_return_ctx.push_back(context);
		down_forward_ctx.push_back(context);
		if(upper_option == "")
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   strToSatType(satellite_type)))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					this->encap_plug[upper_option]->getPacketHandler(),
					strToSatType(satellite_type)))
		{
			UTI_ERROR("%s %s is not supported for %s "
			          "IP option", FUNCNAME, upper_option.c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_option = context->getName();
		UTI_INFO("%s add IP option: %s\n",
		         FUNCNAME, upper_option.c_str());
	}

	// get the number of encapsulation context to use for up/return link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	upper_name = upper_option;
	for(i = 0; i < encap_nbr; i++)
	{
		string encap_name;
		EncapPlugin::EncapContext *context;

		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		if(this->encap_plug[encap_name] == NULL)
		{
			UTI_ERROR("%s missing plugin for %s encapsulation",
			          FUNCNAME, encap_name.c_str());
			goto error;
		}

		context = this->encap_plug[encap_name]->getContext();
		up_return_ctx.push_back(context);
		if(upper_name == "")
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   strToSatType(satellite_type)))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					this->encap_plug[upper_name]->getPacketHandler(),
					strToSatType(satellite_type)))
		{
			UTI_ERROR("%s upper encapsulation type %s is not supported for %s "
			          "encapsulation", FUNCNAME, upper_name.c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_name = context->getName();
		UTI_DEBUG("%s add up/return encapsulation layer: %s\n",
		          FUNCNAME, upper_name.c_str());
	}

	// get the number of encapsulation context to use for down/forward link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	upper_name = upper_option;
	for(i = 0; i < encap_nbr; i++)
	{
		string encap_name;
		EncapPlugin::EncapContext *context;

		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		if(this->encap_plug[encap_name] == NULL)
		{
			UTI_ERROR("%s missing plugin for %s encapsulation",
			          FUNCNAME, encap_name.c_str());
			goto error;
		}

		context = this->encap_plug[encap_name]->getContext();
		down_forward_ctx.push_back(context);
		if(upper_name == "")
		{
			if(!context->setUpperPacketHandler(this->ip_handler,
			                                   strToSatType(satellite_type)))
			{
				goto error;
			}
		}
		else if(!context->setUpperPacketHandler(
					this->encap_plug[upper_name]->getPacketHandler(),
					strToSatType(satellite_type)))
		{
			UTI_ERROR("%s upper encapsulation type %s is not supported for %s "
			          "encapsulation", FUNCNAME, upper_name.c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_name = context->getName();
		UTI_DEBUG("%s add down/forward encapsulation layer: %s\n",
		          FUNCNAME, upper_name.c_str());
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

	return mgl_ok;
error:
	return mgl_ko;
}

mgl_status BlocEncap::onTimer(mgl_timer timer)
{
	const char *FUNCNAME = "[BlocEncap::onTimer]";
	std::map < mgl_timer, int >::iterator it;
	int id;
	NetBurst *burst;
	mgl_msg *msg; // margouilla message

	UTI_DEBUG("%s emission timer received, flush corresponding emission "
	          "context\n", FUNCNAME);

	// find encapsulation context to flush
	it = this->timers.find(timer);
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
		goto clean;

	// create the Margouilla message
	// with encapsulation burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, burst, sizeof(burst));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}

mgl_status BlocEncap::onRcvIpFromUp(NetPacket *packet)
{
	const char *FUNCNAME = "[BlocEncap::onRcvIpFromUp]";
	NetBurst *burst;
	map<long, int> time_contexts;
	vector<EncapPlugin::EncapContext *>::iterator iter;
	mgl_msg *msg; // margouilla message
	string name = packet->getName();

	// check packet validity
	if(packet == NULL)
	{
		UTI_ERROR("%s packet is not valid\n", FUNCNAME);
		goto error;
	}

	packet->addTrace(HERE());

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
		std::map < mgl_timer, int >::iterator it;
		bool found = false;

		// check if there is already a timer armed for the context
		for(it = this->timers.begin(); !found && it != this->timers.end(); it++)
		    found = ((*it).second == (*time_iter).second);

		// set a new timer if no timer was found and timer is not null
		if(!found && (*time_iter).first != 0)
		{
			mgl_timer timer;
			this->setTimer(timer, (*time_iter).first);
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
		goto clean;

	// create the Margouilla message
	// with encapsulation burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, burst, sizeof(burst));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	// everything is fine
	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}

mgl_status BlocEncap::onRcvBurstFromDown(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncap::onRcvBurstFromDown]";
	NetBurst *ip_packets;
	NetBurst::iterator ip_pkt_it;
	mgl_msg *msg; // margouilla message
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
		(*ip_pkt_it)->addTrace(HERE());

		// create the Margouilla message
		// with IP packet as data
		//
		msg = this->newMsgWithBodyPtr(msg_ip, *ip_pkt_it, sizeof(*ip_pkt_it));
		if(!msg)
		{
			UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
			delete *ip_pkt_it;
			continue;
		}

		// send the message to the upper layer
		if(this->sendMsgTo(this->getUpperLayer(), msg) == mgl_ko)
		{
			UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
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
	return mgl_ok;

error:
	return mgl_ko;
}

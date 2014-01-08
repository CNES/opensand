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
 * @file BlockEncapSat.cpp
 * @brief Generic Encapsulation Bloc for SE
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "opensand_conf/uti_debug.h"


#include "BlockEncapSat.h"

#include "Plugin.h"


Event *BlockEncapSat::error_init = NULL;

BlockEncapSat::BlockEncapSat(const string &name):
	Block(name)
{
	// TODO we need a mutex here because some parameters may be used in upward and downward
	this->enableChannelMutex();
	
	if(error_init == NULL)
	{
		error_init = Output::registerEvent("BlockEncapSat:init", LEVEL_ERROR);
	}
}

BlockEncapSat::~BlockEncapSat()
{
}

bool BlockEncapSat::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_timer:
		{
			// timer event, flush corresponding encapsulation context
			return this->onTimer(event->getFd());
		}
		break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockEncapSat::onUpwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			NetBurst *burst;

			UTI_DEBUG("message received from the lower layer\n");
			burst = (NetBurst *)((MessageEvent *)event)->getData();
			return this->onRcvBurstFromDown(burst);
		}

		break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockEncapSat::onInit()
{
	const char *FUNCNAME = "[BlockEncapSat::onInit]";
	int i;
	int encap_nbr;
	EncapPlugin *plugin = NULL;
	EncapPlugin *upper_encap = NULL;
	// The list of uplink encapsulation protocols to ignore them at downlink
	// encapsulation
	vector <string> up_proto;

	// get the number of encapsulation context to use for up link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	for(i = 0; i < encap_nbr; i++)
	{
		string encap_name;

		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		up_proto.push_back(encap_name);
	}

	// get the number of encapsulation context to use for forward link
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}

	upper_encap = NULL;
	for(i = 0; i < encap_nbr; i++)
	{
		bool next = false;
		string encap_name;
		vector<string>::iterator iter;
		EncapPlugin::EncapContext *context;


		// get all the encapsulation to use from lower to upper
		if(!globalConfig.getValueInList(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
		                                POSITION, toString(i), ENCAP_NAME, encap_name))
		{
			UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
			          FUNCNAME, GLOBAL_SECTION, i, POSITION);
			goto error;
		}

		if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
		{
			UTI_ERROR("%s cannot get plugin for %s encapsulation",
			          FUNCNAME, encap_name.c_str());
			goto error;
		}

		context = plugin->getContext();
		for(iter = up_proto.begin(); iter!= up_proto.end(); iter++)
		{
			if(*iter == encap_name)
			{
				upper_encap = plugin;
				// no need to encapsulate with this protocol because it will
				// already be done on uplink
				next = true;
				break;
			}
		}
		if(next)
		{
			continue;
		}

		this->downlink_ctx.push_back(context);
		if(upper_encap != NULL &&
		   !context->setUpperPacketHandler(
					upper_encap->getPacketHandler(),
					REGENERATIVE))
		{
			UTI_ERROR("%s upper encapsulation type %s not supported for %s "
			          "encapsulation", FUNCNAME, upper_encap->getName().c_str(),
			          context->getName().c_str());
			goto error;
		}
		upper_encap = plugin;
		UTI_DEBUG("%s add downlink encapsulation layer: %s\n",
		          FUNCNAME, upper_encap->getName().c_str());
	}

	return true;

error:
	return false;
}

bool BlockEncapSat::onTimer(event_id_t timer_id)
{
	const char *FUNCNAME = "[BlockEncapSat::onTimer]";
	std::map<event_id_t, int>::iterator it;
	int id;
	NetBurst *burst;

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

	// flush the last encapsulation context
	burst = this->downlink_ctx.back()->flush(id);
	if(burst == NULL)
	{
		UTI_DEBUG("%s flushing context %d failed\n", FUNCNAME, id);
		goto error;
	}

	UTI_DEBUG("%s %zu encapsulation packet(s) flushed\n",
	          FUNCNAME, burst->size());

	if(burst->size() <= 0)
		goto clean;

	// send the message to the lower layer
	if(!this->sendDown((void **)&burst))
	{
		UTI_ERROR("failed to send burst to lower layer\n");
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	return true;

clean:
	delete burst;
error:
	return false;
}

bool BlockEncapSat::onRcvBurstFromDown(NetBurst *burst)
{
	const char *FUNCNAME = "[BlockEncapSat::onRcvBurstFromDown]";
	bool status;

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s message contains a burst of %d %s packet(s)\n",
	          FUNCNAME, burst->length(), burst->name().c_str());

	if(this->downlink_ctx.size() > 0)
	{
		status = this->EncapsulatePackets(burst);
	}
	else
	{
		status = this->ForwardPackets(burst);
	}

	return status;

error:
	return false;
}

bool BlockEncapSat::ForwardPackets(NetBurst *burst)
{
	const char *FUNCNAME = "[BlockEncapSat::ForwardPackets]";

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	// send the message to the lower layer
	if(!this->sendDown((void **)&burst))
	{
		UTI_ERROR("failed to send burst to lower layer\n");
		goto clean;
	}

	UTI_DEBUG("%s burst sent to the lower layer\n", FUNCNAME);

	// everthing is fine
	return true;

clean:
	delete burst;
error:
	return false;
}

bool BlockEncapSat::EncapsulatePackets(NetBurst *burst)
{
	const char *FUNCNAME = "[BlockEncapSat::EncapsulatePackets]";
	NetBurst::iterator pkt_it;
	NetBurst *packets;
	map<long, int> time_contexts;
	vector<EncapPlugin::EncapContext *>::iterator iter;
	bool status = false;

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	packets = burst;
	for(iter = this->downlink_ctx.begin(); iter != this->downlink_ctx.end();
	    iter++)
	{
		packets = (*iter)->encapsulate(packets, time_contexts);
		if(packets == NULL)
		{
			UTI_ERROR("%s encapsulation failed in %s context\n",
			          FUNCNAME, (*iter)->getName().c_str());
			goto clean;
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

		// set a new timer if no timer was found
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

	// create and send message only if at least one packet was created
	if(packets->size() <= 0)
	{
		status = true;
		goto clean;
	}

	// send the message to the lower layer
	if(!this->sendDown((void **)&packets))
	{
		UTI_ERROR("failed to send burst to lower layer\n");
		goto clean;
	}

	UTI_DEBUG("%s %s burst sent to the lower layer\n", FUNCNAME,
	          (this->downlink_ctx.back())->getName().c_str());

	// everthing is fine
	return true;

clean:
	delete packets;
error:
	return status;
}

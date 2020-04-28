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
 * @file BlockEncapSat.cpp
 * @brief Generic Encapsulation Bloc for SE
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */



#include "BlockEncapSat.h"

#include <opensand_output/Output.h>

#include "Plugin.h"


BlockEncapSat::BlockEncapSat(const string &name):
	Block(name),
	lan_plugin()
{
	// register static log
	NetBurst::log_net_burst = Output::Get()->registerLog(LEVEL_WARNING, "NetBurst");
}

BlockEncapSat::~BlockEncapSat()
{
}

bool BlockEncapSat::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_timer:
		{
			// timer event, flush corresponding encapsulation context
			return this->onTimer(event->getFd());
		}
		break;

		case evt_message:
		{
			NetBurst *burst;

			LOG(this->log_receive, LEVEL_INFO,
			    "message received from the lower layer\n");
			burst = (NetBurst *)((MessageEvent *)event)->getData();
			return this->onRcvBurst(burst);
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockEncapSat::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			NetBurst *burst;

			LOG(this->log_receive, LEVEL_INFO,
			    "message received from the lower layer\n");
			burst = (NetBurst *)((MessageEvent *)event)->getData();

			if(!this->onRcvBurst(burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to transmist burst to opposite "
				    "block\n");
				delete burst;
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockEncapSat::Upward::onRcvBurst(NetBurst *burst)
{
	unsigned int nb_bursts;
	spot_id_t spot;
	vector<EncapPlugin::EncapContext *>::iterator iter;
	NetBurst::iterator packet;

	// Check burst validity
	if(!burst)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	// Check packets count
	nb_bursts = burst->size();
	LOG(this->log_receive, LEVEL_INFO,
	    "message contains a burst of %d %s packet(s)\n",
	    nb_bursts, burst->name().c_str());
	if(burst->size() == 0)
	{
		delete burst;
		return true;
	}

	// Check encapsulation contexts
	if(this->uplink_ctx.size() == 0)
	{
		goto forward;
	}

	// Get spot id
	packet = burst->begin();
	spot = (*packet)->getSpot();
	LOG(this->log_receive, LEVEL_DEBUG,
	    "burst spot %u\n",
	    spot);

	// iterate on all the deencapsulation contexts to get the ip packets
	for(iter = this->uplink_ctx.begin();
			iter != this->uplink_ctx.end();
			++iter)
	{
		burst = (*iter)->deencapsulate(burst);
		if(!burst)
		{
			LOG(this->log_receive, LEVEL_ERROR,
					"deencapsulation failed in %s context\n",
					(*iter)->getName().c_str());
			goto error;
		}
	}
	LOG(this->log_receive, LEVEL_INFO,
			"%d %s packet => %zu %s packet(s)\n",
			nb_bursts, this->uplink_ctx[0]->getName().c_str(),
			burst->size(), burst->name().c_str());

	// Set spot id
	for(packet = burst->begin(); packet != burst->end(); ++packet)
	{
		(*packet)->setSpot(spot);
	}

forward:
	// send the burst to the opposite channel
	if(!this->shareMessage((void **)&burst))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to the opposite channel\n");
		delete burst;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "burst of deencapsulated packets sent to the "
	    "opposite channel\n");

	// everthing is fine
	return true;

error:
	return false;
}


bool BlockEncapSat::onInit()
{
	bool remove;
	int j;
	unsigned int n;

	vector<string> up_proto;
	vector<string> down_proto;

	vector<EncapPlugin::EncapContext *> up_ctx;
	vector<EncapPlugin::EncapContext *> down_ctx;

	StackPlugin::StackPacketHandler *top_pkt_hdl;
	StackPlugin::StackPacketHandler *upper_pkt_hdl;

	// Get the number of encapsulation context to use for up link
	if(!Conf::getNbListItems(Conf::section_map[COMMON_SECTION],
		                     RETURN_UP_ENCAP_SCHEME_LIST,
	                         n))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", COMMON_SECTION,
		    RETURN_UP_ENCAP_SCHEME_LIST);
		goto error;
	}

	// Get all the encapsulation to use from lower to upper for uplink
	for(unsigned int i = 0; i < n; ++i)
	{
		string encap_name;

		if(!Conf::getValueInList(Conf::section_map[COMMON_SECTION],
			                     RETURN_UP_ENCAP_SCHEME_LIST,
		                         POSITION, toString((int)i),
		                         ENCAP_NAME, encap_name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, invalid value %d for parameter '%s'\n",
			    COMMON_SECTION, i, POSITION);
			goto error;
		}
		up_proto.push_back(encap_name);
	}

	// Get the number of encapsulation context to use for downlink
	if(!Conf::getNbListItems(Conf::section_map[COMMON_SECTION],
		                     FORWARD_DOWN_ENCAP_SCHEME_LIST,
	                         n))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", COMMON_SECTION,
		    RETURN_UP_ENCAP_SCHEME_LIST);
		goto error;
	}

	// Get all the encapsulation to use from lower to upper for downlink
	for(unsigned int i = 0; i < n; ++i)
	{
		string encap_name;

		if(!Conf::getValueInList(Conf::section_map[COMMON_SECTION],
		                         FORWARD_DOWN_ENCAP_SCHEME_LIST,
		                         POSITION, toString((int)i),
		                         ENCAP_NAME, encap_name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, invalid value %d for parameter '%s'\n",
			    COMMON_SECTION, i, POSITION);
			goto error;
		}
		down_proto.push_back(encap_name);
	}

	// Check stacks size
	if(down_proto.size() < up_proto.size())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Invalid encapsulation stacks: "
		    "fewer downlink encapsulations than uplink");
		goto error;
	}

	// Remove useless encapsulations for the satellite
	remove = false;
	top_pkt_hdl = NULL;
	j = (int)(up_proto.size() - 1);
	while(0 <= j)
	{
		// Check downlink encapsulation
		if(up_proto[j] != down_proto[j])
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Invalid encapsulation stacks: "
			    "no %s encapsulation in down link",
			    up_proto[j].c_str());
			goto error;
		}

		EncapPlugin *plugin;

		// Get plugin
		if(!Plugin::getEncapsulationPlugin(up_proto[j], &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Can not get plugin for uplink %s encapsulation",
			    up_proto[j].c_str());
			goto error;
		}

		if(!remove)
		{
			// Check it handles variable length
			if(plugin->getPacketHandler()->getFixedLength() == 0)
			{
				--j;
				continue;
			}

			// Save the top packet handler (first plugin removed)
			top_pkt_hdl = plugin->getPacketHandler();
		}

		// Remove encapsulations
		down_proto.erase(down_proto.begin() + j);
		up_proto.erase(up_proto.begin() + j);
		--j;
		remove = true;
	}

	// Check top pkt handler is not null
	if(!top_pkt_hdl)
	{
		top_pkt_hdl = this->lan_plugin.getPacketHandler();
	}

	// Load downlink encapsulation
	upper_pkt_hdl = top_pkt_hdl;
	for(unsigned int i = 0; i < down_proto.size(); ++i)
	{
		EncapPlugin *plugin;
		EncapPlugin::EncapContext *ctx;

		// Get plugin
		if(!Plugin::getEncapsulationPlugin(down_proto[i], &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Can not get plugin for downlink %s encapsulation",
			    down_proto[i].c_str());
			goto error;
		}

		// Get context
		ctx = plugin->getContext();

		// Set upper encapsulation if required
		if(upper_pkt_hdl
		   && !ctx->setUpperPacketHandler(upper_pkt_hdl, REGENERATIVE))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "upper %s encapsulation is not supported "
			    "for %s encapsulation",
			    upper_pkt_hdl->getName().c_str(),
			    plugin->getName().c_str());
			goto error;
		}

		// Save context
		down_ctx.push_back(ctx);
		upper_pkt_hdl = plugin->getPacketHandler();

		LOG(this->log_init, LEVEL_INFO,
		    "Add downlink encapsulation layer: %s\n",
		    upper_pkt_hdl->getName().c_str());
	}

	// Load uplink encapsulation
	upper_pkt_hdl = top_pkt_hdl;
	for(unsigned int  i = 0; i < up_proto.size(); ++i)
	{
		EncapPlugin *plugin;
		EncapPlugin::EncapContext *ctx;

		// Get plugin
		if(!Plugin::getEncapsulationPlugin(up_proto[i], &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Can not get plugin for uplink %s encapsulation",
			    up_proto[i].c_str());
			goto error;
		}

		// Get context
		ctx = plugin->getContext();

		// Set upper encapsulation if required
		if(upper_pkt_hdl
		   && !ctx->setUpperPacketHandler(upper_pkt_hdl, REGENERATIVE))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "upper %s encapsulation is not supported "
			    "for %s encapsulation",
			    upper_pkt_hdl->getName().c_str(),
			    plugin->getName().c_str());
			goto error;
		}

		// Save context
		up_ctx.push_back(ctx);
		upper_pkt_hdl = plugin->getPacketHandler();

		LOG(this->log_init, LEVEL_INFO,
		    "Add uplink encapsulation layer: %s\n",
		    upper_pkt_hdl->getName().c_str());
	}

	// Set encap light stacks to channels
	std::reverse(up_ctx.begin(), up_ctx.end());
	((Upward *)this->upward)->setUplinkContexts(up_ctx);
	((Downward *)this->downward)->setDownlinkContexts(down_ctx);

	return true;

error:
	return false;
}

void BlockEncapSat::Downward::setDownlinkContexts(const vector<EncapPlugin::EncapContext *> &ctx)
{
	this->downlink_ctx = ctx;
}

void BlockEncapSat::Upward::setUplinkContexts(const vector<EncapPlugin::EncapContext *> &ctx)
{
	this->uplink_ctx = ctx;
}

bool BlockEncapSat::Downward::onTimer(event_id_t timer_id)
{
	std::map<event_id_t, int>::iterator it;
	int id;
	NetBurst *burst;

	LOG(this->log_receive, LEVEL_INFO,
	    "emission timer received, flush corresponding emission "
	    "context\n");

	// find encapsulation context to flush
	it = this->timers.find(timer_id);
	if(it == this->timers.end())
	{
		LOG(this->log_receive, LEVEL_ERROR, "timer not found\n");
		goto error;
	}

	// context found
	id = (*it).second;
	LOG(this->log_receive, LEVEL_INFO,
	    "corresponding emission context found (ID = %d)\n", id);

	// remove emission timer from the list
	this->removeEvent((*it).first);
	this->timers.erase(it);

	// flush the last encapsulation context
	burst = this->downlink_ctx.back()->flush(id);
	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_INFO,
		    "flushing context %d failed\n", id);
		goto error;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "%zu encapsulation packet(s) flushed\n", burst->size());

	if(burst->size() <= 0)
		goto clean;

	// send the message to the lower layer
	if(!this->enqueueMessage((void **)&burst))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		goto clean;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	return true;

clean:
	delete burst;
error:
	return false;
}

bool BlockEncapSat::Downward::onRcvBurst(NetBurst *burst)
{
	bool status;

	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "message contains a burst of %d %s packet(s)\n",
	    burst->length(), burst->name().c_str());

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

bool BlockEncapSat::Downward::ForwardPackets(NetBurst *burst)
{
	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_send, LEVEL_ERROR, "burst is not valid\n");
		goto error;
	}

	// send the message to the lower layer
	if(!this->enqueueMessage((void **)&burst))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		goto clean;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "burst sent to the lower layer\n");

	// everthing is fine
	return true;

clean:
	delete burst;
error:
	return false;
}

bool BlockEncapSat::Downward::EncapsulatePackets(NetBurst *burst)
{
	NetBurst::iterator pkt_it;
	NetBurst *packets;
	map<long, int> time_contexts;
	vector<EncapPlugin::EncapContext *>::iterator iter;
	bool status = false;

	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	packets = burst;
	for(iter = this->downlink_ctx.begin(); iter != this->downlink_ctx.end();
	    iter++)
	{
		packets = (*iter)->encapsulate(packets, time_contexts);
		if(packets == NULL)
		{
			LOG(this->log_send, LEVEL_ERROR,
			    "encapsulation failed in %s context\n",
			    (*iter)->getName().c_str());
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
			timer = this->addTimerEvent(name.str(),
			                            (*time_iter).first,
			                            false);

			this->timers.insert(std::make_pair(timer, (*time_iter).second));
			LOG(this->log_send, LEVEL_INFO,
			    "timer for context ID %d armed with "
			    "%ld ms\n", (*time_iter).second,
			    (*time_iter).first);
		}
		else
		{
			LOG(this->log_send, LEVEL_INFO,
			    "timer already set for context ID %d\n",
			    (*time_iter).second);
		}
	}

	// create and send message only if at least one packet was created
	if(packets->size() <= 0)
	{
		status = true;
		goto clean;
	}

	// send the message to the lower layer
	if(!this->enqueueMessage((void **)&packets))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		goto clean;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "%s burst sent to the lower layer\n",
	    (this->downlink_ctx.back())->getName().c_str());

	// everthing is fine
	return true;

clean:
	delete packets;
error:
	return status;
}

BlockEncapSat::SatLanPlugin::SatLanPlugin():
	StackPlugin((uint16_t)0)
{
	this->packet_handler = new BlockEncapSat::SatLanPlugin::PacketHandler(*this);
	this->context = new BlockEncapSat::SatLanPlugin::Context(*this);
}

NetPacket *BlockEncapSat::SatLanPlugin::PacketHandler::build(const Data &data,
		size_t data_length,
		uint8_t qos,
		uint8_t src_tal_id,
		uint8_t dst_tal_id) const
{
	LOG(this->log, LEVEL_DEBUG, "LAN build packet from tal %u to tal %u with qos %u"
	    " (len %u bytes)",
		src_tal_id,
		dst_tal_id,
		qos,
		data_length);
	return new NetPacket(data,
			data_length,
			this->getName(),
			this->getEtherType(),
			qos,
			src_tal_id,
			dst_tal_id,
			0);
}

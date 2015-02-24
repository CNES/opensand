/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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


#include "TerminalCategoryDama.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <stdint.h>


BlockEncap::BlockEncap(const string &name, tal_id_t mac_id):
	Block(name),
	group_id(-1),
	tal_id(-1),
	state(link_down),
	mac_id(mac_id)
{
	// TODO we need a mutex here because some parameters may be used in upward and downward
	this->enableChannelMutex();
	// register static log
	NetBurst::log_net_burst = Output::registerLog(LEVEL_WARNING, "NetBurst");
}

BlockEncap::~BlockEncap()
{
}


bool BlockEncap::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_timer:
		{
			// timer event, flush corresponding encapsulation context
			LOG(this->log_rcv_from_up, LEVEL_INFO,
			    "Timer received %s\n", event->getName().c_str());
			return this->onTimer(event->getFd());
		}
		break;

		case evt_message:
		{
			// message received from another bloc
			LOG(this->log_rcv_from_up, LEVEL_INFO,
			    "message received from the upper-layer bloc\n");
			NetBurst *burst;
			burst = (NetBurst *)((MessageEvent *)event)->getData();
			return this->onRcvBurstFromUp(burst);
		}
		break;

		default:
			LOG(this->log_rcv_from_up, LEVEL_ERROR,
			    "unknown event received %s",
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
			LOG(this->log_rcv_from_down, LEVEL_INFO,
			    "message received from the lower layer\n");

			if(((MessageEvent *)event)->getMessageType() == msg_link_up)
			{
				T_LINK_UP *link_up_msg;
				vector<EncapPlugin::EncapContext*>::iterator encap_it;

				// 'link up' message received => forward it to upper layer
				LOG(this->log_rcv_from_down, LEVEL_INFO,
				    "'link up' message received, forward it\n");

				link_up_msg = (T_LINK_UP *)((MessageEvent *)event)->getData();
				if(this->state == link_up)
				{
					LOG(this->log_rcv_from_down, LEVEL_NOTICE,
					    "duplicate link up msg\n");
					delete link_up_msg;
					return false;
				}

				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = link_up;

				// send the message to the upper layer
				if(!this->sendUp((void **)&link_up_msg,
					             sizeof(T_LINK_UP), msg_link_up))
				{
					LOG(this->log_rcv_from_down, LEVEL_ERROR,
					    "cannot forward 'link up' message\n");
					delete link_up_msg;
					return false;
				}

				LOG(this->log_rcv_from_down, LEVEL_INFO,
				    "'link up' message sent to the upper layer\n");

				// Set tal_id 'filter' for reception context
				
	
				for(encap_it = this->reception_ctx.begin();
				    encap_it != this->reception_ctx.end();
				    ++encap_it)
				{
					(*encap_it)->setFilterTalId(this->tal_id);
				}

				for(encap_it = this->reception_ctx_scpc.begin();
				    encap_it != this->reception_ctx_scpc.end();
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
			LOG(this->log_rcv_from_down, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockEncap::onInit()
{
	string up_return_encap_proto;
	string downlink_encap_proto;
	string lan_name;
	string satellite_type;
	ConfigurationList option_list;
	vector <EncapPlugin::EncapContext *> up_return_ctx;
	vector <EncapPlugin::EncapContext *> up_return_ctx_scpc;
	vector <EncapPlugin::EncapContext *> down_forward_ctx;
	int lan_nbr;
	int i=0;
	LanAdaptationPlugin *lan_plugin = NULL;
	string compo_name;
	component_t host;

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to init output\n");
		goto error;
	}

	// satellite type: regenerative or transparent ?
	if(!Conf::getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                   satellite_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	

	LOG(this->log_init, LEVEL_INFO,
	    "satellite type = %s\n", satellite_type.c_str());

	// Retrieve last packet handler in lan adaptation layer
	if(!Conf::getNbListItems(GLOBAL_SECTION, LAN_ADAPTATION_SCHEME_LIST,
	                         lan_nbr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", GLOBAL_SECTION,
		    LAN_ADAPTATION_SCHEME_LIST);
		goto error;
	}
	if(!Conf::getValueInList(GLOBAL_SECTION, LAN_ADAPTATION_SCHEME_LIST,
	                         POSITION, toString(lan_nbr - 1),
	                         PROTO, lan_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, invalid value %d for parameter "
		    "'%s' in %s\n", GLOBAL_SECTION, i, POSITION,
		    LAN_ADAPTATION_SCHEME_LIST);
		goto error;
	}
	if(!Plugin::getLanAdaptationPlugin(lan_name, &lan_plugin))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get plugin for %s lan adaptation",
		    lan_name.c_str());
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "lan adaptation upper layer is %s\n", lan_name.c_str());
	
	//TODO: Check if Tal is in SCPC mode
	if (this->mac_id != GW_TAL_ID)
	{
		LOG(this->log_init, LEVEL_DEBUG,
	    "Going to check if Tal with id:  %d is in Scpc mode\n", this->mac_id);
	

		if(this->checkIfScpc(satellite_type))
		{
			LOG(this->log_init, LEVEL_INFO,
				"SCPC mode available for ST %d - BlockEncap \n", this->mac_id);
			if(!this->getEncapContext(GLOBAL_SECTION, RETURN_UP_ENCAP_SCHEME_LIST, 
			                          lan_plugin, up_return_ctx, satellite_type,
			                          "return/up", true)) 
			{
				LOG(this->log_init, LEVEL_ERROR,
					"Cannot get Up/Return GSE Encapsulation context");
				goto error;
			}
		}
	
		else
		{
			LOG(this->log_init, LEVEL_INFO,
				"SCPC mode not available for ST %d - BlockEncap \n", this->mac_id);
			if(!this->getEncapContext(GLOBAL_SECTION, RETURN_UP_ENCAP_SCHEME_LIST,
				                     lan_plugin, up_return_ctx, satellite_type,
					                 "return/up", false)) 
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Cannot get Up/Return Encapsulation context");
				goto error;
			}
		}
	}
	else
	{
		LOG(this->log_init, LEVEL_NOTICE,
			"SCPC mode available - BlockEncap");
		if(!this->getEncapContext(GLOBAL_SECTION, RETURN_UP_ENCAP_SCHEME_LIST, 
		                          lan_plugin, up_return_ctx_scpc, satellite_type,
		                          "return/up", true)) 
		{
			LOG(this->log_init, LEVEL_ERROR,
				"Cannot get Up/Return GSE Encapsulation context");
			goto error;
		}

		if(!this->getEncapContext(GLOBAL_SECTION, RETURN_UP_ENCAP_SCHEME_LIST, 
		                          lan_plugin, up_return_ctx, satellite_type,
		                          "return/up", false)) 
		{
			LOG(this->log_init, LEVEL_ERROR,
				"Cannot get Up/Return Encapsulation context");
			goto error;
		}


	}

	if(!this->getEncapContext(GLOBAL_SECTION, FORWARD_DOWN_ENCAP_SCHEME_LIST,
	                         lan_plugin, down_forward_ctx, satellite_type,
	                         "forward/down", false)) 
	{
		LOG(this->log_init, LEVEL_ERROR,
	    "Cannot get Down/Forward Encapsulation context");
		goto error;
	}

	// get host type
	compo_name = "";
	if(!Conf::getComponent(compo_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get component type\n");
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE, "host type = %s\n",
	    compo_name.c_str());
	host = getComponentType(compo_name);

	if(host == terminal || satellite_type == "regenerative")
	{
		this->emission_ctx = up_return_ctx;
		this->reception_ctx = down_forward_ctx;
	}
	else
	{
		this->reception_ctx = up_return_ctx;
		this->reception_ctx_scpc = up_return_ctx_scpc;
		this->emission_ctx = down_forward_ctx;
	}
	// reorder reception context to get the deencapsulation contexts in the
	// right order
	reverse(this->reception_ctx.begin(), this->reception_ctx.end());

	return true;
error:
	return false;
}

bool BlockEncap::initOutput()
{
	this->log_init = Output::registerLog(LEVEL_WARNING, "Encap.init");
	this->log_rcv_from_up = Output::registerLog(LEVEL_WARNING, "Encap.Downward.receive");
	this->log_rcv_from_down = Output::registerLog(LEVEL_WARNING, "Encap.Upward.receive");
	this->log_send_down = Output::registerLog(LEVEL_WARNING, "Encap.Downward.send");
	return true;
}

bool BlockEncap::onTimer(event_id_t timer_id)
{
	std::map<event_id_t, int>::iterator it;
	int id;
	NetBurst *burst;
	bool status = false;

	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "emission timer received, flush corresponding emission "
	    "context\n");

	// find encapsulation context to flush
	it = this->timers.find(timer_id);
	if(it == this->timers.end())
	{
		LOG(this->log_rcv_from_up, LEVEL_ERROR,
		    "timer not found\n");
		goto error;
	}

	// context found
	id = (*it).second;
	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "corresponding emission context found (ID = %d)\n",
	    id);

	// remove emission timer from the list
	this->downward->removeEvent((*it).first);
	this->timers.erase(it);

	// flush the last encapsulation contexts
	burst = (this->emission_ctx.back())->flush(id);
	if(burst == NULL)
	{
		LOG(this->log_rcv_from_up, LEVEL_ERROR,
		    "flushing context %d failed\n", id);
		goto error;
	}

	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "%zu encapsulation packets flushed\n",
	    burst->size());

	if(burst->size() <= 0)
	{
		status = true;
		goto clean;
	}

	// send the message to the lower layer
	if(!this->sendDown((void **)&burst))
	{
		LOG(this->log_rcv_from_up, LEVEL_ERROR,
		    "cannot send burst to lower layer failed\n");
		goto clean;
	}

	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	return true;

clean:
	delete burst;
error:
	return status;
}

bool BlockEncap::onRcvBurstFromUp(NetBurst *burst)
{
	map<long, int> time_contexts;
	vector<EncapPlugin::EncapContext *>::iterator iter;
	string name;
	size_t size;
	bool status = false;

	// check packet validity
	if(burst == NULL)
	{
		LOG(this->log_rcv_from_up, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	name = burst->name();
	size = burst->size();
	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "encapsulate %zu %s packet(s)\n",
	    size, name.c_str());

	// encapsulate packet
	for(iter = this->emission_ctx.begin(); iter != this->emission_ctx.end();
	    iter++)
	{
		burst = (*iter)->encapsulate(burst, time_contexts);
		if(burst == NULL)
		{
			LOG(this->log_rcv_from_up, LEVEL_ERROR,
			    "encapsulation failed in %s context\n",
			    (*iter)->getName().c_str());
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
			LOG(this->log_rcv_from_up, LEVEL_INFO,
			    "timer for context ID %d armed with %ld ms\n",
			    (*time_iter).second, (*time_iter).first);
		}
		else
		{
			LOG(this->log_rcv_from_up, LEVEL_INFO,
			    "timer already set for context ID %d\n",
			    (*time_iter).second);
		}
	}

	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_rcv_from_up, LEVEL_ERROR,
		    "encapsulation failed\n");
		goto error;
	}

	if(burst->size() > 0)
	{
		LOG(this->log_rcv_from_up, LEVEL_INFO,
		    "encapsulation packet of type %s (QoS = %d)\n",
		    burst->front()->getName().c_str(),
		    burst->front()->getQos());
	}

	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "%zu %s packet => %zu encapsulation packet(s)\n",
	    size, name.c_str(), burst->size());

	// if no encapsulation packet was created, avoid sending a message
	if(burst->size() <= 0)
	{
		status = true;
		goto clean;
	}


	// send the message to the lower layer
	if(!this->sendDown((void **)&burst))
	{
		LOG(this->log_rcv_from_up, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		goto clean;
	}

	LOG(this->log_rcv_from_up, LEVEL_INFO,
	    "encapsulation burst sent to the lower layer\n");

	// everything is fine
	return true;

clean:
	delete burst;
error:
	return status;
}

bool BlockEncap::onRcvBurstFromDown(NetBurst *burst)
{
	vector <EncapPlugin::EncapContext *>::iterator iter;
	unsigned int nb_bursts;


	// check burst validity
	if(burst == NULL)
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "burst is not valid\n");
		goto error;
	}

	nb_bursts = burst->size();
	LOG(this->log_rcv_from_down, LEVEL_INFO,
	    "message contains a burst of %d %s packet(s)\n",
	    nb_bursts, burst->name().c_str());

	if(burst->name() == "GSE" && this->mac_id == GW_TAL_ID)
	{
		// iterate on all the deencapsulation contexts to get the ip packets
		for(iter = this->reception_ctx_scpc.begin(); iter != this->reception_ctx_scpc.end();
		    ++iter)
		{
			burst = (*iter)->deencapsulate(burst);
			if(burst == NULL)
			{
				LOG(this->log_rcv_from_down, LEVEL_ERROR,
				    "deencapsulation failed in %s context\n",
				    (*iter)->getName().c_str());
				goto error;
			}
		}
		LOG(this->log_rcv_from_down, LEVEL_INFO,
			"%d %s packet => %zu %s packet(s)\n",
			nb_bursts, this->reception_ctx_scpc[0]->getName().c_str(),
			burst->size(), burst->name().c_str());
	}
	else
	{
		// iterate on all the deencapsulation contexts to get the ip packets
		for(iter = this->reception_ctx.begin(); iter != this->reception_ctx.end();
		    ++iter)
		{
			burst = (*iter)->deencapsulate(burst);
			if(burst == NULL)
			{
				LOG(this->log_rcv_from_down, LEVEL_ERROR,
				    "deencapsulation failed in %s context\n",
				    (*iter)->getName().c_str());
				goto error;
			}
		}
		LOG(this->log_rcv_from_down, LEVEL_INFO,
			"%d %s packet => %zu %s packet(s)\n",
			nb_bursts, this->reception_ctx[0]->getName().c_str(),
			burst->size(), burst->name().c_str());
	}

	if(burst->size() == 0)
	{
		delete burst;
		return true;
	}

	// send the burst to the upper layer
	if(!this->sendUp((void **)&burst))
	{
		LOG(this->log_rcv_from_down, LEVEL_ERROR,
		    "failed to send burst to upper layer\n");
		delete burst;
	}

	LOG(this->log_rcv_from_down, LEVEL_INFO,
	    "burst of deencapsulated packets sent to the upper "
	    "layer\n");

	// everthing is fine
	return true;

error:
	return false;
}

bool BlockEncap::checkIfScpc(string &sat_type)
{
	TerminalCategories<TerminalCategoryDama> scpc_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalMapping<TerminalCategoryDama>::const_iterator tal_map_it;
	TerminalCategoryDama *default_category;
	TerminalCategoryDama *tal_category = NULL;
	time_ms_t scpc_carr_duration_ms;
	FmtSimulation scpc_fmt_simu;
	fifos_t dvb_fifos;
	fmt_groups_t ret_fmt_groups;
	bool is_scpc = false;
	
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	if(!Conf::getListItems(DVB_TAL_SECTION, FIFO_LIST, fifo_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing fifo list", DVB_TAL_SECTION,
		    FIFO_LIST);
		goto no_scpc;
	}

	for(iter = fifo_list.begin(); iter != fifo_list.end(); iter++)
	{
		string fifo_access_type;

		// get the fifo CR type
		if(!Conf::getAttributeValue(iter, FIFO_ACCESS_TYPE, fifo_access_type))
		{
			LOG(this->log_init, LEVEL_ERROR,
			"cannot get %s from section '%s, %s'\n",
			FIFO_ACCESS_TYPE, DVB_TAL_SECTION,
			FIFO_LIST);
			goto no_scpc;
		}
		
		if(fifo_access_type == "SCPC")
		{   
			is_scpc = true;
		}
	}
	
	if(!is_scpc)
	{
		goto no_scpc;
	}	

	if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2,
							  FORWARD_DOWN_MODCOD_TIME_SERIES,
							  scpc_fmt_simu))
	{
		LOG(this->log_init, LEVEL_ERROR,
			"failed to initialize the down/forward MODCOD files\n");
		goto no_scpc;
	}
	
	//  Duration of the carrier -- in ms
	if(!Conf::getValue(SCPC_SECTION, SCPC_C_DURATION,
	                   scpc_carr_duration_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", SCPC_C_DURATION);
		goto no_scpc;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "scpc_carr_duration_ms = %d ms\n", scpc_carr_duration_ms);



	if(!this->initBand<TerminalCategoryDama>(RETURN_UP_BAND,
	                                         SCPC,
	                                         scpc_carr_duration_ms,
	                                         sat_type,
	                                         scpc_fmt_simu.getModcodDefinitions(),
	                                         scpc_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         ret_fmt_groups))
	{
		goto no_scpc;
	}

	if(scpc_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No SCPC carriers\n");
		goto no_scpc;
	}
	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by SCPC
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a SCPC category\n");
			goto no_scpc;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = (*tal_map_it).second;
	}
	
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No SCPC carrier\n");
		//Even if there are SCPC FIFOs, SCPC is no used because there are no SCPC carriers
		goto no_scpc;
	}
	
	return true;
	
	no_scpc: 
		return false;
	
}

bool BlockEncap::getEncapContext(const char *section, 
	                             const char *scheme_list,
	                             LanAdaptationPlugin *l_plugin,
	                             vector <EncapPlugin::EncapContext *> &ctx,
	                             string &sat_type,
	                             const char *link_type, bool scpc_scheme)
{
	StackPlugin *upper_encap = NULL;
	EncapPlugin *plugin;
	int encap_nbr = 1;
	int i;
	
	
	if(!scpc_scheme)
	{
		// get the number of encapsulation context if Tal is not in SCPC mode
		if(!Conf::getNbListItems(section, scheme_list,
		                         encap_nbr))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, %s missing\n", section,
				scheme_list);
			goto error;
		}
	}

	upper_encap = l_plugin;

	// get all the encapsulation to use upper to lower
	for(i = 0; i < encap_nbr; i++)
	{
	
		string encap_name;
		EncapPlugin::EncapContext *context;
		
		// If Tal is in SCPC mode, only GSE is allowed
		if(!scpc_scheme)
		{
			if(!Conf::getValueInList(section, scheme_list,
				                     POSITION, toString(i), ENCAP_NAME, encap_name))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Section %s, invalid value %d for parameter '%s'\n",
				    section, i, POSITION);
				goto error;
			}
		}
		else
		{
			encap_name = "GSE";
			LOG(this->log_init, LEVEL_INFO,
			    "Setting the encapsulation to %s because SCPC is %d\n",
			    encap_name.c_str(), scpc_scheme);
		}

		if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get plugin for %s encapsulation",
			    encap_name.c_str());
			goto error;
		}

		context = plugin->getContext();
		ctx.push_back(context);
		if(!context->setUpperPacketHandler(
					upper_encap->getPacketHandler(),
					strToSatType(sat_type)))
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




bool BlockEncap::initModcodFiles(const char *def,
                                 const char *simu,
                                 FmtSimulation &fmt_simu)
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD simulations and definitions for down/forward link
	if(!Conf::getValue(PHYSICAL_LAYER_SECTION, simu,
	                   modcod_simu_file))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, simu);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "down/forward link MODCOD simulation path set to %s\n",
	    modcod_simu_file.c_str());

	if(!Conf::getValue(PHYSICAL_LAYER_SECTION, def,
	                   modcod_def_file))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, def);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "down/forward link MODCOD definition path set to %s\n",
	    modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!fmt_simu.setModcodDef(modcod_def_file))
	{
		goto error;
	}

	// no need for simulation file if there is a physical layer
		// set the MODCOD simulation file
	if(!fmt_simu.setModcodSimu(modcod_simu_file))
	{
		goto error;
	}



	return true;

error:
	return false;
}

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
 * @file BlockDvb.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "BlockDvb.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>


BlockDvb::BlockDvb(const string &name):
	Block(name),
	satellite_type(),
	frame_duration_ms(),
	fwd_timer_ms(),
	frames_per_superframe(-1),
	super_frame_counter(0),
	frame_counter(0),
	ret_fmt_simu(),
	fwd_fmt_simu(),
	with_phy_layer(false),
	dvb_scenario_refresh(-1),
	stats_period_ms()
{
/*	event_login_received = Output::registerEvent("Dvb:login_received");
	event_login_response = Output::registerEvent("Dvb:login_response");*/

	// TODO remove
	this->log_rcv_from_down = Output::registerLog(LEVEL_WARNING,
	                                              "Dvb.Upward.receive");	
	this->log_send_up = Output::registerLog(LEVEL_WARNING,
	                                        "Dvb.Upward.send");
	this->log_send_down = Output::registerLog(LEVEL_WARNING,
	                                          "Dvb.Downward.send");
	this->log_rcv_from_up = Output::registerLog(LEVEL_WARNING,
	                                            "Dvb.Downward.receive");
	this->log_band = Output::registerLog(LEVEL_WARNING, "Dvb.Ncc.Band");	
	this->log_request_simulation = Output::registerLog(LEVEL_WARNING, 
	                                                   "Dvb.RequestSimulation");	
	this->log_qos_server = Output::registerLog(LEVEL_WARNING, 
	                                           "Dvb.QoSServer");	
	this->log_frame_tick = Output::registerLog(LEVEL_WARNING, 
	                                           "Dvb.DamaAgent.FrameTick");	
}


BlockDvb::~BlockDvb()
{
}

BlockDvb::DvbUpward::~DvbUpward()
{
	// release the reception DVB standards
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}
}

bool BlockDvb::initCommon()
{
	string encap_name;
	int encap_nbr;
	EncapPlugin *plugin;
	string sat_type;

	// satellite type
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          sat_type))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	// Retrieve the value of the ‘enable’ parameter for the physical layer
	if(!globalConfig.getValue(PHYSICAL_LAYER_SECTION, ENABLE,
	                          this->with_phy_layer))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Section %s, %s missing\n",
		                PHYSICAL_LAYER_SECTION, ENABLE);
		goto error;
	}

	// get the packet types
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Section %s, %s missing\n",
		                GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}


	// get all the encapsulation to use from lower to upper
	if(!globalConfig.getValueInList(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                POSITION, toString(encap_nbr - 1),
	                                ENCAP_NAME, encap_name))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Section %s, invalid value %d for parameter '%s'\n",
		                GLOBAL_SECTION, encap_nbr - 1, POSITION);
		goto error;
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot get plugin for %s encapsulation",
		                encap_name.c_str());
		goto error;
	}

	this->up_return_pkt_hdl = plugin->getPacketHandler();
	if(!this->up_return_pkt_hdl)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "up/return encapsulation scheme = %s\n",
	                this->up_return_pkt_hdl->getName().c_str());

	if(!globalConfig.getNbListItems(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Section %s, %s missing\n",
		                GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST);
		goto error;
	}

	// get all the encapsulation to use from lower to upper
	if(!globalConfig.getValueInList(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                POSITION, toString(encap_nbr - 1),
	                                ENCAP_NAME, encap_name))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Section %s, invalid value %d for parameter '%s'\n",
		                GLOBAL_SECTION, encap_nbr - 1, POSITION);
		goto error;
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "missing plugin for %s encapsulation",
		                encap_name.c_str());
		goto error;
	}

	this->down_forward_pkt_hdl = plugin->getPacketHandler();
	if(!this->down_forward_pkt_hdl)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "down/forward encapsulation scheme = %s\n",
	                this->down_forward_pkt_hdl->getName().c_str());

	// frame duration
	if(!globalConfig.getValue(GLOBAL_SECTION, DVB_F_DURATION,
	                          this->frame_duration_ms))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                GLOBAL_SECTION, DVB_F_DURATION);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "frame duration set to %d\n", this->frame_duration_ms);

	// forward timer
	if(!globalConfig.getValue(PERF_SECTION, FWD_TIMER,
	                          this->fwd_timer_ms))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                PERF_SECTION, FWD_TIMER);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "forward timer set to %u\n",
	                this->fwd_timer_ms);

	// number of frame per superframe
	if(!globalConfig.getValue(DVB_MAC_SECTION, DVB_FPF,
	                          this->frames_per_superframe))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                DVB_MAC_SECTION, DVB_FPF);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "frames_per_superframe set to %d\n",
	                this->frames_per_superframe);

	// scenario refresh interval
	if(!globalConfig.getValue(GLOBAL_SECTION, DVB_SCENARIO_REFRESH,
	                          this->dvb_scenario_refresh))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                GLOBAL_SECTION, DVB_SCENARIO_REFRESH);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "dvb_scenario_refresh set to %d\n",
	                this->dvb_scenario_refresh);

	// statistics timer
	if(!globalConfig.getValue(PERF_SECTION, STATS_TIMER,
	                          this->stats_period_ms))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                PERF_SECTION, STATS_TIMER);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "statistics_timer set to %d\n",
	                this->stats_period_ms);

	this->stats_timer = this->downward->addTimerEvent("dvb_stats",
	                                                  this->stats_period_ms);

	return true;
error:
	return false;
}

bool BlockDvb::initForwardModcodFiles()
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD simulations and definitions for down/forward link
	if(!globalConfig.getValue(GLOBAL_SECTION, DOWN_FORWARD_MODCOD_SIMU,
	                          modcod_simu_file))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s', missing parameter '%s'\n",
		                GLOBAL_SECTION, DOWN_FORWARD_MODCOD_SIMU);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "down/forward link MODCOD simulation path set to %s\n",
	                modcod_simu_file.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, DOWN_FORWARD_MODCOD_DEF,
	                          modcod_def_file))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s', missing parameter '%s'\n",
		                GLOBAL_SECTION, DOWN_FORWARD_MODCOD_DEF);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "down/forward link MODCOD definition path set to %s\n",
	                modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!this->fwd_fmt_simu.setModcodDef(modcod_def_file))
	{
		goto error;
	}

	// no need for simulation file if there is a physical layer
	if(!this->with_phy_layer)
	{
		// set the MODCOD simulation file
		if(!this->fwd_fmt_simu.setModcodSimu(modcod_simu_file))
		{
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvb::initReturnModcodFiles()
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD simulations and definitions for up/return link
	if(!globalConfig.getValue(GLOBAL_SECTION, UP_RETURN_MODCOD_SIMU,
	                          modcod_simu_file))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s', missing parameter '%s'\n",
		                GLOBAL_SECTION, UP_RETURN_MODCOD_SIMU);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "up/return link MODCOD simulation path set to %s\n",
	                modcod_simu_file.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, UP_RETURN_MODCOD_DEF,
	                          modcod_def_file))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s', missing parameter '%s'\n",
		                GLOBAL_SECTION, UP_RETURN_MODCOD_DEF);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "up/return link MODCOD definition path set to %s\n",
	                modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!this->ret_fmt_simu.setModcodDef(modcod_def_file))
	{
		goto error;
	}

	// no need for simulation file if there is a physical layer
	if(!this->with_phy_layer)
	{
		// set the MODCOD simulation file
		if(!this->ret_fmt_simu.setModcodSimu(modcod_simu_file))
		{
			goto error;
		}
	}

	return true;

error:
	return false;
}

bool BlockDvb::DvbDownward::sendBursts(list<DvbFrame *> *complete_frames,
                                       uint8_t carrier_id)
{
	list<DvbFrame *>::iterator frame_it;
	bool status = true;

	// send all complete DVB-RCS frames
	Output::sendLog(this->log_send, LEVEL_DEBUG,
	                "send all %zu complete DVB frames...\n",
	                complete_frames->size());
	for(frame_it = complete_frames->begin();
	    frame_it != complete_frames->end();
	    ++frame_it)
	{
		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(*frame_it, carrier_id))
		{
			status = false;
			if(*frame_it)
			{
				delete *frame_it;
			}
			continue;
		}

		// DVB frame is now sent, so delete its content
		//delete frame;
		Output::sendLog(this->log_send, LEVEL_INFO,
		                "complete DVB frame sent to carrier %u\n", carrier_id);
	}
	// clear complete DVB frames
	complete_frames->clear();

	return status;
}

bool BlockDvb::DvbDownward::sendDvbFrame(DvbFrame *dvb_frame, uint8_t carrier_id)
{
	if(!dvb_frame)
	{
		Output::sendLog(this->log_send, LEVEL_ERROR,
		                "frame is %p\n", dvb_frame);
		goto error;
	}

	dvb_frame->setCarrierId(carrier_id);

	if(dvb_frame->getTotalLength() <= 0)
	{
		Output::sendLog(this->log_send, LEVEL_ERROR,
		                "empty frame, header and payload are not present\n");
		goto error;
	}

	// send the message to the lower layer
	// do not count carrier_id in len, this is the dvb_meta->hdr length
	if(!this->enqueueMessage((void **)(&dvb_frame)))
	{
		Output::sendLog(this->log_send, LEVEL_ERROR,
		                "failed to send DVB frame to lower layer\n");
		goto release_dvb_frame;
	}
	Output::sendLog(this->log_send, LEVEL_INFO,
	                "DVB frame sent to the lower layer\n");

	return true;

release_dvb_frame:
	delete dvb_frame;
error:
	return false;
}


bool BlockDvb::DvbDownward::onRcvEncapPacket(NetPacket *packet,
                                             DvbFifo *fifo,
                                             time_ms_t fifo_delay)
{

	MacFifoElement *elem;
	clock_t current_time = this->getCurrentTime();

	// create a new satellite cell to store the packet
	elem = new MacFifoElement(packet, current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		Output::sendLog(this->log_receive, LEVEL_ERROR,
	                    "cannot allocate FIFO element, drop packet\n");
		goto error;
	}

	// append the new packet in the fifo
	if(!fifo->push(elem))
	{
		Output::sendLog(this->log_receive, LEVEL_ERROR,
		                "FIFO is full: drop packet\n");
		goto release_elem;
	}

	Output::sendLog(this->log_receive, LEVEL_INFO,
	                "encapsulation packet %s stored in FIFO "
	                "(tick_in = %ld, tick_out = %ld)\n",
	                packet->getName().c_str(),
	                elem->getTickIn(), elem->getTickOut());

	return true;

release_elem:
	delete elem;
error:
	delete packet;
	return false;
}


// TODO remove this function
bool BlockDvb::SendNewMsgToUpperLayer(NetBurst *burst)
{
	// send the message to the upper layer
	if(!this->sendUp((void **)&burst))
	{
		Output::sendLog(this->log_send_up, LEVEL_ERROR, 
		                "failed to send burst of packets to upper layer\n");
		goto release_burst;
	}
	Output::sendLog(this->log_send_up, LEVEL_INFO, 
	                "burst sent to the upper layer\n");

	return true;

release_burst:
	delete burst;
	return false;
}

bool BlockDvb::initBand(const char *band,
                        time_ms_t duration_ms,
                        TerminalCategories &categories,
                        TerminalMapping &terminal_affectation,
                        TerminalCategory **default_category,
                        fmt_groups_t &fmt_groups)
{
	freq_khz_t bandwidth_khz;
	double roll_off;
	freq_mhz_t bandwidth_mhz = 0;
	ConfigurationList conf_list;
	ConfigurationList aff_list;
	TerminalCategories::iterator cat_iter;
	unsigned int carrier_id = 0;
	int i;
	string default_category_name;

	// Get the value of the bandwidth for return link
	if(!globalConfig.getValue(band, BANDWIDTH,
	                          bandwidth_mhz))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		        "section '%s': missing parameter '%s'\n",
		        band, BANDWIDTH);
		goto error;
	}
	bandwidth_khz = bandwidth_mhz * 1000;
	Output::sendLog(this->log_band, LEVEL_INFO,
	                "%s: bandwitdh is %u kHz\n", band, bandwidth_khz);

	// Get the value of the roll off
	if(!globalConfig.getValue(band, ROLL_OFF,
	                          roll_off))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                band, ROLL_OFF);
		goto error;
	}

	// get the FMT groups
	if(!globalConfig.getListItems(band,
	                              FMT_GROUP_LIST,
	                              conf_list))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "Section %s, %s missing\n",
		                band, FMT_GROUP_LIST);
		goto error;
	}

	// create group list
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		unsigned int group_id;
		string fmt_id;
		FmtGroup *group;

		// Get group id name
		if(!globalConfig.getAttributeValue(iter, GROUP_ID, group_id))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in FMT "
			                "groups\n", band, GROUP_ID);
			goto error;
		}

		// Get FMT IDs
		if(!globalConfig.getAttributeValue(iter, FMT_ID, fmt_id))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in FMT "
							"groups\n", band, FMT_ID);
			goto error;
		}

		if(fmt_groups.find(group_id) != fmt_groups.end())
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, duplicated FMT group %u\n", band,
			                group_id);
			goto error;
		}
		group = new FmtGroup(group_id, fmt_id);
		fmt_groups[group_id] = group;
	}

	conf_list.clear();
	// get the carriers distribution
	if(!globalConfig.getListItems(band, CARRIERS_DISTRI_LIST, conf_list))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "Section %s, %s missing\n", band,
		                CARRIERS_DISTRI_LIST);
		goto error;
	}

	i = 0;
	carrier_id = 0;
	// create terminal categories according to channel distribution
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		string name;
		double ratio;
		rate_symps_t symbol_rate_symps;
		unsigned int group_id;
		string access_type;
		TerminalCategory *category;
		fmt_groups_t::const_iterator group_it;

		i++;

		// Get carriers' name
		if(!globalConfig.getAttributeValue(iter, CATEGORY, name))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in carriers "
			                "distribution table entry %u\n", band,
			                CATEGORY, i);
			goto error;
		}

		// Get carriers' ratio
		if(!globalConfig.getAttributeValue(iter, RATIO, ratio))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in carriers "
			                "distribution table entry %u\n", band, RATIO, i);
			goto error;
		}

		// Get carriers' symbol ratge
		if(!globalConfig.getAttributeValue(iter, SYMBOL_RATE, symbol_rate_symps))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in carriers "
			                "distribution table entry %u\n", band,
			                SYMBOL_RATE, i);
			goto error;
		}

		// Get carriers' FMT id
		if(!globalConfig.getAttributeValue(iter, FMT_GROUP, group_id))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in carriers "
			                "distribution table entry %u\n", band,
			                FMT_GROUP, i);
			goto error;
		}

		// Get carriers' access type
		if(!globalConfig.getAttributeValue(iter, ACCESS_TYPE, access_type))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in carriers "
			                "distribution table entry %u\n", band,
			                ACCESS_TYPE, i);
			goto error;
		}

		Output::sendLog(this->log_band, LEVEL_NOTICE,
		                "%s: new carriers: category=%s, Rs=%G, FMT group=%u, "
		                "ratio=%G, access type=%s\n", band, name.c_str(),
		                symbol_rate_symps, group_id, ratio,
		                access_type.c_str());
		if((!strcmp(band, UP_RETURN_BAND) && access_type != "DAMA") ||
		   (!strcmp(band, DOWN_FORWARD_BAND) && access_type != "TDM"))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "%s access type is not supported\n",
			                access_type.c_str());
			goto error;
		}

		group_it = fmt_groups.find(group_id);
		if(group_it == fmt_groups.end())
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, nentry for FMT group with ID %u\n",
			                band, group_id);
			goto error;
		}

		// create the category if it does not exist
		cat_iter = categories.find(name);
		category = (*cat_iter).second;
		if(cat_iter == categories.end())
		{
			category = new TerminalCategory(name);
			categories[name] = category;
		}
		category->addCarriersGroup(carrier_id, (*group_it).second, ratio,
		                           symbol_rate_symps);
		carrier_id++;
	}

	// get the default terminal category
	if(!globalConfig.getValue(band, DEFAULT_AFF,
	                          default_category_name))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "Section %s, missing %s parameter\n", band,
		                DEFAULT_AFF);
		goto error;
	}

	// Look for associated category
	*default_category = NULL;
	cat_iter = categories.find(default_category_name);
	if(cat_iter != categories.end())
	{
		*default_category = (*cat_iter).second;
	}
	if(*default_category == NULL)
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "Section %s, could not find categorie %s\n",
		                band, default_category_name.c_str());
		goto error;
	}
	Output::sendLog(this->log_band, LEVEL_NOTICE,
	                "ST default category: %s in %s\n",
	               (*default_category)->getLabel().c_str(), band);

	// get the terminal affectations
	if(!globalConfig.getListItems(band, TAL_AFF_LIST, aff_list))
	{
		Output::sendLog(this->log_band, LEVEL_NOTICE,
		                "Section %s, missing %s parameter\n", band,
		                TAL_AFF_LIST);
		goto error;
	}

	i = 0;
	for(ConfigurationList::iterator iter = aff_list.begin();
	    iter != aff_list.end(); ++iter)
	{
		// To prevent compilator to issue warning about non initialised variable
		tal_id_t tal_id = -1;
		string name;
		TerminalCategory *category;

		i++;
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in terminal "
			                "affection table entry %u\n", band, TAL_ID, i);
			goto error;
		}
		if(!globalConfig.getAttributeValue(iter, CATEGORY, name))
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, problem retrieving %s in terminal "
			                "affection table entry %u\n", band, CATEGORY, i);
			goto error;
		}

		// Look for the category
		category = NULL;
		cat_iter = categories.find(name);
		if(cat_iter != categories.end())
		{
			category = (*cat_iter).second;
		}
		if(category == NULL)
		{
			Output::sendLog(this->log_band, LEVEL_ERROR,
			                "Section %s, could not find category %s", band,
			                name.c_str());
			goto error;
		}

		terminal_affectation[tal_id] = category;
		Output::sendLog(this->log_band, LEVEL_INFO,
		                "%s: terminal %u will be affected to category %s\n",
		                band, tal_id, name.c_str());
	}

	// Compute bandplan
	if(!this->computeBandplan(bandwidth_khz, roll_off, duration_ms, categories))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "Cannot compute band plan for %s\n", band);
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvb::computeBandplan(freq_khz_t available_bandplan_khz,
                               double roll_off,
                               time_ms_t duration_ms,
                               TerminalCategories &categories)
{
	TerminalCategories::const_iterator category_it;

	double weighted_sum_ksymps = 0.0;

	// compute weighted sum
	for(category_it = categories.begin();
	    category_it != categories.end();
	    ++category_it)
	{
		TerminalCategory *category = (*category_it).second;

		// Compute weighted sum in ks/s since available bandplan is in kHz.
		weighted_sum_ksymps += category->getWeightedSum();
	}

	Output::sendLog(this->log_band, LEVEL_DEBUG,
	                "Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if(equals(weighted_sum_ksymps, 0.0))
	{
		Output::sendLog(this->log_band, LEVEL_ERROR,
		                "Weighted ratio sum is 0\n");
		goto error;
	}

	// compute carrier number per category
	for(category_it = categories.begin();
	    category_it != categories.end();
		category_it++)
	{
		unsigned int carriers_number = 0;
		TerminalCategory *category = (*category_it).second;
		unsigned int ratio = category->getRatio();

		carriers_number = ceil(
		    (ratio / weighted_sum_ksymps) *
		    (available_bandplan_khz / (1 + roll_off)));
		Output::sendLog(this->log_band, LEVEL_NOTICE,
		                "Number of carriers for category %s: %d\n",
		                category->getLabel().c_str(), carriers_number);

		// set the carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number,
		                               duration_ms);
	}

	return true;
error:
	return false;
}



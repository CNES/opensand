/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file BlockDvbSatRegen.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "BlockDvbSatRegen.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "GenericSwitch.h"
#include "SlottedAlohaFrame.h"
#include "OpenSandConf.h"

#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>

#include <opensand_output/Output.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>
#include <set>


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/

BlockDvbSatRegen::BlockDvbSatRegen(const string &name):
	BlockDvbSat(name)
{
}


// BlockDvbSatRegen dtor
BlockDvbSatRegen::~BlockDvbSatRegen()
{
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/
BlockDvbSatRegen::DownwardRegen::DownwardRegen(const string &name):
	Downward(name)
{
};



BlockDvbSatRegen::DownwardRegen::~DownwardRegen()
{
}

bool BlockDvbSatRegen::DownwardRegen::initSatLink(void)
{
	for(sat_gws_t::iterator it_gw = this->gws.begin();
	    it_gw != this->gws.end(); ++it_gw)
	{
		TerminalCategories<TerminalCategoryDama> st_categories;
		TerminalCategories<TerminalCategoryDama> gw_categories;
		ConfigurationList current_spot;
		ConfigurationList current_gw;
		ConfigurationList spot_list;
		SatGw *gw = it_gw->second;
		spot_id_t spot_id = gw->getSpotId();
		tal_id_t gw_id = gw->getGwId();
		FmtDefinitionTable *output_modcod_def = gw->getOutputModcodDef();

		if(!Conf::getListNode(Conf::section_map[FORWARD_DOWN_BAND],
			                  SPOT_LIST,
			                  spot_list))
		{
			LOG(this->log_init, LEVEL_ERROR, 
			    "section %s, missing %s", 
			    FORWARD_DOWN_BAND, SPOT_LIST);
		}

		if(!Conf::getElementWithAttributeValue(spot_list,
		                                       ID,
		                                       spot_id,
		                                       current_spot))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section %s,%s, missing %s",
			    FORWARD_DOWN_BAND, SPOT_LIST, ID);
		}
			
		if(!Conf::getElementWithAttributeValue(current_spot,
		                                       GW,
		                                       gw_id,
		                                       current_gw))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section %s,%s, missing %s",
			    FORWARD_DOWN_BAND, SPOT_LIST, ID);
		}
		// TODO no need of tal_aff and dflt_cat in attributes
		if(!this->initBand<TerminalCategoryDama>(current_spot,
		                                         FORWARD_DOWN_BAND,
		                                         TDM,
		                                         this->fwd_down_frame_duration_ms,
		                                         this->satellite_type,
		                                         output_modcod_def,
		                                         st_categories,
		                                         this->terminal_affectation,
		                                         &this->default_category,
		                                         this->fmt_groups))
		{
			return false;
		}

		// FIXME we init the same band for GW
		if(!this->initBand<TerminalCategoryDama>(current_spot,
		                                         FORWARD_DOWN_BAND,
		                                         TDM,
		                                         this->fwd_down_frame_duration_ms,
		                                         this->satellite_type,
		                                         output_modcod_def,
		                                         gw_categories,
		                                         this->terminal_affectation,
		                                         &this->default_category,
		                                         this->fmt_groups))
		{
			return false;
		}

		if(st_categories.size() != 1)
		{
			// TODO see NCC for that
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot support more than one category for "
			    "downlink band\n");
			return false;
		}

		TerminalCategoryDama *st_category = st_categories.begin()->second;
		TerminalCategoryDama *gw_category = gw_categories.begin()->second;

		// Finding the good fmt simulation
		if(!gw->initScheduling(this->fwd_down_frame_duration_ms,
		                          this->pkt_hdl,
		                          st_category,
		                          gw_category))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to init the spot scheduling\n");
			delete gw;
			TerminalCategories<TerminalCategoryDama>::iterator cat_it;
			for(cat_it = st_categories.begin();
			    cat_it != st_categories.end(); ++cat_it)
			{
				delete (*cat_it).second;
			}
			st_categories.clear();

			for(cat_it = gw_categories.begin();
			    cat_it != gw_categories.end(); ++cat_it)
			{
				delete (*cat_it).second;
			}
			gw_categories.clear();
			return false;
		}
	}
	return true;
}

bool BlockDvbSatRegen::DownwardRegen::initTimers(void)
{
	// create frame timer (also used to send packets waiting in fifo)
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                       this->fwd_down_frame_duration_ms);

	// TODO why not scenario timer on up ?
	sat_gws_t::iterator it_gw;
	for(it_gw = this->gws.begin(); it_gw != this->gws.end(); ++it_gw)
	{
		SatGw *gw = it_gw->second;
		// launch the timer in order to retrieve the modcods
		event_id_t scenario_timer = this->addTimerEvent("dvb_scenario_timer",
		                                                5000, // the duration will be change when started
		                                                false, // no rearm
		                                                false // do not start
		                                                );
		gw->initScenarioTimer(scenario_timer);
		this->raiseTimer(gw->getScenarioTimer());
	}

	return true;
}


bool BlockDvbSatRegen::DownwardRegen::handleMessageBurst(const RtEvent *const event)
{
	NetBurst *burst;
	NetBurst::iterator pkt_it;

	// message from upper layer: burst of encapsulation packets
	burst = (NetBurst *)((MessageEvent *)event)->getData();

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation burst received (%d packet(s))\n",
	    burst->length());

	// for each packet of the burst
	for(pkt_it = burst->begin(); pkt_it != burst->end();
	    pkt_it++)
	{
		if(!this->handleRcvEncapPacket(*pkt_it))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Rcv encap packet failed");
			burst->clear();
			delete burst;
			return false;
		}
	}
	// avoid deteleting packets when deleting burst
	burst->clear();
	delete burst;
	return true;
}

bool BlockDvbSatRegen::DownwardRegen::handleRcvEncapPacket(NetPacket *packet)
{
	sat_gws_t::iterator it_gw;
	spot_id_t spot_id;
	tal_id_t gw_id;
	tal_id_t tal_id;
	tal_id_t tal_id_src;
	DvbFifo *out_fifo = NULL;
	DvbFifo *out_fifo_gw = NULL;

	LOG(this->log_receive, LEVEL_INFO,
	    "store one encapsulation packet\n");

	spot_id = packet->getSpot();
	tal_id = packet->getDstTalId();
	tal_id_src = packet->getSrcTalId();

	// FIXME at the moment broadcast is sent on all spots
	if(tal_id == BROADCAST_TAL_ID)
	{
		// Send to all spot and all gw
		for(it_gw = this->gws.begin(); it_gw != this->gws.end(); ++it_gw)
		{
			SatGw *gw = it_gw->second;
			NetPacket *packet_copy = new NetPacket(packet);
			out_fifo = gw->getDataOutStFifo();
			if(!this->onRcvEncapPacket(packet_copy,
			                           out_fifo,
			                           0))
			{
				// FIXME a problem occured, we got memory allocation error
				// or fifo full and we won't empty fifo until next
				// call to onDownwardEvent => return
				LOG(this->log_receive, LEVEL_ERROR,
				    "unable to store packet\n");
				delete packet_copy;
				return false;
			}
			if(!OpenSandConf::isGw(tal_id_src))
			{
				out_fifo_gw = gw->getDataOutGwFifo();
				NetPacket *packet_copy_gw = new NetPacket(packet);
				if(!this->onRcvEncapPacket(packet_copy_gw,
				                           out_fifo_gw,
				                           0))
				{
					// FIXME a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					LOG(this->log_receive, LEVEL_ERROR,
					    "unable to store packet\n");
					delete packet_copy_gw;
					return false;
				}
			}
		}
		delete packet;
	}
	else
	{
		if(OpenSandConf::isGw(tal_id))
		{
			gw_id = tal_id;
		}
		else if(OpenSandConf::gw_table.find(tal_id) != OpenSandConf::gw_table.end())
		{
			gw_id = OpenSandConf::gw_table[tal_id];
		}
		else if(!Conf::getValue(Conf::section_map[GW_TABLE_SECTION],
		                        DEFAULT_GW, gw_id))
		{
			LOG(this->log_receive, LEVEL_ERROR, 
			    "couldn't find gw for tal %d", 
			    tal_id);
			return false;
		}

		SatGw *gw = this->gws[std::make_pair(spot_id, gw_id)];
		if(gw == NULL)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "coudn't find gw %u in spot %u\n",
			    gw_id, spot_id);
			return false;
		}
		if(packet->getDstTalId() == gw_id)
		{
			out_fifo = gw->getDataOutGwFifo();
		}
		else
		{
			out_fifo = gw->getDataOutStFifo();
		}

		if(!this->onRcvEncapPacket(packet,
		                           out_fifo,
		                           0))
		{
			// FIXME a problem occured, we got memory allocation error
			// or fifo full and we won't empty fifo until next
			// call to onDownwardEvent => return
			LOG(this->log_receive, LEVEL_ERROR,
			    "unable to store packet\n");
			return false;
		}
	}
	return true;
}

bool BlockDvbSatRegen::DownwardRegen::handleTimerEvent(SatGw *current_gw)
{
	if(!current_gw->schedule(this->down_frame_counter,
	                         (time_ms_t)getCurrentTime()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to schedule packets for satellite spot %u "
		    "on regenerative satellite\n", current_gw->getSpotId());
			return false;
	}

	// send ST bursts
	if(!this->sendBursts(&current_gw->getCompleteStDvbFrames(),
	                     current_gw->getDataOutStFifo()->getCarrierId()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to build and send DVB/BB frames toward ST"
		    "for satellite spot %u on regenerative satellite\n",
		    current_gw->getSpotId());
			return false;
	}

	// send GW bursts
	if(!this->sendBursts(&current_gw->getCompleteGwDvbFrames(),
	                    current_gw->getDataOutGwFifo()->getCarrierId()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to build and send DVB/BB frames toward GW"
		    "for satellite spot %u on regenerative satellite\n",
		    current_gw->getSpotId());
		return false;
	}

	return true;
}

bool BlockDvbSatRegen::DownwardRegen::handleScenarioTimer(SatGw *current_gw)
{
	LOG(this->log_receive, LEVEL_DEBUG,
	    "MODCOD scenario timer expired, update MODCOD table\n");
	
	double duration;
	event_id_t scenario_timer = current_gw->getScenarioTimer();

	if(!current_gw->goNextScenarioStepInput(duration))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to update MODCOD IDs\n");
		return false;
	}

	if(duration <= 0)
	{
		// we hare reach the end of the file (of it is malformed)
		// so we keep the modcod as they are
		this->removeEvent(scenario_timer);
	}
	else
	{
		this->setDuration(scenario_timer, duration);
		this->startTimer(scenario_timer);
	}

	return true;
}



/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/


BlockDvbSatRegen::UpwardRegen::UpwardRegen(const string &name):
	Upward(name)
{
};


BlockDvbSatRegen::UpwardRegen::~UpwardRegen()
{
}

bool BlockDvbSatRegen::UpwardRegen::onInit()
{

	if(!BlockDvbSat::Upward::onInit())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation\n");
		return false;
	}

	if(this->with_phy_layer)
	{
		string generate;
		time_ms_t acm_period_ms;

		// Check whether we generate the time series
		if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
		                   GENERATE_TIME_SERIES_PATH, generate))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, %s missing\n",
			    PHYSICAL_LAYER_SECTION, GENERATE_TIME_SERIES_PATH);
			return false;
		}
		if(generate != "none")
		{
			if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
			                   ACM_PERIOD_REFRESH,
			                   acm_period_ms))
			{
				LOG(this->log_init, LEVEL_ERROR,
				   "section '%s': missing parameter '%s'\n",
				   PHYSICAL_LAYER_SECTION, ACM_PERIOD_REFRESH);
				return false;
			}

			LOG(this->log_init, LEVEL_NOTICE,
			    "ACM period set to %d ms\n",
			    acm_period_ms);

			this->modcod_timer = this->addTimerEvent("generate_time_series",
			                                         acm_period_ms);
		}
	}

	// load the modcod files (regenerative satellite only)
	// initialize the satellite internal switch
	if(!this->initSwitchTable())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the switch part of the "
		    "initialisation\n");
		return false;
	}
	
	return true;
}

bool BlockDvbSatRegen::UpwardRegen::initSwitchTable(void)
{
	ConfigurationList spot_table;
	ConfigurationList::iterator iter;
	GenericSwitch *generic_switch = new GenericSwitch();
	spot_id_t spot_id;
	unsigned int i;

	// Retrieving switching table entries
	if(!Conf::getListNode(Conf::section_map[SPOT_TABLE_SECTION],
		                   SPOT_LIST, spot_table))
	{

		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing satellite spot "
		    "table\n", SPOT_TABLE_SECTION, SPOT_LIST);
		goto error;
	}


	i = 0;
	for(iter = spot_table.begin(); iter != spot_table.end(); iter++)
	{
		ConfigurationList tal_list;
		ConfigurationList current_spot;
		ConfigurationList::iterator tal_iter;
		current_spot.push_front(*iter);
		tal_id_t tal_id = 0;
		spot_id = 0;

		i++;
		// get the Spot ID attribute
		if(!Conf::getAttributeValue(iter, ID, spot_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in switching table"
			    "entry %u\n", ID, i);
			goto release_switch;
		}
	
		// Retrieving switching table entries
		if(!Conf::getListItems(current_spot, TERMINAL_LIST, tal_list))
		{

			LOG(this->log_init, LEVEL_ERROR,
					"section '%s, %s': missing satellite terminal id ",
					 SPOT_TABLE_SECTION, SPOT_LIST);
			goto error;
		}

		for(tal_iter = tal_list.begin() ; tal_iter != tal_list.end() ;
			++tal_iter)
		{	
			// get the Tal ID attribute
			if(!Conf::getAttributeValue(tal_iter, ID, tal_id))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "problem retrieving %s in spot table"
				    "entry %u\n", TAL_ID, i);
				goto release_switch;
			}

			if(!generic_switch->add(tal_id, spot_id))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to add switching entry "
				    "(Tal ID = %u, Spot ID = %u)\n",
				    tal_id, spot_id);
				goto release_switch;
			}

			LOG(this->log_init, LEVEL_NOTICE,
			    "Switching entry added (Tal ID = %u, "
			    "Spot ID = %u)\n", tal_id, spot_id);
		}
	}

	// get default spot id
	if(!Conf::getValue(Conf::section_map[SPOT_TABLE_SECTION],
		               DEFAULT_SPOT, spot_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SPOT_TABLE_SECTION, DEFAULT_SPOT);
		goto error;
	}
	generic_switch->setDefault(spot_id);

	if(!(dynamic_cast<DvbRcsStd *>(this->reception_std)->setSwitch(generic_switch)))
	{
		goto error;
	}

	return true;

release_switch:
	delete generic_switch;
error:
	return false;
}

bool BlockDvbSatRegen::UpwardRegen::addSt(SatGw *current_gw,
                                          tal_id_t st_id)
{
	if(!current_gw->addTerminal(st_id))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to register simulated ST with MAC "
		    "ID %u\n", st_id);
		return false;
	}

	return true;
}

bool BlockDvbSatRegen::UpwardRegen::handleCorrupted(DvbFrame *dvb_frame)
{
	LOG(this->log_receive, LEVEL_INFO,
	    "frame was corrupted by physical layer, drop it.");
	delete dvb_frame;
	return true;
}


bool BlockDvbSatRegen::UpwardRegen::handleDvbBurst(DvbFrame *dvb_frame,
                                                   SatGw *current_gw)
{
	NetBurst *burst = NULL;
	if(!current_gw->updateFmt(dvb_frame, this->pkt_hdl))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "gw %d failed to handle dvb burst\n",
		    current_gw->getGwId());
		return false;
	}
	
	if(!this->reception_std->onRcvFrame(dvb_frame,
	                                    0 /* no used */,
	                                    &burst))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle received DVB frame "
		    "(regenerative satellite)\n");
			burst = NULL;
		return false;
	}

	// send the message to the upper layer
	if(burst && !this->enqueueMessage((void **)&burst))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to upper layer\n");
		delete burst;
		return false;
	}
	LOG(this->log_receive, LEVEL_INFO,
	    "burst sent to the upper layer\n");

	return true;
}


bool BlockDvbSatRegen::UpwardRegen::handleSac(DvbFrame *dvb_frame,
                                              SatGw *current_gw)
{
	// Update fmt
	if(!current_gw->updateFmt(dvb_frame, this->pkt_hdl))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "gw %d failed to handle dvb burst\n",
		    current_gw->getGwId());
		return false;
	}
	if(!current_gw->handleSac(dvb_frame))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "gw %d failed to handle dvb burst\n",
		     current_gw->getGwId());
		return false;
	}

	return true;
}

bool BlockDvbSatRegen::UpwardRegen::handleBBFrame(DvbFrame UNUSED(*dvb_frame), 
                                                  SatGw UNUSED(*current_gw))
{
	assert(0);
}

bool BlockDvbSatRegen::UpwardRegen::handleSaloha(DvbFrame *UNUSED(dvb_frame),
                                                 SatGw *UNUSED(current_gw))
{
	assert(0);
}

bool BlockDvbSatRegen::UpwardRegen::updateSeriesGenerator(void)
{
	sat_gws_t::iterator it_gw;
	for(it_gw = this->gws.begin(); it_gw != this->gws.end(); ++it_gw)
	{
		SatGw *gw = it_gw->second;
		if(!gw->updateSeriesGenerator())
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Failed to update series generator\n");
			return false;
		}
	}
	return true;
}

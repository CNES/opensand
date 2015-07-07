/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>
#include <set>


typedef struct
{
	tal_id_t tal_id;
	double cni;
} cni_info_t;


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


bool BlockDvbSatRegen::onUpwardEvent(const RtEvent *const event)
{
	return ((UpwardRegen *)this->upward)->onEvent(event);
}


bool BlockDvbSatRegen::onDownwardEvent(const RtEvent *const event)
{
	return ((DownwardRegen *)this->downward)->onEvent(event);
}



/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/
BlockDvbSatRegen::DownwardRegen::DownwardRegen(Block *const bl):
	Downward(bl)
{
};



BlockDvbSatRegen::DownwardRegen::~DownwardRegen()
{
}

bool BlockDvbSatRegen::DownwardRegen::onInit()
{
	// get the common parameters
	// TODO no need to init pkt hdl in transparent mode,
	//      this will avoid loggers for encap to be instanciated
	if(!this->initCommon(FORWARD_DOWN_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		return false;
	}

	this->down_frame_counter = 0;

	set<tal_id_t> listGws = this->getGwIds();
	set<spot_id_t> listSpots = this->getSpotIds();
	for(set<spot_id_t>::iterator it1 = listSpots.begin();
	    it1 != listSpots.end(); it1++)
	{
		for(set<tal_id_t>::iterator it2 = listGws.begin();
		    it2 != listGws.end(); it2++)
		{
			FmtSimulation *fmt_simulation = new FmtSimulation();
			if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2,
			                          FORWARD_DOWN_MODCOD_TIME_SERIES,
			                          *fmt_simulation,
			                          (*it2), (*it1)))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to complete the modcod part of the "
				    "initialisation\n");
				return false;
			}
			this->setFmtSimulation((*it1), (*it2), fmt_simulation);
		}
	}

	// initialize the MODCOD scheme ID
	for(set<spot_id_t>::iterator it1 = listSpots.begin();
	    it1 != listSpots.end(); it1++)
	{
		for(set<tal_id_t>::iterator it2 = listGws.begin();
		    it2 != listGws.end(); it2++)
		{
			if(!this->goFirstScenarioStep((*it1), (*it2)))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to initialize downlink MODCOD IDs\n");
				return false;
			}
		}
	}

	if(!this->initStList())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the ST part of the"
		    "initialisation\n");
		return false;
	}

	if(!this->initSatLink())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of "
		    "link parameters\n");
		return false;
	}

	this->initStatsTimer(this->fwd_down_frame_duration_ms);

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize Output probes ans stats\n");
		return false;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize timers\n");
		return false;
	}

	return true;
}


bool BlockDvbSatRegen::DownwardRegen::initStList(void)
{
	int i = 0;
	ConfigurationList column_list;
	ConfigurationList::iterator iter;

	// Get the list of STs
	if(!Conf::getListItems(Conf::section_map[SAT_SIMU_COL_SECTION],
		                   COLUMN_LIST, column_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': problem retrieving simulation "
		    "column list\n", SAT_SIMU_COL_SECTION, COLUMN_LIST);
		goto error;
	}

	for(iter = column_list.begin(); iter != column_list.end(); iter++)
	{
		i++;
		tal_id_t tal_id = 0;
		long column_nbr;

		// Get the Tal ID
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", TAL_ID, i);
			goto error;
		}
		// Get the column nbr
		if(!Conf::getAttributeValue(iter, COLUMN_NBR, column_nbr))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", COLUMN_NBR, i);
			goto error;
		}

		// register a ST only if it did not exist yet
		// (duplicate because STs are 'defined' in spot table)
		// TODO Add terminal only for the right spot and gw
		sat_spots_t::iterator it;
		for(it = this->spots.begin();
		    it != this->spots.end(); it++)
		{
			list<SatGw *>::const_iterator it2;
			list<SatGw *> list = it->second->getGwList();
			for(it2 = list.begin();
			    it2 != list.end(); it2++)
			{
				if(!(*it2)->doTerminalExist(tal_id))
				{
					// TODO Do that on loggon
					if(!(*it2)->addTerminal(tal_id, column_nbr))
					{
						LOG(this->log_init, LEVEL_ERROR,
						    "failed to register ST with Tal ID %u\n",
						    tal_id);
						goto error;
					}
				}
			}
		}
	}

	return true;

error:
	return false;
}

bool BlockDvbSatRegen::DownwardRegen::initSatLink(void)
{
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               SAT_DELAY, this->sat_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, SAT_DELAY);
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "Satellite delay = %d\n", this->sat_delay);

	for(sat_spots_t::iterator i_spot = this->spots.begin();
	    i_spot != this->spots.end(); i_spot++)
	{
		// Init all gw by spot
		SatSpot *spot = (*i_spot).second;
		list<SatGw*> sat_gws = spot->getGwList();
		list<SatGw*>::iterator iter;
		for(iter = sat_gws.begin() ; iter != sat_gws.end() ; ++iter)
		{
			SatGw * gw = *iter;

			TerminalCategories<TerminalCategoryDama> st_categories;
			TerminalCategories<TerminalCategoryDama> gw_categories;
			ConfigurationList current_spot;
			ConfigurationList current_gw;
			ConfigurationList spot_list;
			SatSpot *spot;
			spot = i_spot->second;
			spot_id_t spot_id = spot->getSpotId();
			tal_id_t gw_id = gw->getGwId();

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
			                                         this->getModcodDefinitions(),
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
			                                         this->getModcodDefinitions(),
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
			FmtSimulation* fmt_simu_sat;
			fmt_simu_sat = gw->getFmtSimuSat();
			if(!gw->initScheduling(this->fwd_down_frame_duration_ms,
		                           this->pkt_hdl,
		                           fmt_simu_sat,
		                           st_category,
		                           gw_category))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to init the spot scheduling\n");
				delete spot;
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
	}
	return true;
}

bool BlockDvbSatRegen::DownwardRegen::initTimers(void)
{
	// create frame timer (also used to send packets waiting in fifo)
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                       this->fwd_down_frame_duration_ms);

	if(!this->with_phy_layer)
	{
		// launch the timer in order to retrieve the modcods
		this->scenario_timer = this->addTimerEvent("dvb_scenario_timer",
		                                           1, // the duration will be change when started
		                                           true, // no rearm
		                                           false // do not start
		                                           );
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



bool BlockDvbSatRegen::DownwardRegen::handleTimerEvent(SatGw *current_gw, 
                                                       uint8_t spot_id)
{
	if(!current_gw->schedule(this->down_frame_counter,
	                         (time_ms_t)getCurrentTime()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to schedule packets for satellite spot %u "
		    "on regenerative satellite\n", spot_id);
			return false;
	}

	// send ST bursts
	if(!this->sendBursts(&current_gw->getCompleteStDvbFrames(),
	                     current_gw->getDataOutStFifo()->getCarrierId()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to build and send DVB/BB frames toward ST"
		    "for satellite spot %u on regenerative satellite\n",
		    spot_id);
			return false;
	}

	// send GW bursts
	if(!this->sendBursts(&current_gw->getCompleteGwDvbFrames(),
	                    current_gw->getDataOutGwFifo()->getCarrierId()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to build and send DVB/BB frames toward GW"
		    "for satellite spot %u on regenerative satellite\n",
		    spot_id);
		return false;
	}

	return true;
}


/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/


BlockDvbSatRegen::UpwardRegen::UpwardRegen(Block *const bl):
	Upward(bl)
{
};


BlockDvbSatRegen::UpwardRegen::~UpwardRegen()
{
}

bool BlockDvbSatRegen::UpwardRegen::onInit()
{
	// get the common parameters
	// TODO no need to init pkt hdl in transparent mode,
	//      this will avoid loggers for encap to be instanciated
	if(!this->initCommon(RETURN_UP_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
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

bool BlockDvbSatRegen::UpwardRegen::handleDvbBurst(DvbFrame *dvb_frame,
                                                   SatGw UNUSED(*current_gw),
                                                   SatSpot UNUSED(*current_spot))
{
	NetBurst *burst = NULL;
	DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
	
	if(this->with_phy_layer  &&
	   this->reception_std->getType() == "DVB-RCS")
	{
		tal_id_t src_tal_id;
		// decode the first packet in frame to be able to get source terminal ID
		if(!this->pkt_hdl->getSrc(frame->getPayload(), src_tal_id))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "unable to read source terminal ID in "
			    "frame, won't be able to update C/N "
			    "value\n");
		}
		else
		{
			double cn = frame->getCn();
			LOG(this->log_receive, LEVEL_INFO,
			    "Uplink CNI for terminal %u = %f\n",
			    src_tal_id, cn);
			this->cni[src_tal_id] = cn;
		}
	}

	if(!this->reception_std->onRcvFrame(dvb_frame,
	                                    0 /* no used */,
	                                    &burst))
	{
		LOG(this->log_receive, LEVEL_ERROR,
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
                                              SatGw UNUSED(*current_gw))
{
	if(this->with_phy_layer)
	{
		// handle SAC here to get the uplink ACM parameters
		Sac *sac = (Sac *)dvb_frame;
		tal_id_t tal_id;
		cni_info_t *cni_info = new cni_info_t;
		tal_id = sac->getTerminalId();
		cni_info->cni = sac->getCni();
		cni_info->tal_id = tal_id;
		LOG(this->log_receive, LEVEL_INFO,
		    "Get SAC from ST%u, with C/N0 = %.2f\n",
		    tal_id, cni_info->cni);
		// transmit downlink CNI to downlink channel
		if(!this->shareMessage((void **)&cni_info, 
			                   sizeof(cni_info_t),
			                   msg_cni))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Unable to transmit downward CNI to "
			    "channel\n");
		}
		// update ACM parameters with uplink value, thus the GW will
		// known uplink C/N and thus update uplink MODCOD used in TTP
		if(this->cni.find(tal_id) != this->cni.end())
		{
			sac->setAcm(this->cni[tal_id]);
		}
		// TODO we won't update ACM parameters if we did not receive
		// traffic from this terminal, GW will have a wrong value...
		delete dvb_frame;
	}
	
	return true;
}


bool BlockDvbSatRegen::UpwardRegen::handleBBFrame(DvbFrame UNUSED(*dvb_frame), 
                                                  SatGw UNUSED(*current_gw),
                                                  SatSpot UNUSED(*current_spot))
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "Satellite Regenratif shoudln't handle BB Frame\n");
	
	return false;

}

bool BlockDvbSatRegen::UpwardRegen::handleSaloha(DvbFrame UNUSED(*dvb_frame), 
                                                 SatGw UNUSED(*current_gw),
                                                 SatSpot UNUSED(*current_spot))
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "Satellite Regenratif shoudln't handle Saloha\n");
	
	return false;

}	                       

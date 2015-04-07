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
 * @file BlockDvbSat.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 */

#include "BlockDvbSat.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "GenericSwitch.h"
#include "SlottedAlohaFrame.h"

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


typedef struct
{
	tal_id_t tal_id;
	double cni;
} cni_info_t;


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/

BlockDvbSat::BlockDvbSat(const string &name):
	BlockDvb(name),
	spots()
{
}


// BlockDvbSat dtor
BlockDvbSat::~BlockDvbSat()
{
	sat_spots_t::iterator i_spot;

	// delete the satellite spots
	for(i_spot = this->spots.begin(); i_spot != this->spots.end(); i_spot++)
	{
		delete i_spot->second;
	}
	
}


bool BlockDvbSat::onInit()
{
	// initialize the satellite spots
	if(!this->initSpots())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the spots part of the "
		    "initialisation\n");
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvbSat::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}


bool BlockDvbSat::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}


bool BlockDvbSat::initSpots(void)
{
	sat_spots_t::iterator it_s;
	list<SatGw *>::iterator it_g;
	list<SatGw *> list_g;
	
	int i = 0;
	size_t fifo_size;
	ConfigurationList spot_list;
	ConfigurationList::iterator carrier_iter;
	ConfigurationList::iterator spot_iter;

	// get satellite channels from configuration
	if(!Conf::getListNode(Conf::section_map[SATCAR_SECTION], 
		                  SPOT_LIST, 
		                  spot_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, SPOT_LIST);
		goto error;
	}

	for(spot_iter = spot_list.begin(); spot_iter != spot_list.end(); spot_iter++)
	{
		ConfigurationList carrier_list ; 
		
		spot_id_t spot_id = 0;
		tal_id_t gw_id = 0;
		uint8_t ctrl_id = 0;
		uint8_t data_in_carrier_id = 0;
		uint8_t data_out_gw_id = 0;
		uint8_t data_out_st_id = 0;
		uint8_t log_id = 0;
		SatSpot *new_spot;
		SatGw *new_gw;

		// Retrive FIFO size
		if(!Conf::getValue(Conf::section_map[ADV_SECTION],
			               DELAY_BUFFER, fifo_size))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s': missing parameter '%s'\n",
			    ADV_SECTION, DELAY_BUFFER);
			goto error;
		}

		i++;
		// get the spot_id
		if(!Conf::getAttributeValue(spot_iter, 
			                        ID, 
			                        spot_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SATCAR_SECTION, SPOT_LIST,
			    ID, i);
			goto error;
		}
		
		// get the gw_id
		if(!Conf::getAttributeValue(spot_iter, 
			                        GW, 
			                        gw_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SATCAR_SECTION, SPOT_LIST,
			    GW, i);
			goto error;
		}
		// get satellite channels from configuration
		if(!Conf::getListItems(*spot_iter, CARRIER_LIST, carrier_list))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s/%s%d, %s': missing satellite channels\n",
			    SATCAR_SECTION, SPOT_LIST, spot_id, CARRIER_LIST);
			goto error;
		}

		// check id du spot correspond au id du spot dans lequel est le bloc actuel!
		for(carrier_iter = carrier_list.begin(); carrier_iter != carrier_list.end(); 
		    carrier_iter++)
		{
			unsigned int carrier_id;
			string carrier_type;
			
			// Get the carrier id
			if(!Conf::getAttributeValue(carrier_iter,
				                        CARRIER_ID,
				                        carrier_id))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s/%s%d/%s': missing parameter '%s'\n",
				    SATCAR_SECTION, SPOT_LIST, spot_id, 
				    CARRIER_LIST, CARRIER_ID);
				goto error;
			}

			// Get the carrier type
			if(!Conf::getAttributeValue(carrier_iter,
				                        CARRIER_TYPE,
				                        carrier_type))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s/%s%d/%s': missing parameter '%s'\n",
				    SATCAR_SECTION, SPOT_LIST, spot_id, 
				    CARRIER_LIST, CARRIER_TYPE);
				goto error;
			}

			// Get the ID for control carrier
			if(strcmp(carrier_type.c_str(), CTRL_OUT) == 0)
			{
				ctrl_id = carrier_id;
			}
			// Get the ID for data carrier
			else if(strcmp(carrier_type.c_str(), DATA_IN_ST) == 0)
			{
				data_in_carrier_id = carrier_id;
			}
			// Get the ID for data out gw carrier
			else if(strcmp(carrier_type.c_str(), DATA_OUT_GW) == 0)
			{
				data_out_gw_id = carrier_id;
			}
			// Get the ID for data out st carrier
			else if(strcmp(carrier_type.c_str(), DATA_OUT_ST) == 0)
			{
				data_out_st_id = carrier_id;
			}
			// Get the ID for logon out carrier
			else if(strcmp(carrier_type.c_str(), LOGON_OUT) == 0)
			{
				log_id = carrier_id;
			}
		}
	
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#: carrier IDs for Ctrl = %u, data_in = %u, "
		    "data out gw = %u, data out st = %u, log id = %u\n", 
		    ctrl_id, data_in_carrier_id, data_out_gw_id,
		    data_out_st_id, log_id);
		//***************************
		// create a new gw
		//***************************
		new_gw = new SatGw(gw_id, spot_id,
		                   data_in_carrier_id,
		                   log_id,
		                   ctrl_id,
		                   data_out_st_id,
		                   data_out_gw_id,
		                   fifo_size);

		if(this->spots[spot_id] == NULL)
		{
			//***************************
			// create a new spot
			//***************************
			new_spot = new SatSpot(spot_id);
			if(new_spot == NULL)
			{
				LOG(this->log_init, LEVEL_ERROR,
						"failed to create a new satellite spot\n");
				goto error;
			}
			
			new_spot->addGw(new_gw);

			LOG(this->log_init, LEVEL_NOTICE,
					"satellite spot %u: logon = %u, control = %u, "
					"data out ST = %u, data out GW = %u\n",
					spot_id, log_id, ctrl_id, data_out_st_id,
					data_out_gw_id);
			// store the new satellite spot in the list of spots
			this->spots[spot_id] = new_spot;
		}
		else
		{
			this->spots[spot_id]->addGw(new_gw);
		}
	}


	((Upward *)this->upward)->setSpots(this->spots);
	((Downward *)this->downward)->setSpots(this->spots);


	return true;

error:
	return false;
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/
BlockDvbSat::Downward::Downward(Block *const bl):
	DvbDownward(bl),
	down_frame_counter(),
	sat_delay(),
	fwd_timer(-1),
	scenario_timer(-1),
	terminal_affectation(),
	default_category(),
	fmt_groups(),
	spots(),
	probe_frame_interval(NULL)
{
};



BlockDvbSat::Downward::~Downward()
{

	// delete FMT groups here because they may be present in many carriers
	for(fmt_groups_t::iterator it = this->fmt_groups.begin();
	    it != this->fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	this->terminal_affectation.clear();
}


void BlockDvbSat::Downward::setSpots(const sat_spots_t &spots)
{
	this->spots = spots;
}

bool BlockDvbSat::Downward::onInit()
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

	// load the modcod files (regenerative satellite only)
	if(this->satellite_type == REGENERATIVE)
	{
		if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2,
		                          FORWARD_DOWN_MODCOD_TIME_SERIES))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the modcod part of the "
			    "initialisation\n");
			return false;
		}
		// initialize the MODCOD scheme ID
		if(!this->fmt_simu.goNextScenarioStep(true))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to initialize downlink MODCOD IDs\n");
			return false;
		}

		if(!this->initStList())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the ST part of the"
			    "initialisation\n");
			return false;
		}
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


bool BlockDvbSat::Downward::initSatLink(void)
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

	if(this->satellite_type == REGENERATIVE)
	{
		// TODO check for multispot, loop should also be on initBand
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

				TerminalCategories<TerminalCategoryDama> categories;
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
				                                         this->fmt_simu.getModcodDefinitions(),
				                                         categories,
				                                         this->terminal_affectation,
				                                         &this->default_category,
				                                         this->fmt_groups))
				{
					return false;
				}
				

				if(categories.size() != 1)
				{
					// TODO see NCC for that, we may handle categories in
					//      spots here.
					LOG(this->log_init, LEVEL_ERROR,
							"cannot support more than one category for "
							"downlink band\n");
					return false;
				}

				TerminalCategoryDama *category = categories.begin()->second;

				// TODO delete category dans le spot
				if(!gw->initScheduling(this->fwd_down_frame_duration_ms,
			                           this->pkt_hdl,
			                           &this->fmt_simu,
			                           category))
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "failed to init the spot scheduling\n");
					delete spot;
					delete gw;
					TerminalCategories<TerminalCategoryDama>::iterator cat_it;
					for(cat_it = categories.begin();
							cat_it != categories.end(); ++cat_it)
					{
						delete (*cat_it).second;
					}
					categories.clear();
					return false;
				}
			}
		}
	}
	return true;
}


bool BlockDvbSat::Downward::initTimers(void)
{
	// create frame timer (also used to send packets waiting in fifo)
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                       this->fwd_down_frame_duration_ms);

	if(this->satellite_type == REGENERATIVE && !this->with_phy_layer)
	{
		// launch the timer in order to retrieve the modcods
		this->scenario_timer = this->addTimerEvent("dvb_scenario_timer",
		                                           this->dvb_scenario_refresh);
	}

	return true;
}


bool BlockDvbSat::Downward::initStList(void)
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
		if(!this->fmt_simu.doTerminalExist(tal_id))
		{
			if(!this->fmt_simu.addTerminal(tal_id, column_nbr))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to register ST with Tal ID %u\n",
				    tal_id);
				goto error;
			}
		}
	}

	return true;

error:
	return false;
}

bool BlockDvbSat::Downward::initOutput(void)
{
	// Output probes and stats
	sat_spots_t::iterator spot_it;
	for(spot_it = this->spots.begin(); spot_it != spots.end(); ++spot_it)
	{
		SatSpot* spot = spot_it->second;
		list<SatGw *> list_gw = spot->getGwList();
		list<SatGw *>::iterator gw_it;

		for(gw_it = list_gw.begin() ; gw_it != list_gw.end() ;
		    ++ gw_it)
		{
			SatGw *gw = *gw_it;
			gw->initProbes(this->satellite_type);
		}
	}

	this->probe_frame_interval = Output::registerProbe<float>(
		"Perf.Frames_interval", "ms", true, SAMPLE_LAST);

	return true;
}


bool BlockDvbSat::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			if(((MessageEvent *)event)->getMessageType() == msg_cni)
			{
				cni_info_t *info = (cni_info_t *)((MessageEvent *)event)->getData();
				this->fmt_simu.setRequiredModcod(info->tal_id,
				                                 info->cni);
				delete info;
				break;
			}
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				bool status = true;
				DvbFrame *dvb_frame;
				dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
				sat_spots_t::iterator spot;
				spot_id_t spot_id ;
				unsigned int carrier_id = dvb_frame->getCarrierId();
				tal_id_t gw_id = 0;
				
				if(!Conf::getSpotWithCarrierId(carrier_id, spot_id, gw_id))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find spot with carrier ID %u in spot "
					    "list\n", carrier_id);
					break;
				}

				if(spot_id != dvb_frame->getSpot())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Frame: wrong carrier id (%u) or spot id (%u)\n",
					    carrier_id, dvb_frame->getSpot());
					break;
				}

				spot = this->spots.find(spot_id);
				if(spot == this->spots.end())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find spot with ID %u in spot "
					    "list\n", spot_id);
					break;
				}
				SatSpot *current_spot = spots[spot_id];
				SatGw *current_gw = current_spot->getGw(gw_id);

				if(current_gw == NULL)
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "Spot %u does'nt have gw %u\n", spot_id, gw_id);
					break;
				}

				// send frame for every satellite spot
				if(dvb_frame->getMessageType() != MSG_TYPE_SOF)
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "Forwarded frame is not a SoF\n");
					status = false;
					break;
				}

				// create a message for the DVB frame
				if(!this->sendDvbFrame(dvb_frame,
				     current_gw->getControlCarrierId()))
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "failed to send sig frame to lower layer, "
					    "drop it\n");
					status = false;
				}
				return status;
			}

			if(this->satellite_type != REGENERATIVE)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "message event while satellite is "
				    "transparent");
				return false;
			}

			NetBurst *burst;
			spot_id_t spot_id;
			tal_id_t gw_id;
			tal_id_t tal_id;
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
				sat_spots_t::iterator iter;
				tal_id_t tab[NB_GW] = {GW_TAL_ID};
				list<tal_id_t> gw_tal_id (tab, tab + sizeof(tab) / sizeof(tal_id_t) );
				list<tal_id_t>::iterator it_tal_id;
				
				LOG(this->log_receive, LEVEL_INFO,
				    "store one encapsulation packet\n");
				
				spot_id = (*pkt_it)->getSpot();
				tal_id = (*pkt_it)->getDstTalId();
				
				it_tal_id = find(gw_tal_id.begin(), gw_tal_id.end(), tal_id);
				
				if(it_tal_id != gw_tal_id.end())
				{
					gw_id = (*it_tal_id);
				}
				else if(Conf::gw_table.find(tal_id) != Conf::gw_table.end())
				{
					gw_id = Conf::gw_table[tal_id];
				}
				else if(!Conf::getValue(Conf::section_map[GW_TABLE_SECTION],
					                    DEFAULT_GW, gw_id))
				{
					LOG(this->log_receive, LEVEL_ERROR, 
							"couldn't find gw for tal %d", 
							tal_id);
					return false;
				}

				iter = this->spots.find(spot_id);
				if(iter == this->spots.end())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find spot with ID %u in spot "
					    "list\n", spot_id);
					break;
				}
				
				SatGw *gw = this->spots[spot_id]->getGw(gw_id);

				if(gw == NULL)
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "coudn't find gw %u in spot %u\n",
					    gw_id, spot_id);
					burst->clear();
					delete burst;
					return false;
				}
				
				if(!this->onRcvEncapPacket(*pkt_it,
					                       gw->getDataOutStFifo(),
					                       this->sat_delay))
				{
					// FIXME a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					LOG(this->log_receive, LEVEL_ERROR,
					    "unable to store packet\n");
					burst->clear();
					delete burst;
					return false;
				}
			}

			// avoid deteleting packets when deleting burst
			burst->clear();

			delete burst;
		}
		break;

		case evt_timer:
		{
			if(*event == this->fwd_timer)
			{
				this->updateStats();
				// Update stats and probes
				if(this->probe_frame_interval->isEnabled())
				{
					timeval interval = event->getAndSetCustomTime();
					float val = interval.tv_sec * 1000000L + interval.tv_usec;
					this->probe_frame_interval->put(val / 1000);
				}

				// increment counter of superframes
				this->down_frame_counter++;
				LOG(this->log_receive, LEVEL_DEBUG,
				    "frame timer expired, send DVB frames\n");

				// send frame for every satellite spot
				for(sat_spots_t::iterator i_spot = this->spots.begin();
				    i_spot != this->spots.end(); i_spot++)
				{

					SatSpot *current_spot = i_spot->second;
					list<SatGw *> listGw = current_spot->getGwList();
					list<SatGw *>::iterator gw_iter;

					for(gw_iter = listGw.begin() ; gw_iter != listGw.end() ;
					    ++gw_iter)
					{
						SatGw *current_gw = *gw_iter;
						LOG(this->log_send, LEVEL_DEBUG,
								"send logon frames on satellite spot %u\n",
								i_spot->first);
						if(!this->sendFrames(current_gw->getLogonFifo()))
						{
							LOG(this->log_send, LEVEL_ERROR,
									"Failed to send logon frames on spot %u\n",
									i_spot->first);
						}

						LOG(this->log_send, LEVEL_DEBUG,
								"send control frames on satellite spot %u\n",
								i_spot->first);
						if(!this->sendFrames(current_gw->getControlFifo()))
						{
							LOG(this->log_send, LEVEL_ERROR,
									"Failed to send contol frames on spot %u\n",
									i_spot->first);
						}

						if(this->satellite_type == TRANSPARENT)
						{
							// send frame for every satellite spot
							bool status = true;

							LOG(this->log_receive, LEVEL_DEBUG,
									"send data frames on satellite spot "
									"%u\n", i_spot->first);
							if(!this->sendFrames(current_gw->getDataOutGwFifo()))
							{
								status = false;
							}
							if(!this->sendFrames(current_gw->getDataOutStFifo()))
							{
								status = false;
							}
							if(!status)
							{
								return false;
							}
						}
						else
						{
							if(!current_gw->schedule(this->down_frame_counter,
										(time_ms_t)getCurrentTime()))
							{
								LOG(this->log_receive, LEVEL_ERROR,
										"failed to schedule packets for satellite spot %u "
										"on regenerative satellite\n", i_spot->first);
								return false;
							}

							if(!this->sendBursts(&current_gw->getCompleteDvbFrames(),
										current_gw->getDataOutStFifo()->getCarrierId()))
							{
								LOG(this->log_receive, LEVEL_ERROR,
										"failed to build and send DVB/BB frames "
										"for satellite spot %u on regenerative satellite\n",
										i_spot->first);
								return false;
							}
						}
					}
				}
			}
			else if(*event == this->scenario_timer)
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "MODCOD scenario timer expired\n");

				LOG(this->log_receive, LEVEL_DEBUG,
				    "update modcod table\n");
				if(!this->fmt_simu.goNextScenarioStep(true))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to update MODCOD IDs\n");
					return false;
				}
			}
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unknown timer event received %s\n",
				    event->getName().c_str());
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event: %s\n", event->getName().c_str());
	}

	return true;
}


bool BlockDvbSat::Downward::sendFrames(DvbFifo *fifo)
{
	MacFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	while(fifo->getTickOut() <= current_time &&
	      fifo->getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;
		size_t length;

		elem = fifo->pop();
		assert(elem != NULL);

		dvb_frame = elem->getElem<DvbFrame>();
		length = dvb_frame->getTotalLength();

		// create a message for the DVB frame
		if(!this->sendDvbFrame(dvb_frame, fifo->getCarrierId()))
		{
			LOG(this->log_send, LEVEL_ERROR,
			    "failed to send message, drop the DVB or BB "
			    "frame\n");
			goto error;
		}

		LOG(this->log_send, LEVEL_INFO,
		    "Frame sent with a size of %zu\n", length);

		delete elem;
	}

	return true;

error:
	delete elem;
	return false;
}

void BlockDvbSat::Downward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}
	// Update stats and probes
	sat_spots_t::iterator spot_it;
	for (spot_it = this->spots.begin(); spot_it != spots.end(); ++spot_it)
	{
		SatSpot* spot = (*spot_it).second;

		list<SatGw *> listGw = spot->getGwList();
		list<SatGw *>::iterator gw_iter;

		for(gw_iter = listGw.begin() ; gw_iter != listGw.end() ;
				++gw_iter)
		{
			SatGw *current_gw = *gw_iter;
			current_gw->updateProbes(this->satellite_type, this->stats_period_ms);	
		}
	}

	// Send probes
	Output::sendProbes();
}



/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/


BlockDvbSat::Upward::Upward(Block *const bl):
	DvbUpward(bl),
	reception_std(NULL),	
	spots(),
	cni(),
	sat_delay()
{
};


BlockDvbSat::Upward::~Upward()
{
	// release the reception DVB standards
	if(this->reception_std != NULL)
	{
	   delete this->reception_std;
	}
}


void BlockDvbSat::Upward::setSpots(const sat_spots_t &spots)
{
	this->spots = spots;
}


bool BlockDvbSat::Upward::onInit()
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
	if(this->satellite_type == REGENERATIVE)
	{
		// initialize the satellite internal switch
		if(!this->initSwitchTable())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the switch part of the "
			    "initialisation\n");
			return false;
		}
	}

	return true;
}


bool BlockDvbSat::Upward::initMode(void)
{
	// Delay to apply to the medium
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION], 
		               SAT_DELAY, this->sat_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, SAT_DELAY);
		goto error;
	}
	    
	LOG(this->log_init, LEVEL_NOTICE,
	     "Satellite delay = %d", this->sat_delay);

	if(this->satellite_type == REGENERATIVE)
	{
		this->reception_std = new DvbRcsStd(this->pkt_hdl);
	}
	else
	{
		// create the reception standard
		this->reception_std = new DvbRcsStd(); 
	}
	if(this->reception_std == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the reception standard\n");
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvbSat::Upward::initSwitchTable(void)
{
	ConfigurationList spot_table;
	ConfigurationList::iterator iter;
	GenericSwitch *generic_switch = new GenericSwitch();
	spot_id_t spot_id;
	unsigned int i;

	// no need for switch in non-regenerative mode
	if(this->satellite_type != REGENERATIVE)
	{
		return true;
	}

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


bool BlockDvbSat::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message from lower layer: dvb frame
			DvbFrame *dvb_frame;
			dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
		
			if(!this->onRcvDvbFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle received DVB frame\n");
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event: %s", event->getName().c_str());
			return false;
	}
	return true;
}



// About multithreaded channels implementation:
// We choose to let the transparent treatment and push in FIFO in Upward
// while we could have only transmitted the frame to the Downward channel
// that would have analysed it but with this solution brings better task
// sharing between channels
// The fifo is protected with a mutex
// The spots also protected for some shared elements
bool BlockDvbSat::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	bool status = true;
	sat_spots_t::iterator spot;
	spot_id_t spot_id;
	tal_id_t gw_id;
	unsigned int carrier_id = dvb_frame->getCarrierId();

	// get the satellite spot from which the DVB frame comes from
	// TODO with spot id, not loop and carrier id
	//      check if input carrier id is still usefull
	if(!Conf::getSpotWithCarrierId(carrier_id, spot_id, gw_id))
	{
		LOG(this->log_receive, LEVEL_ERROR,
				"cannot find gw id for carrier %u\n", carrier_id);
		return false;
	}

	SatSpot *current_spot = spots[spot_id];
	SatGw *current_gw = current_spot->getGw(gw_id);

	if(current_gw == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
				"cannot find gw id in spot %u\n", spot_id);
		return false;
	}

	spot = this->spots.find(spot_id);
	if(spot == this->spots.end())
	{
		LOG(this->log_receive, LEVEL_ERROR,
				"cannot find spot with ID %u in spot "
				"list\n", spot_id);
		return false;
	}
	
	LOG(this->log_receive, LEVEL_DEBUG,
	    "DVB frame received from lower layer (type = %d, len %zu)\n",
	    dvb_frame->getMessageType(),
	    dvb_frame->getTotalLength());


	switch(dvb_frame->getMessageType())
	{
		case MSG_TYPE_CORRUPTED:
			if(this->satellite_type == TRANSPARENT)
			{
				// in transparent scenario, satellite physical layer cannot corrupt
				LOG(this->log_receive, LEVEL_INFO,
				    "the message was corrupted by physical layer, "
				    "drop it\n");
				delete dvb_frame;
				break;
			}
			// continue to handle the corrupted message in onRcvFrame
		case MSG_TYPE_DVB_BURST:
		{
			/* the DVB frame contains a burst of packets:
			 *  - if the satellite is a regenerative one, forward the burst to the
			 *    encapsulation layer,
			 *  - if the satellite is a transparent one, forward DVB burst as the
			 *    other DVB frames.
			 */

			LOG(this->log_receive, LEVEL_INFO,
			    "DVB-Frame received\n");
			
			// satellite spot found, forward DVB frame on the same spot
			DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();

		 	if(this->satellite_type == TRANSPARENT)
			{
				// Update probes and stats
				current_gw->updateL2FromSt(frame->getPayloadLength());

				// TODO: forward according to a table
				LOG(this->log_receive, LEVEL_INFO,
				    "DVB burst comes from spot %u (carrier "
				    "%u) => forward it to spot %u (carrier "
				    "%u)\n", current_spot->getSpotId(),
				    dvb_frame->getCarrierId(),
				    current_spot->getSpotId(),
				    current_gw->getDataOutGwFifo()->
				    getCarrierId());

				if(!this->forwardDvbFrame(current_gw->getDataOutGwFifo(),
				                          dvb_frame))
				{
					status = false;
				}
			}
			else // else satellite_type == REGENERATIVE
			{
				/* The satellite is a regenerative one and the DVB frame contains
				 * a burst:
				 *  - extract the packets from the DVB frame,
				 *  - find the destination spot ID for each packet
				 *  - create a burst of encapsulation packets (NetBurst object)
				 *    with all the packets extracted from the DVB frame,
				 *  - send the burst to the upper layer.
				 */

				NetBurst *burst = NULL;

				// Update probes and stats
				// get the satellite spot from which the DVB frame comes from
				current_gw->updateL2FromSt(frame->getPayloadLength());

				if(this->with_phy_layer && this->satellite_type == REGENERATIVE &&
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
							0 /* no used */, &burst))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to handle received DVB frame "
					    "(regenerative satellite)\n");
					status = false;
					burst = NULL;
				}

				// send the message to the upper layer
				if(burst && !this->enqueueMessage((void **)&burst))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to send burst to upper layer\n");
					delete burst;
					status = false;
				}
				LOG(this->log_receive, LEVEL_INFO,
				    "burst sent to the upper layer\n");
			}
		}
		break;

			/* forward the BB frame (and the burst that the frame contains) */
			// TODO see if we can factorize
		case MSG_TYPE_BBFRAME:
		{
			/* we should not receive BB frame in regenerative mode */
			assert(this->satellite_type == TRANSPARENT);

			LOG(this->log_receive, LEVEL_INFO,
			    "BBFrame received\n");

			// satellite spot found, forward BBframe on the same spot
			BBFrame *bbframe = dvb_frame->operator BBFrame*();

			// Update probes and stats
			current_gw->updateL2FromGw(bbframe->getPayloadLength());

			// TODO: forward according to a table
			LOG(this->log_receive, LEVEL_INFO,
			    "BBFRAME burst comes from spot %u (carrier "
			    "%u) => forward it to spot %u (carrier %u)\n",
			    current_spot->getSpotId(),
			    dvb_frame->getCarrierId(),
			    current_spot->getSpotId(),
			    current_gw->getDataOutStFifo()->
			    getCarrierId());

			if(!this->forwardDvbFrame(current_gw->getDataOutStFifo(),
			                          dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "cannot forward burst\n");
				status = false;
			}
		}
		break;

		case MSG_TYPE_SALOHA_DATA:
		case MSG_TYPE_SALOHA_CTRL:
		{
			/* we should not receive BB frame in regenerative mode */
			assert(this->satellite_type == TRANSPARENT);

			LOG(this->log_receive, LEVEL_INFO,
			    "Slotted Aloha frame received\n");

			DvbFifo *fifo;
			
			// satellite spot found, forward frame on the same spot
			SlottedAlohaFrame *sa_frame = dvb_frame->operator SlottedAlohaFrame*();

			// Update probes and stats
			current_gw->updateL2FromSt(sa_frame->getPayloadLength());

			if(dvb_frame->getMessageType() == MSG_TYPE_SALOHA_DATA)
			{
				fifo = current_gw->getDataOutGwFifo();
			}
			else
			{
				fifo = current_gw->getDataOutStFifo();
			}

			// TODO: forward according to a table
			LOG(this->log_receive, LEVEL_INFO,
			    "Slotted Aloha frame comes from spot %u (carrier "
			    "%u) => forward it to spot %u (carrier %u)\n",
			    current_spot->getSpotId(),
			    dvb_frame->getCarrierId(),
			    current_spot->getSpotId(),
			    fifo->getCarrierId());

			if(!this->forwardDvbFrame(fifo,
			                          dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "cannot forward burst\n");
				status = false;
			}

		}
		break;

		// Generic control frames (SAC, TTP, etc)
		case MSG_TYPE_SAC:
			if(this->with_phy_layer && this->satellite_type == REGENERATIVE)
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
			}
			// do not break here !
		case MSG_TYPE_TTP:
		case MSG_TYPE_SYNC:
		case MSG_TYPE_SESSION_LOGON_RESP:
			{
				// forward the frame copy
				if(!this->forwardDvbFrame(current_gw->getControlFifo(),
				                          dvb_frame))
				{
					status = false;
				}
				//delete dvb_frame;
			}
			break;

			// Special case of logon frame with dedicated channel
		case MSG_TYPE_SESSION_LOGON_REQ:
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "ST logon request received, "
				    "forward it on all satellite spots\n");

				// forward the frame copy
				if(!this->forwardDvbFrame(current_gw->getLogonFifo(),
				                          dvb_frame))
				{
					status = false;
				}
				//delete dvb_frame;
			}
			break;


		case MSG_TYPE_SOF:
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "control frame (type = %u) received, "
				    "forward it on all satellite spots\n",
				    dvb_frame->getMessageType());
				// the SOF message should not be stored in fifo, because it
				// would be kept a random amount of time between [0, fwd_timer]
				// and we need a perfect synchronization
				if(!this->shareMessage((void **)&dvb_frame, 
					                   sizeof(dvb_frame),
					                   msg_sig))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit sig to downward channel\n");
				}
			}
			break;

		default:
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unknown type (%u) of DVB frame\n",
				    dvb_frame->getMessageType());
				delete dvb_frame;
			}
			break;
	}

	return status;
}

bool BlockDvbSat::Upward::forwardDvbFrame(DvbFifo *fifo, DvbFrame *dvb_frame)
{
    return this->pushInFifo(fifo, (NetContainer *)dvb_frame, this->sat_delay);
}



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
		uint8_t data_in_gw_id = 0;
		uint8_t data_in_st_id = 0;
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
			// Get the ID for data in gw carrier
			else if(strcmp(carrier_type.c_str(), DATA_IN_GW) == 0)
			{
				data_in_gw_id = carrier_id;
			}
			// Get the ID for data in st carrier
			else if(strcmp(carrier_type.c_str(), DATA_IN_ST) == 0)
			{
				data_in_st_id = carrier_id;
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
		    "SF#: carrier IDs for Ctrl = %u, "
		    "data in gw = %u, data in st = %u, "
		    "data out gw = %u, data out st = %u, "
		    "log id = %u\n", 
		    ctrl_id, data_in_gw_id, data_in_st_id,
		    data_out_gw_id, data_out_st_id, log_id);
		//***************************
		// create a new gw
		//***************************
		new_gw = new SatGw(gw_id, spot_id,
		                   log_id,
		                   ctrl_id,
		                   data_in_st_id,
		                   data_in_gw_id,
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
	cni(),
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
			gw->initProbes();
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
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				bool status = true;
				DvbFrame *dvb_frame;
				dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
				sat_spots_t::iterator spot;
				spot_id_t spot_id ;
				unsigned int carrier_id = dvb_frame->getCarrierId();
				tal_id_t gw_id = 0;
				
				if(!OpenSandConf::getSpotWithCarrierId(carrier_id, spot_id, gw_id))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find spot with carrier ID %u in spot "
					    "list\n", carrier_id);
					delete dvb_frame;
					break;
				}

				if(spot_id != dvb_frame->getSpot())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Frame: wrong carrier id (%u) or spot id (%u)\n",
					    carrier_id, dvb_frame->getSpot());
					delete dvb_frame;
					break;
				}

				spot = this->spots.find(spot_id);
				if(spot == this->spots.end())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find spot with ID %u in spot "
					    "list\n", spot_id);
					delete dvb_frame;
					break;
				}
				SatSpot *current_spot = spots[spot_id];
				SatGw *current_gw = current_spot->getGw(gw_id);

				if(current_gw == NULL)
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "Spot %u does'nt have gw %u\n", spot_id, gw_id);
					delete dvb_frame;
					break;
				}

				if(dvb_frame->getMessageType() != MSG_TYPE_SOF)
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "Forwarded frame is not a SoF\n");
					status = false;
					delete dvb_frame;
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
					delete dvb_frame;
				}
				return status;
			}

			if(!handleMessageBurst(event))
			{
				return false;
			}
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

						if(!this->handleTimerEvent(current_gw, 
						                           i_spot->first))
						{
							return false;
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
				double duration;
				//TODO Timer per spot and per gw
				if(!this->goNextScenarioStepInput(duration, 0, 0))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to update MODCOD IDs\n");
					return false;
				}
				if(duration <= 0)
				{
					// we hare reach the end of the file (of it is malformed)
					// so we keep the modcod as they are
					this->removeEvent(this->scenario_timer);
				}
				else
				{
					this->setDuration(this->scenario_timer, duration);
					this->startTimer(this->scenario_timer);
				}
				// Update the cni for all terminals
				map<tal_id_t, double>::iterator it;
				for(it = this->cni.begin(); it != this->cni.end(); it++)
				{
					uint8_t current_modcod = this->getCurrentModcodIdInput(it->first);
					this->cni[it->first] = this->input_modcod_def.getRequiredEsN0(current_modcod);
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


bool BlockDvbSat::Downward::handleRcvEncapPacket(NetPacket *packet)
{
	map<tal_id_t, spot_id_t>::iterator tal_iter;
	sat_spots_t::iterator spot;
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

	if(tal_id == BROADCAST_TAL_ID)
	{
		// Send to all spot and all gw
		for(spot = this->spots.begin(); spot != this->spots.end(); ++spot)
		{
			list<SatGw *> gws = (*spot).second->getListGw();
			list<SatGw *>::iterator gw;
			for(gw = gws.begin(); gw != gws.end(); ++gw)
			{
				NetPacket *packet_copy = new NetPacket(packet);
				out_fifo = (*gw)->getDataOutStFifo();
				if(!this->onRcvEncapPacket(packet_copy,
				                           out_fifo,
				                           this->sat_delay))
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
					out_fifo_gw = (*gw)->getDataOutGwFifo();
					NetPacket *packet_copy_gw = new NetPacket(packet);
					if(!this->onRcvEncapPacket(packet_copy_gw,
					                           out_fifo_gw,
					                           this->sat_delay))
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

		spot = this->spots.find(spot_id);
		if(spot == this->spots.end())
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "cannot find spot with ID %u in spot "
			    "list\n", spot_id);
			return false;
		}

		SatGw *gw = this->spots[spot_id]->getGw(gw_id);

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
		                           this->sat_delay))
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


bool BlockDvbSat::Downward::sendFrames(DvbFifo *fifo)
{
	MacFifoElement *elem;
	//Be careful: When using other than time_ms_t in 32-bit machine, the RTT is no correct!
	time_ms_t current_time = getCurrentTime();

	while(((unsigned long)fifo->getTickOut()) <= current_time &&
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
			delete dvb_frame;
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

		for(gw_iter = listGw.begin() ; gw_iter != listGw.end(); ++gw_iter)
		{
			SatGw *current_gw = *gw_iter;
			current_gw->updateProbes(this->stats_period_ms);
		}
	}

	// Send probes
	Output::sendProbes();
}


set<tal_id_t> BlockDvbSat::Downward::getGwIds(void)
{
	set<tal_id_t> result;
	sat_spots_t::iterator it;
	list<SatGw *>::iterator it2;

	for(it = this->spots.begin();
		it != this->spots.end(); it++)
	{
		list<SatGw *> list = it->second->getListGw();
		for(it2 = list.begin(); it2 != list.end(); it2++)
		{
			result.insert((*it2)->getGwId());
		}
	}

	return result;
}


set<spot_id_t> BlockDvbSat::Downward::getSpotIds(void)
{
	set<spot_id_t> result;
	sat_spots_t::iterator it;

	for(it = this->spots.begin(); it != this->spots.end(); it++)
	{
		result.insert(it->second->getSpotId());
	}

	return result;
}

bool BlockDvbSat::Downward::initModcodSimu(void)
{
	set<tal_id_t> listGws = this->getGwIds();
	set<spot_id_t> listSpots = this->getSpotIds();
	for(set<spot_id_t>::iterator it1 = listSpots.begin();
	    it1 != listSpots.end(); it1++)
	{
		for(set<tal_id_t>::iterator it2 = listGws.begin();
		    it2 != listGws.end(); it2++)
		{
			FmtSimulation *fmt_simulation = new FmtSimulation();
			if(!this->initModcodFiles(RETURN_UP_MODCOD_TIME_SERIES,
			                          *fmt_simulation,
			                          (*it2), (*it1)))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to complete the modcod part of the "
				    "initialisation\n");
				return false;
			}
			this->setFmtSimulation((*it1), (*it2), fmt_simulation);
			if(!this->initModcodDefFile(RETURN_UP_MODCOD_DEF_RCS,
			                            this->input_modcod_def))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to complete the modcod part of the "
				    "initialisation\n");
				return false;
			}
			if(!this->initModcodDefFile(FORWARD_DOWN_MODCOD_DEF_S2,
			                            this->output_modcod_def))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to complete the modcod part of the "
				    "initialisation\n");
				return false;
			}
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
	return true;
}

void BlockDvbSat::Downward::setFmtSimulation(spot_id_t spot_id, tal_id_t gw_id,
                                             FmtSimulation* new_fmt_simu)
{
	sat_spots_t::iterator it = this->spots.find(spot_id);

	if(it == this->spots.end())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Spot %d not found\n", spot_id);
	}
	else
	{
		it->second->setFmtSimulation(gw_id, new_fmt_simu);
	}
}


bool BlockDvbSat::Downward::goFirstScenarioStep(spot_id_t spot_id, tal_id_t gw_id)
{
	sat_spots_t::iterator it = this->spots.find(spot_id);

	if(it != this->spots.end())
	{
		return it->second->goFirstScenarioStep(gw_id);
	}

	LOG(this->log_init, LEVEL_ERROR,
	    "Spot %d not found\n", spot_id);

	return false;
}


bool BlockDvbSat::Downward::goNextScenarioStep(spot_id_t spot_id, tal_id_t gw_id,
                                               double &duration)
{
	sat_spots_t::iterator it = this->spots.find(spot_id);

	if(it != this->spots.end())
	{
		return it->second->goNextScenarioStep(gw_id, duration);
	}

	LOG(this->log_init, LEVEL_ERROR,
	    "Spot %d not found\n", spot_id);

	return false;
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
				delete dvb_frame;
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
	sat_spots_t::iterator spot;
	spot_id_t spot_id;
	tal_id_t gw_id;
	unsigned int carrier_id = dvb_frame->getCarrierId();

	// get the satellite spot from which the DVB frame comes from
	if(!OpenSandConf::getSpotWithCarrierId(carrier_id, spot_id, gw_id))
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
			// Update probes and stats
			if(carrier_id == current_gw->getDataInStId())
			{
				// Update probes and stats
				current_gw->updateL2FromSt(frame->getPayloadLength());
			}
			else if(carrier_id == current_gw->getDataInGwId())
			{
				// Update probes and stats
				current_gw->updateL2FromGw(frame->getPayloadLength());
			}
			else
			{
				LOG(this->log_receive, LEVEL_CRITICAL,
				    "Wrong input carrier ID %u\n", carrier_id);
				return false;
			}


				/* The satellite is a regenerative one and the DVB frame contains
				 * a burst:
				 *  - extract the packets from the DVB frame,
				 *  - find the destination spot ID for each packet
				 *  - create a burst of encapsulation packets (NetBurst object)
				 *    with all the packets extracted from the DVB frame,
				 *  - send the burst to the upper layer.
				 */
			this->handleDvbBurst(dvb_frame, 
			                     current_gw, 
			                     current_spot);
		}
		break;

		/* forward the BB frame (and the burst that the frame contains) */
		// TODO see if we can factorize
		case MSG_TYPE_BBFRAME:
		{
			if(!this->handleBBFrame(dvb_frame, 
			                        current_gw, 
			                        current_spot))
			{
				return false;
			}
		}
		break;

		case MSG_TYPE_SALOHA_DATA:
		case MSG_TYPE_SALOHA_CTRL:
		{
			if(!this->handleSaloha(dvb_frame, 
			                       current_gw, 
			                       current_spot))
			{
				return false;
			}
		}
		break;

		// Generic control frames (SAC, TTP, etc)
		case MSG_TYPE_SAC:
		{
			if(!this->handleSac(dvb_frame, current_gw))
			{
				return false;
			}
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
				return false;
			}
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
				return false;
			}
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
				return false;
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

	return true;
}

bool BlockDvbSat::Upward::forwardDvbFrame(DvbFifo *fifo, DvbFrame *dvb_frame)
{
	return this->pushInFifo(fifo, (NetContainer *)dvb_frame, this->sat_delay);
}



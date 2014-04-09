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
 * @file BlockDvbSat.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "BlockDvbSat.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "GenericSwitch.h"

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
		    "initialisation");
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
	ConfigurationList::iterator iter;

	// Retrive FIFO size
	if(!globalConfig.getValue(SAT_DVB_SECTION, DVB_SIZE_FIFO, fifo_size))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SAT_DVB_SECTION, DVB_SIZE_FIFO);
		goto error;
	}

	// Retrieving the spots description
	if(!globalConfig.getListItems(SAT_DVB_SECTION, SPOT_LIST, spot_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing satellite spot list\n",
		    SAT_DVB_SECTION, SPOT_LIST);
		goto error;
	}

	for(iter = spot_list.begin(); iter != spot_list.end(); iter++)
	{
		spot_id_t spot_id = 0;
		uint8_t ctrl_id = 0;
		uint8_t data_in_carrier_id = 0;
		uint8_t data_out_gw_id = 0;
		uint8_t data_out_st_id = 0;
		uint8_t log_id = 0;
		SatSpot *new_spot;

		i++;
		// get the spot_id
		if(!globalConfig.getAttributeValue(iter, SPOT_ID, spot_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SAT_DVB_SECTION, SPOT_LIST,
			    SPOT_ID, i);
			goto error;
		}
		// get the ctrl_id
		if(!globalConfig.getAttributeValue(iter, CTRL_ID, ctrl_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SAT_DVB_SECTION, SPOT_LIST,
			    CTRL_ID, i);
			goto error;
		}
		// get the data_in_carrier_id
		if(!globalConfig.getAttributeValue(iter, DATA_IN_ID, data_in_carrier_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SAT_DVB_SECTION, SPOT_LIST,
			    DATA_IN_ID, i);
			goto error;
		}
		// get the data_out_gw_id
		if(!globalConfig.getAttributeValue(iter, DATA_OUT_GW_ID, data_out_gw_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SAT_DVB_SECTION, SPOT_LIST,
			    DATA_OUT_GW_ID, i);
			goto error;
		}
		// get the data_out_st_id
		if(!globalConfig.getAttributeValue(iter, DATA_OUT_ST_ID, data_out_st_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SAT_DVB_SECTION, SPOT_LIST,
			    DATA_OUT_ST_ID, i);
			goto error;
		}
		// get the log_id
		if(!globalConfig.getAttributeValue(iter, LOG_ID, log_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': failed to retrieve %s at "
			    "line %d\n", SAT_DVB_SECTION, SPOT_LIST,
			    LOG_ID, i);
			goto error;
		}

		// create a new spot
		new_spot = new SatSpot(spot_id,
		                       data_in_carrier_id,
		                       log_id,
		                       ctrl_id,
		                       data_out_st_id,
		                       data_out_gw_id,
		                       fifo_size);
		if(new_spot == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to create a new satellite spot\n");
			goto error;
		}

		LOG(this->log_init, LEVEL_NOTICE,
		    "satellite spot %u: logon = %u, control = %u, "
		    "data out ST = %u, data out GW = %u\n",
		    spot_id, log_id, ctrl_id, data_out_st_id,
		    data_out_gw_id);
		// store the new satellite spot in the list of spots
		this->spots[spot_id] = new_spot;
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
	categories(),
	terminal_affectation(),
	default_category(),
	fmt_groups(),
	spots(),
	probe_sat_output_gw_queue_size(),
	probe_sat_output_gw_queue_size_kb(),
	probe_sat_output_st_queue_size(),
	probe_sat_output_st_queue_size_kb(),
	probe_sat_l2_from_st(),
	probe_sat_l2_to_st(),
	probe_sat_l2_from_gw(),
	probe_sat_l2_to_gw(),
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

	for(TerminalCategories::iterator it = this->categories.begin();
	    it != this->categories.end(); ++it)
	{
		delete (*it).second;
	}
	this->categories.clear();

	this->terminal_affectation.clear();
}


void BlockDvbSat::Downward::setSpots(const sat_spots_t &spots)
{
	this->spots = spots;
}



bool BlockDvbSat::Downward::onInit()
{
	// get the common parameters
	if(!this->initCommon(DOWN_FORWARD_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation");
		return false;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation");
		return false;
	}

	this->down_frame_counter = 0;

	if(!this->initSatLink())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of "
		    "link parameters");
		return false;
	}

	// load the modcod files (regenerative satellite only)
	if(this->satellite_type == REGENERATIVE)
	{
		if(!this->initModcodFiles(DOWN_FORWARD_MODCOD_DEF,
		                          DOWN_FORWARD_MODCOD_SIMU))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the modcod part of the "
			    "initialisation");
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
			    "initialisation");
			return false;
		}
	}

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize Output probes ans stats");
		return false;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize timers");
		return false;
	}

	return true;
}


bool BlockDvbSat::Downward::initSatLink(void)
{
	if(!globalConfig.getValue(GLOBAL_SECTION, SAT_DELAY, this->sat_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, SAT_DELAY);
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "Satellite delay = %d", this->sat_delay);

	if(this->satellite_type == REGENERATIVE)
	{
		if(!this->initBand(DOWN_FORWARD_BAND,
		                   this->fwd_timer_ms,
		                   this->categories,
		                   this->terminal_affectation,
		                   &this->default_category,
		                   this->fmt_groups))
		{
			return false;
		}

		if(this->categories.size() != 1)
		{
			// TODO see NCC for that, we may handle categories in
			//      spots here.
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot support more than one category for "
			    "downlink band\n");
			return false;
		}

		for(sat_spots_t::iterator i_spot = this->spots.begin();
		    i_spot != this->spots.end(); i_spot++)
		{
			SatSpot *spot;
			spot = i_spot->second;

			if(!spot->initScheduling(this->pkt_hdl,
			                         &this->fmt_simu,
			                         this->categories.begin()->second))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to init the spot scheduling\n");
				delete spot;
				return false;
			}
		}
	}
	return true;
}


bool BlockDvbSat::Downward::initTimers(void)
{
	// create frame timer (also used to send packets waiting in fifo)
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                       this->fwd_timer_ms);

	this->stats_timer = this->addTimerEvent("dvb_stats",
	                                        this->stats_period_ms);

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
	if(!globalConfig.getListItems(SAT_SIMU_COL_SECTION, COLUMN_LIST,
	                              column_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': problem retrieving simulation "
		    "column list\n", SAT_SIMU_COL_SECTION, COLUMN_LIST);
		goto error;
	}

	for(iter = column_list.begin(); iter != column_list.end(); iter++)
	{
		i++;
		tal_id_t tal_id;
		long column_nbr;

		// Get the Tal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in simulation column "
			    "entry %d\n", TAL_ID, i);
			goto error;
		}
		// Get the column nbr
		if(!globalConfig.getAttributeValue(iter, COLUMN_NBR, column_nbr))
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
		unsigned int spot_id = spot->getSpotId();
		Probe<int> *probe_output_gw;
		Probe<int> *probe_output_gw_kb;
		Probe<int> *probe_output_st;
		Probe<int> *probe_output_st_kb;
		Probe<int> *probe_l2_to_st;
		Probe<int> *probe_l2_from_st;

		probe_output_gw = Output::registerProbe<int>(
			"Packets", true, SAMPLE_LAST,
			"Spot %d.Queue size.Output_GW", spot_id);
		this->probe_sat_output_gw_queue_size.insert(
			std::pair<unsigned int, Probe<int> *> (spot_id, probe_output_gw));

		probe_output_gw_kb = Output::registerProbe<int>(
			"Kbits", true, SAMPLE_LAST,
			"Spot %d.Queue size.Output_GW_kb", spot_id);
		this->probe_sat_output_gw_queue_size_kb.insert(
			std::pair<unsigned int, Probe<int> *>(spot_id, probe_output_gw_kb));

		probe_output_st = Output::registerProbe<int>(
			"Packets", true, SAMPLE_LAST,
			"Spot %d.Queue size.Output_ST", spot_id);
		this->probe_sat_output_st_queue_size.insert(
			std::pair<unsigned int, Probe<int> *>(spot_id, probe_output_st));

		probe_output_st_kb = Output::registerProbe<int>(
			"Kbits", true, SAMPLE_LAST,
			"Spot %d.Queue size.Output_ST_kb", spot_id);
		this->probe_sat_output_st_queue_size_kb.insert(
			std::pair<unsigned int, Probe<int> *>(spot_id, probe_output_st_kb));

		probe_l2_to_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot %d.Throughputs.L2_to_ST", spot_id);
		this->probe_sat_l2_to_st.insert(
			std::pair<unsigned int, Probe<int> *>(spot_id, probe_l2_to_st));

		probe_l2_from_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot %d.Throughputs.L2_from_ST", spot_id);
		this->probe_sat_l2_from_st.insert(
			std::pair<unsigned int, Probe<int> *>(spot_id, probe_l2_from_st));

		if(this->satellite_type == TRANSPARENT)
		{
			Probe<int> *probe_l2_to_gw;
			Probe<int> *probe_l2_from_gw;

			probe_l2_to_gw = Output::registerProbe<int>(
				"Kbits/s", true, SAMPLE_LAST, "Spot %d.Throughputs.L2_to_GW",
				spot_id);
			this->probe_sat_l2_to_gw.insert(
				std::pair<unsigned int, Probe<int> *>(spot_id, probe_l2_to_gw));
			probe_l2_from_gw = Output::registerProbe<int>(
				"Kbits/s", true, SAMPLE_LAST, "Spot %d.Throughputs.L2_from_GW",
				spot_id);
			this->probe_sat_l2_from_gw.insert(
				std::pair<unsigned int, Probe<int> *>
				(spot_id, probe_l2_from_gw));
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
				// send frame for every satellite spot
				for(sat_spots_t::iterator i_spot = this->spots.begin();
				    i_spot != this->spots.end(); i_spot++)
				{
					SatSpot *current_spot = i_spot->second;
					// copy the frame because it will be sent on other spots
					DvbFrame *dvb_frame_copy = new DvbFrame(dvb_frame);

					if(!this->sendSigFrame(dvb_frame_copy, current_spot))
					{
						status = false;
					}
				}
				delete dvb_frame;
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
			uint8_t spot_id;
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
				LOG(this->log_receive, LEVEL_INFO,
				    "store one encapsulation packet\n");
				spot_id = (*pkt_it)->getDstSpot();
				iter = this->spots.find(spot_id);
				if(iter == this->spots.end())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find spot with ID %u in spot "
					    "list\n", spot_id);
					break;
				}
				if(!this->onRcvEncapPacket(*pkt_it,
					                       this->spots[spot_id]->getDataOutStFifo(),
					                       this->sat_delay))
				{
					// a problem occured, we got memory allocation error
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

					if(this->satellite_type == TRANSPARENT)
					{
						// send frame for every satellite spot
						bool status = true;

						LOG(this->log_receive, LEVEL_DEBUG,
						    "send data frames on satellite spot "
						    "%u\n", i_spot->first);
						if(!this->sendFrames(current_spot->getDataOutGwFifo()))
						{
							status = false;
						}
						if(!this->sendFrames(current_spot->getDataOutStFifo()))
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
						if(!current_spot->schedule(this->down_frame_counter,
						                           this->getCurrentTime()))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "failed to schedule packets for satellite spot %u "
							    "on regenerative satellite\n", i_spot->first);
							return false;
						}

						if(!this->sendBursts(&current_spot->getCompleteDvbFrames(),
						                     current_spot->getDataOutStFifo()->getCarrierId()))
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
			else if(*event == this->stats_timer)
			{
				this->updateStats();
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
			    "unknown event: %s", event->getName().c_str());
	}

	return true;
}


bool BlockDvbSat::Downward::sendFrames(DvbFifo *fifo)
{
	MacFifoElement *elem;
	time_ms_t current_time = this->getCurrentTime();

	while(fifo->getTickOut() <= current_time &&
	      fifo->getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;
		size_t length;

		elem = fifo->pop();
		assert(elem != NULL);

		// check that we got a DVB frame in the SAT cell
		if(elem->getType() != 0)
		{
			LOG(this->log_send, LEVEL_ERROR,
			    "FIFO element does not contain a DVB or BB "
			    "frame\n");
			goto error;
		}
		dvb_frame = elem->getFrame();
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


bool BlockDvbSat::Downward::sendSigFrame(DvbFrame *frame, const SatSpot *const spot)
{
	uint8_t carrier_id;
	size_t length;

	switch(frame->getMessageType())
	{
		case MSG_TYPE_SAC:
		case MSG_TYPE_SOF:
		case MSG_TYPE_TTP:
		case MSG_TYPE_SYNC:
		case MSG_TYPE_SESSION_LOGON_RESP:
			carrier_id = spot->getControlCarrierId();
			break;

		case MSG_TYPE_SESSION_LOGON_REQ:
			carrier_id = spot->getLogonCarrierId();
			break;

		default:
			LOG(this->log_send, LEVEL_ERROR,
			    "Frame is not a sig frame\n");
			goto error;
	}

	length = frame->getTotalLength();
	// create a message for the DVB frame
	if(!this->sendDvbFrame(frame, carrier_id))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send sig frame to lower layer, "
		    "drop it\n");
		goto error;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "Sig frame sent with a size of %zu\n", length);

	return true;

error:
	delete frame;
	return false;
}


void BlockDvbSat::Downward::updateStats(void)
{
	// Update stats and probes

	sat_spots_t::iterator spot_it;
	for (spot_it = this->spots.begin(); spot_it != spots.end(); ++spot_it)
	{
		SatSpot* spot = (*spot_it).second;
		unsigned int spot_id = spot->getSpotId();
		// Queue sizes
		mac_fifo_stat_context_t output_gw_fifo_stat;
		mac_fifo_stat_context_t output_st_fifo_stat;
		spot->getDataOutStFifo()->getStatsCxt(output_st_fifo_stat);
		spot->getDataOutGwFifo()->getStatsCxt(output_gw_fifo_stat);
		this->probe_sat_output_gw_queue_size[spot_id]->put(
			output_gw_fifo_stat.current_pkt_nbr);
		this->probe_sat_output_gw_queue_size_kb[spot_id]->put(
			((int) output_gw_fifo_stat.current_length_bytes * 8 / 1000));

		this->probe_sat_output_st_queue_size[spot_id]->put(
			output_st_fifo_stat.current_pkt_nbr);
		this->probe_sat_output_st_queue_size_kb[spot_id]->put(
			((int) output_st_fifo_stat.current_length_bytes * 8 / 1000));

		// Throughputs
		// L2 from ST
		this->probe_sat_l2_from_st[spot_id]->put(
			spot->getL2FromSt() * 8 / this->stats_period_ms);

		// L2 to ST
		this->probe_sat_l2_to_st[spot_id]->put(
			((int) output_st_fifo_stat.out_length_bytes * 8 /
			this->stats_period_ms));

		if(this->satellite_type == TRANSPARENT)
		{
			// L2 from GW
			this->probe_sat_l2_from_gw[spot_id]->put(
				spot->getL2FromGw() * 8 / this->stats_period_ms);

			// L2 to GW
			this->probe_sat_l2_to_gw[spot_id]->put(
				((int) output_gw_fifo_stat.out_length_bytes * 8 /
				this->stats_period_ms));
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
	spots(),
	cni(),
	sat_delay()
{
};


BlockDvbSat::Upward::~Upward()
{
}


void BlockDvbSat::Upward::setSpots(const sat_spots_t &spots)
{
	this->spots = spots;
}



bool BlockDvbSat::Upward::onInit()
{
	// get the common parameters
	if(!this->initCommon(UP_RETURN_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation");
		return false;;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation");
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
			    "initialisation");
			return false;
		}
	}

	return true;
}


bool BlockDvbSat::Upward::initMode(void)
{
	// Delay to apply to the medium
	if(!globalConfig.getValue(GLOBAL_SECTION, SAT_DELAY, this->sat_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, SAT_DELAY);
		goto error;
	}
	    
	LOG(this->log_init, LEVEL_NOTICE,
	     "Satellite delay = %d", this->sat_delay);

	if(this->satellite_type == REGENERATIVE)
	{
		this->receptionStd = new DvbRcsStd(this->pkt_hdl);
	}
	else
	{
		// create the reception standard
		this->receptionStd = new DvbRcsStd(); 
	}
	if(this->receptionStd == NULL)
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
	ConfigurationList switch_list;
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
	if(!globalConfig.getListItems(SAT_SWITCH_SECTION, SWITCH_LIST, switch_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing satellite switching "
		    "table\n", SAT_SWITCH_SECTION, SWITCH_LIST);
		goto error;
	}


	i = 0;
	for(iter = switch_list.begin(); iter != switch_list.end(); iter++)
	{
		tal_id_t tal_id = 0;
		spot_id = 0;

		i++;
		// get the Tal ID attribute
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in switching table"
			    "entry %u\n", TAL_ID, i);
			goto release_switch;
		}

		// get the Spot ID attribute
		if(!globalConfig.getAttributeValue(iter, SPOT_ID, spot_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "problem retrieving %s in switching table"
			    "entry %u\n", SPOT_ID, i);
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

	// get default spot id
	if(!globalConfig.getValue(SAT_SWITCH_SECTION, DEFAULT_SPOT, spot_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SAT_SWITCH_SECTION, DEFAULT_SPOT);
		goto error;
	}
	generic_switch->setDefault(spot_id);

	if(!(dynamic_cast<DvbRcsStd *>(this->receptionStd)->setSwitch(generic_switch)))
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
			    "drop it");
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

		if(this->satellite_type == TRANSPARENT)
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "DVB-Frame received\n");

			// get the satellite spot from which the DVB frame comes from
			for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
			{
				SatSpot *current_spot = spot->second;

				if(current_spot->getInputCarrierId() == dvb_frame->getCarrierId())
				{
					// satellite spot found, forward DVB frame on the same spot
					DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();

					// Update probes and stats
					current_spot->updateL2FromSt(frame->getPayloadLength());

					// TODO: forward according to a table
					LOG(this->log_receive, LEVEL_INFO,
					    "DVB burst comes from spot %u (carrier "
					    "%u) => forward it to spot %u (carrier "
					    "%u)\n", current_spot->getSpotId(),
					    current_spot->getInputCarrierId(),
					    current_spot->getSpotId(),
					    current_spot->getDataOutGwFifo()->
					    getCarrierId());

					if(!this->forwardDvbFrame(current_spot->getDataOutGwFifo(),
					                          dvb_frame))
					{
						status = false;
					}

					// satellite spot found, abort the search
					break;
				}
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
			for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
			{
				SatSpot *current_spot = spot->second;

				if(current_spot->getInputCarrierId() == dvb_frame->getCarrierId())
				{
					DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
					current_spot->updateL2FromSt(frame->getPayloadLength());
				}
			}

			if(this->with_phy_layer && this->satellite_type == REGENERATIVE &&
			   this->receptionStd->getType() == "DVB-RCS")
			{
				DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
				tal_id_t tal_id;
				// decode the first packet in frame to be able to get source terminal ID
				if(!this->pkt_hdl->getSrc(frame->getPayload(), tal_id))
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
					    tal_id, cn);

					this->cni[tal_id] = cn;
				}
			}

			if(!this->receptionStd->onRcvFrame(dvb_frame,
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

		// get the satellite spot from which the DVB frame comes from
		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			SatSpot *current_spot = spot->second;

			if(current_spot->getInputCarrierId() == dvb_frame->getCarrierId())
			{
				// satellite spot found, forward BBframe on the same spot
				BBFrame *bbframe = dvb_frame->operator BBFrame*();

				// Update probes and stats
				current_spot->updateL2FromGw(bbframe->getPayloadLength());

				// TODO: forward according to a table
				LOG(this->log_receive, LEVEL_INFO,
				    "BBFRAME burst comes from spot %u (carrier "
				    "%u) => forward it to spot %u (carrier %u)\n",
				    current_spot->getSpotId(),
				    current_spot->getInputCarrierId(),
				    current_spot->getSpotId(),
				    current_spot->getDataOutStFifo()->
				    getCarrierId());

				if(!this->forwardDvbFrame(current_spot->getDataOutStFifo(),
				                          dvb_frame))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot forward burst\n");
					status = false;
				}

				// satellite spot found, abort the search
				break;
			}
		}
	}
	break;

	// Generic control frames (SAC, TTP, etc)
	case MSG_TYPE_SAC:
		if(this->with_phy_layer && this->satellite_type == REGENERATIVE)
		{
			// handle SAC here to get the uplink ACM parameters
			// TODO Sac *sac = dynamic_cast<Sac *>(dvb_frame);
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
			if(!this->shareMessage((void **)&cni_info, sizeof(cni_info_t),
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
	case MSG_TYPE_SOF:
	case MSG_TYPE_TTP:
	case MSG_TYPE_SYNC:
	case MSG_TYPE_SESSION_LOGON_RESP:
	case MSG_TYPE_SESSION_LOGON_REQ:
	{
		LOG(this->log_receive, LEVEL_DEBUG,
		    "control frame (type = %u) received, forward it on all satellite spots\n",
		    dvb_frame->getMessageType());
		// the message should not be stored in fifo, especially SOF because it
		// would be kept a random amount of time between [0, fwd_timer]
		// and we need a perfect synchronization
		if(!this->shareMessage((void **)&dvb_frame, sizeof(dvb_frame),
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

// TODO duplicate onRcvEncapPacket if we store NetContainer in Fifos
bool BlockDvbSat::Upward::forwardDvbFrame(DvbFifo *fifo, DvbFrame *dvb_frame)
{
	MacFifoElement *elem;
	time_ms_t current_time;

	current_time = this->getCurrentTime();

	// Get a room with timestamp in fifo
	elem = new MacFifoElement(dvb_frame, current_time,
	                          current_time + this->sat_delay);
	if(!elem)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to create a MAC FIFO element, drop the "
		    "frame\n");
		goto error;
	}

	// Fill the delayed queue
	if(!fifo->push(elem))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "FIFO %s full, drop the DVB frame\n",
		    fifo->getName().c_str());
		goto release_elem;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "frame stored in FIFO for carrier ID %d "
	    "(tick_in = %ld, tick_out = %ld)\n",
	    fifo->getCarrierId(),
	    elem->getTickIn(), elem->getTickOut());

	return true;

release_elem:
	delete elem;
error:
	delete dvb_frame;
	return false;
}



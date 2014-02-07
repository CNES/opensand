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

// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_DVB_RCS_SAT
#include <opensand_conf/uti_debug.h>

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


BlockDvbSat::BlockDvbSat(const string &name):
	BlockDvb(name),
	spots(),
	frame_timer(-1),
	scenario_timer(-1),
	categories(),
	terminal_affectation(),
	fmt_groups(),
	// TODO add a parameter for that or use frame timer
	stats_period_ms(106),
	probe_frame_interval(NULL)
{
}


// BlockDvbSat dtor
BlockDvbSat::~BlockDvbSat()
{
	SpotMap::iterator i_spot;

	// delete the satellite spots
	for(i_spot = this->spots.begin(); i_spot != this->spots.end(); i_spot++)
	{
		delete i_spot->second;
	}

	// release the reception DVB standards
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}
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


bool BlockDvbSat::onUpwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
			// message from lower layer: dvb frame
			DvbFrame *dvb_frame;

			dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			if(!this->onRcvDvbFrame(dvb_frame))
			{
				UTI_ERROR("failed to handle received DVB frame\n");
				return false;
			}

			break;

		default:
			UTI_ERROR("unknown event: %s", event->getName().c_str());
			return false;
	}
	return true;
}

bool BlockDvbSat::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			NetBurst *burst;
			uint8_t spot_id;
			NetBurst::iterator pkt_it;

			if(this->satellite_type != REGENERATIVE)
			{
				UTI_ERROR("message event while satellite is transparent");
				return false;
			}
			// message from upper layer: burst of encapsulation packets
			burst = (NetBurst *)((MessageEvent *)event)->getData();

			UTI_DEBUG("encapsulation burst received (%d packet(s))\n",
			          burst->length());

			// for each packet of the burst
			for(pkt_it = burst->begin(); pkt_it != burst->end();
			    pkt_it++)
			{
				SpotMap::iterator iter;
				UTI_DEBUG("store one encapsulation packet\n");
				spot_id = (*pkt_it)->getDstSpot();
				iter = this->spots.find(spot_id);
				if(iter == this->spots.end())
				{
					UTI_ERROR("cannot find spot with ID %u in spot list\n",
					          spot_id);
					break;
				}
				if(!this->onRcvEncapPacket(*pkt_it,
					                       &this->spots[spot_id]->data_out_st_fifo,
					                       this->m_delay))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					UTI_ERROR("unable to store packet\n");
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
			if(*event == this->frame_timer)
			{

				// Update stats and probes
				if(this->probe_frame_interval->isEnabled())
				{
					timeval time = event->getAndSetCustomTime();
					float val = time.tv_sec * 1000000L + time.tv_usec;
					this->probe_frame_interval->put(val / 1000);
				}

				// increment counter of frames per superframe
				this->frame_counter++;

				// if we reached the end of a superframe and the
				// beginning of a new one, send SOF and run allocation
				// algorithms (DAMA)
				if(this->frame_counter == this->frames_per_superframe)
				{
					// increase the superframe number and reset
					// counter of frames per superframe
					this->super_frame_counter++;
					this->frame_counter = 0;
				}
				UTI_DEBUG_L3("frame timer expired, send DVB frames\n");

				// send frame for every satellite spot
				for(SpotMap::iterator i_spot = this->spots.begin();
				    i_spot != this->spots.end(); i_spot++)
				{
					SatSpot *current_spot;

					current_spot = i_spot->second;

					UTI_DEBUG_L3("send logon frames on satellite spot %u\n",
					             i_spot->first);
					if(!this->sendSigFrames(&current_spot->logon_fifo))
					{
						UTI_ERROR("Failed to send logon frames on spot %u\n",
						          i_spot->first);
					}

					UTI_DEBUG_L3("send control frames on satellite spot %u\n",
					             i_spot->first);
					if(!this->sendSigFrames(&current_spot->control_fifo))
					{
						UTI_ERROR("Failed to send contol frames on spot %u\n",
						          i_spot->first);
					}

					if(this->satellite_type == TRANSPARENT)
					{
						bool status = true;
						// note: be careful that the reception standard
						// is also used to send frames because the reception
						// standard toward ST is the emission standard
						// toward GW (this should be reworked)

						UTI_DEBUG_L3("send data frames on satellite spot %u\n",
						             i_spot->first);
						if(!this->onSendFrames(&current_spot->data_out_gw_fifo,
						                       this->getCurrentTime()))
						{
							status = false;
						}
						if(!this->onSendFrames(&current_spot->data_out_st_fifo,
						                       this->getCurrentTime()))
						{
							status = false;
						}
						if(!status)
						{
							return false;
						}
					}
					else // REGENERATIVE
					{
						if(!current_spot->schedule(this->super_frame_counter,
						                           this->frame_counter,
						                           this->getCurrentTime()))
						{
							UTI_ERROR("failed to schedule packets "
							          "for satellite spot %u "
							          "on regenerative satellite\n",
							          i_spot->first);
							return false;
						}

						if(!this->sendBursts(&current_spot->complete_dvb_frames,
						                     current_spot->data_out_st_fifo.getCarrierId()))
						{
							UTI_ERROR("failed to build and send "
							          "DVB/BB frames "
							          "for satellite spot %u "
							          "on regenerative satellite\n",
							          i_spot->first);
							return false;
						}
					}
				}
			}
			else if(*event == this->scenario_timer)
			{
				UTI_DEBUG_L3("MODCOD scenario timer expired\n");

				UTI_DEBUG_L3("update modcod table\n");
				if(!this->fwd_fmt_simu.goNextScenarioStep(true))
				{
					UTI_ERROR("failed to update MODCOD IDs\n");
					return false;
				}
			}
			else if (*event == this->stats_timer)
			{
				this->updateStats();
			}
			else
			{
				UTI_ERROR("unknown timer event received %s\n",
				          event->getName().c_str());
			}
			break;

		default:
			UTI_ERROR("unknown event: %s", event->getName().c_str());
	}

	return true;
}


bool BlockDvbSat::initMode()
{
	int val;

	// Delay to apply to the medium
	if(!globalConfig.getValue(GLOBAL_SECTION, SAT_DELAY, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SAT_DELAY);
		goto error;
	}
	this->m_delay = val;
	UTI_INFO("m_delay = %d", this->m_delay);

	if(this->satellite_type == REGENERATIVE)
	{
		if(!this->initBand(DOWN_FORWARD_BAND,
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
			UTI_ERROR("cannot support more than one category for downlink band\n");
			goto error;
		}

		this->receptionStd = new DvbRcsStd(this->up_return_pkt_hdl);
		// scheduling will be initialized in spots
	}
	else
	{
		// create the reception standard
		this->receptionStd = new DvbRcsStd();
		if(this->receptionStd == NULL)
		{
			UTI_ERROR("failed to create the reception standard\n");
			goto error;
		}
	}

	return true;

error:
	return false;
}


// TODO call in Downward::onInit()
bool BlockDvbSat::initDownwardTimers()
{
	// launch frame timer
	this->frame_timer = this->downward->addTimerEvent("dvb_frame_timer",
	                                                  this->frame_duration_ms);

	if(this->satellite_type == REGENERATIVE && !this->with_phy_layer)
	{
		// launch the timer in order to retrieve the modcods
		this->scenario_timer = this->downward->addTimerEvent("dvb_scenario_timer",
		                                                     this->dvb_scenario_refresh);
	}

	return true;
}


bool BlockDvbSat::initSwitchTable()
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
		UTI_ERROR("section '%s, %s': missing satellite switching table\n",
		          SAT_SWITCH_SECTION, SWITCH_LIST);
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
			UTI_ERROR("problem retrieving %s in switching table"
			          "entry %u\n", TAL_ID, i);
			goto release_switch;
		}

		// get the Spot ID attribute
		if(!globalConfig.getAttributeValue(iter, SPOT_ID, spot_id))
		{
			UTI_ERROR("problem retrieving %s in switching table"
			          "entry %u\n", SPOT_ID, i);
			goto release_switch;
		}

		if(!generic_switch->add(tal_id, spot_id))
		{
			UTI_ERROR("failed to add switching entry "
			          "(Tal ID = %u, Spot ID = %u)\n",
			          tal_id, spot_id);
			goto release_switch;
		}

		UTI_INFO("Switching entry added (Tal ID = %u, "
		         "Spot ID = %u)\n", tal_id, spot_id);
	}

	// get default spot id
	if(!globalConfig.getValue(SAT_SWITCH_SECTION, DEFAULT_SPOT, spot_id))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
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


bool BlockDvbSat::initSpots()
{
	int i = 0;
	size_t fifo_size;
	ConfigurationList spot_list;
	ConfigurationList::iterator iter;

	// Retrive FIFO size
	if(!globalConfig.getValue(SAT_DVB_SECTION, DVB_SIZE_FIFO, fifo_size))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          SAT_DVB_SECTION, DVB_SIZE_FIFO);
		goto error;
	}

	// Retrieving the spots description
	if(!globalConfig.getListItems(SAT_DVB_SECTION, SPOT_LIST, spot_list))
	{
		UTI_ERROR("section '%s, %s': missing satellite spot list\n",
		          SAT_DVB_SECTION, SPOT_LIST);
		goto error;
	}

	for(iter = spot_list.begin(); iter != spot_list.end(); iter++)
	{
		uint8_t spot_id = 0;
		unsigned int ctrl_id;
		unsigned int data_in_carrier_id;
		unsigned int data_out_gw_id;
		unsigned int data_out_st_id;
		unsigned int log_id;
		SatSpot *new_spot;

		i++;
		// get the spot_id
		if(!globalConfig.getAttributeValue(iter, SPOT_ID, spot_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SAT_DVB_SECTION, SPOT_LIST, SPOT_ID, i);
			goto error;
		}
		// get the ctrl_id
		if(!globalConfig.getAttributeValue(iter, CTRL_ID, ctrl_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SAT_DVB_SECTION, SPOT_LIST, CTRL_ID, i);
			goto error;
		}
		// get the data_in_carrier_id
		if(!globalConfig.getAttributeValue(iter, DATA_IN_ID, data_in_carrier_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SAT_DVB_SECTION, SPOT_LIST, DATA_IN_ID, i);
			goto error;
		}
		// get the data_out_gw_id
		if(!globalConfig.getAttributeValue(iter, DATA_OUT_GW_ID, data_out_gw_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SAT_DVB_SECTION, SPOT_LIST, DATA_OUT_GW_ID, i);
			goto error;
		}
		// get the data_out_st_id
		if(!globalConfig.getAttributeValue(iter, DATA_OUT_ST_ID, data_out_st_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SAT_DVB_SECTION, SPOT_LIST, DATA_OUT_ST_ID, i);
			goto error;
		}
		// get the log_id
		if(!globalConfig.getAttributeValue(iter, LOG_ID, log_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SAT_DVB_SECTION, SPOT_LIST, LOG_ID, i);
			goto error;
		}

		// create a new spot
		new_spot = new SatSpot();
		if(new_spot == NULL)
		{
			UTI_ERROR("failed to create a new satellite spot\n");
			goto error;
		}

		// initialize the new spot
		// TODO: check the fact the spot we enter is not a double
		UTI_INFO("satellite spot %u: logon = %u, control = %u, "
		         "data out ST = %u, data out GW = %u\n",
		         spot_id, log_id, ctrl_id, data_out_st_id, data_out_gw_id);
		if(!new_spot->initFifos(spot_id, data_in_carrier_id,
		                        log_id, ctrl_id, data_out_st_id,
		                        data_out_gw_id, fifo_size))
		{
			UTI_ERROR("failed to init the new satellite spot\n");
			delete new_spot;
			goto error;
		}
		if(this->satellite_type == REGENERATIVE &&
		   !new_spot->initScheduling(this->down_forward_pkt_hdl,
		                             &this->fwd_fmt_simu,
		                             this->categories.begin()->second,
		                             this->frames_per_superframe))
		{
			UTI_ERROR("failed to init the spot scheduling\n");
			delete new_spot;
			goto error;
		}

		// store the new satellite spot in the list of spots
		this->spots[spot_id] = new_spot;
	}

	return true;

error:
	return false;
}


bool BlockDvbSat::initStList()
{
	int i = 0;
	ConfigurationList column_list;
	ConfigurationList::iterator iter;

	// Get the list of STs
	if(!globalConfig.getListItems(SAT_SIMU_COL_SECTION, COLUMN_LIST,
	                              column_list))
	{
		UTI_ERROR("section '%s, %s': problem retrieving simulation column "
		          "list\n", SAT_SIMU_COL_SECTION, COLUMN_LIST);
		goto error;
	}

	for(iter = column_list.begin(); iter != column_list.end(); iter++)
	{
		i++;
		long tal_id;
		long column_nbr;

		// Get the Tal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("problem retrieving %s in simulation column "
			          "entry %d\n", TAL_ID, i);
			goto error;
		}
		// Get the column nbr
		if(!globalConfig.getAttributeValue(iter, COLUMN_NBR, column_nbr))
		{
			UTI_ERROR("problem retrieving %s in simulation column "
			          "entry %d\n", COLUMN_NBR, i);
			goto error;
		}

		// register a ST only if it did not exist yet
		// (duplicate because STs are 'defined' in spot table)
		if(!this->fwd_fmt_simu.doTerminalExist(tal_id))
		{
			if(!this->fwd_fmt_simu.addTerminal(tal_id, column_nbr))
			{
				UTI_ERROR("failed to register ST "
				          "with Tal ID %ld\n", tal_id);
				goto error;
			}
		}
	}

	return true;

error:
	return false;
}

bool  BlockDvbSat::onInit()
{
	int val;

	// get the common parameters
	if(!this->initCommon())
	{
		UTI_ERROR("failed to complete the common part of the initialisation");
		goto error;
	}

	if(!this->initMode())
	{
		UTI_ERROR("failed to complete the mode part of the "
		          "initialisation");
		goto error;
	}

	// load the modcod files (regenerative satellite only)
	if(this->satellite_type == REGENERATIVE)
	{
		if(!this->initForwardModcodFiles())
		{
			UTI_ERROR("failed to complete the modcod part of the "
 			          "initialisation");
			goto error;
		}
		// initialize the MODCOD scheme ID
		if(!this->fwd_fmt_simu.goNextScenarioStep(true))
		{
			UTI_ERROR("failed to initialize downlink MODCOD IDs\n");
			goto error;
		}

		if(!this->initStList())
		{
			UTI_ERROR("failed to complete the ST part of the"
 			          "initialisation");
			goto error;
		}

		// initialize the satellite internal switch
		if(!this->initSwitchTable())
		{
			UTI_ERROR("failed to complete the switch part of the "
			          "initialisation");
			goto error;
		}

	}

	// read the frame duration, the super frame duration
	// and the second duration
	if(!this->initDownwardTimers())
	{
		UTI_ERROR("failed to complete the timers part of the "
		          "initialisation");
		goto error;
	}

	// read and initialize the random seed
	if(!globalConfig.getValue(SAT_DVB_SECTION, SAT_RAND_SEED, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          SAT_DVB_SECTION, SAT_RAND_SEED);
		goto error;
	}
	srand(val);
	UTI_INFO("random seed is %d", val);

	// initialize the satellite spots
	if(!this->initSpots())
	{
		UTI_ERROR("failed to complete the spots part of the "
		          "initialisation");
		goto error;
	}

	// initialize output probe and stats
	this->stats_timer = this->downward->addTimerEvent("BlockSatStats",
			this->stats_period_ms);

	if(!this->initOutput())
	{
		UTI_ERROR("failed to initialize Output probes ans stats");
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvbSat::initOutput()
{
	// Output probes and stats
	SpotMap::iterator spot_it;
	for (spot_it=this->spots.begin(); spot_it!=spots.end(); ++spot_it)
	{
		SatSpot* spot = spot_it->second;
		unsigned int spot_id = spot->getSpotId();
		Probe<int> *probe_output_gw;
		Probe<int> *probe_output_gw_kb;
		Probe<int> *probe_output_st;
		Probe<int> *probe_output_st_kb;
		Probe<int> *probe_phy_from_st;
		Probe<int> *probe_l2_from_st;
		Probe<int> *probe_l2_to_st;

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

		probe_phy_from_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot %d.Throughputs.PHY_from_ST", spot_id);
		this->probe_sat_phy_from_st.insert(
			std::pair<unsigned int, Probe<int> *> (spot_id, probe_phy_from_st));
		this->phy_from_st_bytes.insert(std::pair<unsigned int, int>(spot_id, 0));

		probe_l2_from_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot %d.Throughputs.L2_from_ST", spot_id);
		this->probe_sat_l2_from_st.insert(
			std::pair<unsigned int, Probe<int> *> (spot_id, probe_l2_from_st));
		this->l2_from_st_bytes.insert(std::pair<unsigned int, int>(spot_id, 0));

		probe_l2_to_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot %d.Throughputs.L2_to_ST", spot_id);
		this->probe_sat_l2_to_st.insert(
			std::pair<unsigned int, Probe<int> *> (spot_id, probe_l2_to_st));
		this->l2_to_st_bytes.insert(std::pair<unsigned int, int>(spot_id, 0));

		if(this->satellite_type == TRANSPARENT)
		{
			Probe<int> *probe_phy_from_gw;
			Probe<int> *probe_l2_from_gw;
			Probe<int> *probe_l2_to_gw;

			probe_phy_from_gw = Output::registerProbe<int>(
				"Kbits/s", true, SAMPLE_LAST, "Spot %d.Throughputs.PHY_from_GW",
				spot_id);
			this->probe_sat_phy_from_gw.insert(
				std::pair<unsigned int, Probe<int> *> (spot_id,
				probe_phy_from_gw));
			this->phy_from_gw_bytes.insert(std::pair<unsigned int, int>
				(spot_id, 0));

			probe_l2_from_gw = Output::registerProbe<int>(
				"Kbits/s", true, SAMPLE_LAST, "Spot %d.Throughputs.L2_from_GW",
				spot_id);
			this->probe_sat_l2_from_gw.insert(
				std::pair<unsigned int, Probe<int> *>
				(spot_id, probe_l2_from_gw));
			this->l2_from_gw_bytes.insert(
				std::pair<unsigned int, int>(spot_id, 0));

			probe_l2_to_gw = Output::registerProbe<int>(
				"Kbits/s", true, SAMPLE_LAST, "Spot %d.Throughputs.L2_to_GW",
				spot_id);
			this->probe_sat_l2_to_gw.insert(
				std::pair<unsigned int, Probe<int> *>(spot_id, probe_l2_to_gw));
			this->l2_to_gw_bytes.insert(
				std::pair<unsigned int, int>(spot_id, 0));
		}
	}

	if(this->satellite_type == REGENERATIVE)
	{
		this->probe_sat_phy_output = Output::registerProbe<int>(
			"Total PHY output rate", "Kbits/s", true, SAMPLE_LAST);
	}
	else // Transparent mode
	{
		//does not work in transparent mode
		this->probe_sat_phy_output = Output::registerProbe<int>(
			"Total PHY output rate", "Kbits/s", false, SAMPLE_LAST);
	}

	this->probe_frame_interval = Output::registerProbe<float>(
		"Perf.Frames_interval", "ms", true, SAMPLE_LAST);

	return true;
}

bool BlockDvbSat::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	bool status = true;
	SpotMap::iterator spot;

	UTI_DEBUG_L3("DVB frame received from lower layer "
	             "(type = %d, len %zu)\n",
	             dvb_frame->getMessageType(),
	             dvb_frame->getTotalLength());

	switch(dvb_frame->getMessageType())
	{
	case MSG_TYPE_CORRUPTED:
		if(this->satellite_type == TRANSPARENT)
		{
			// in transparent scenario, satellite physical layer cannot corrupt
			UTI_DEBUG("the message was corrupted by physical layer, drop it");
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
			UTI_DEBUG("DVB-Frame received\n");

			// get the satellite spot from which the DVB frame comes from
			for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
			{
				SatSpot *current_spot = spot->second;

				if(current_spot->data_in_carrier_id == dvb_frame->getCarrierId())
				{
					// satellite spot found, forward DVB frame on the same spot

					// Update probes and stats
					this->l2_from_st_bytes[current_spot->getSpotId()] +=
						dvb_frame->getPayloadLength();
					this->phy_from_st_bytes[current_spot->getSpotId()] +=
						dvb_frame->getMessageLength();

					// TODO: forward according to a table
					UTI_DEBUG("DVB burst comes from spot %u (carrier %u) => "
					          "forward it to spot %u (carrier %u)\n",
					          current_spot->getSpotId(),
					          current_spot->data_in_carrier_id,
					          current_spot->getSpotId(),
					          current_spot->data_out_gw_fifo.getCarrierId());

					if(!this->onForwardFrame(&current_spot->data_out_gw_fifo,
					                         dvb_frame,
					                         this->getCurrentTime(),
					                         this->m_delay))
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
				// satellite spot found, forward DVB frame on the same spot
				SatSpot *current_spot = spot->second;

				if(current_spot->data_in_carrier_id == dvb_frame->getCarrierId())
				{
					this->l2_from_st_bytes[current_spot->getSpotId()] +=
						dvb_frame->getPayloadLength();
					this->phy_from_st_bytes[current_spot->getSpotId()] +=
						dvb_frame->getMessageLength();
				}
			}

			if(this->with_phy_layer && this->satellite_type == REGENERATIVE &&
			   this->receptionStd->getType() == "DVB-RCS")
			{
				DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
				tal_id_t tal_id;
				// decode the first packet in frame to be able to get source terminal ID
				if(!this->up_return_pkt_hdl->getSrc(frame->getPayload(), tal_id))
				{
					UTI_ERROR("unable to read source terminal ID in frame, "
					          "won't be able to update C/N value\n");
				}
				else
				{
					this->cni[tal_id] = frame->getCn();
				}
			}

			if(!this->receptionStd->onRcvFrame(dvb_frame,
			                                   0 /* no used */, &burst))
			{
				UTI_ERROR("failed to handle received DVB frame "
				          "(regenerative satellite)\n");
				status = false;
			}

			if(burst && !this->SendNewMsgToUpperLayer(burst))
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				status = false;
			}
		}
	}
	break;

	/* forward the BB frame (and the burst that the frame contains) */
	// TODO see if we can factorize
	case MSG_TYPE_BBFRAME:
	{
		/* we should not receive BB frame in regenerative mode */
		assert(this->satellite_type == TRANSPARENT);

		UTI_DEBUG("BBFrame received\n");

		// get the satellite spot from which the DVB frame comes from
		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			SatSpot *current_spot = spot->second;

			if(current_spot->data_in_carrier_id == dvb_frame->getCarrierId())
			{
				// satellite spot found, forward BBframe on the same spot

				// Update probes and stats
				this->l2_from_gw_bytes[current_spot->getSpotId()] +=
					dvb_frame->getPayloadLength();
				this->phy_from_gw_bytes[current_spot->getSpotId()] +=
					dvb_frame->getMessageLength();

				// TODO: forward according to a table
				UTI_DEBUG("BBFRAME burst comes from spot %u (carrier %u) => "
				          "forward it to spot %u (carrier %u)\n",
				          current_spot->getSpotId(),
				          current_spot->data_in_carrier_id,
				          current_spot->getSpotId(),
				          current_spot->data_out_st_fifo.getCarrierId());

				if(!this->onForwardFrame(
				          &current_spot->data_out_st_fifo,
				          dvb_frame,
				          this->getCurrentTime(),
				          this->m_delay))
				{
					UTI_ERROR("cannot forward burst\n");
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

			double cni;
			tal_id_t tal_id;

			tal_id = sac->getTerminalId();
			cni = sac->getCni();
			UTI_DEBUG("Get SAC from ST%u, with C/N0 = %.2f\n",
			          tal_id, cni);
			this->fwd_fmt_simu.setRequiredModcod(tal_id, cni);
			// update ACM parameters with uplink value, thus the GW will
			// known uplink C/N and thus update uplink MODCOD
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
	{
		UTI_DEBUG_L3("control frame (type = %u) received, "
		             "forward it on all satellite spots\n",
		             dvb_frame->getMessageType());

		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			DvbFrame *dvb_frame_copy = new DvbFrame(dvb_frame);

			// forward the frame copy
			if(!this->forwardDvbFrame(&spot->second->control_fifo,
			                          dvb_frame_copy))
			{
				status = false;
			}
		}
		delete dvb_frame;
	}
	break;

	// Special case of logon frame with dedicated channel
	case MSG_TYPE_SESSION_LOGON_REQ:
	{
		UTI_DEBUG("ST logon request received, "
		          "forward it on all satellite spots\n");

		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			DvbFrame *dvb_frame_copy = new DvbFrame(dvb_frame);

			// forward the frame copy
			if(!this->forwardDvbFrame(&spot->second->logon_fifo,
			                          dvb_frame_copy))
			{
				status = false;
			}
		}
		delete dvb_frame;
	}
	break;

	default:
	{
		UTI_ERROR("unknown type (%u) of DVB frame\n", dvb_frame->getMessageType());
		delete dvb_frame;
	}
	break;
	}

	return status;
}


/**
 * @brief Send Signalling frame for one specific carrier on the medium
 *        if DVB Messages in the fifo are ready to be sent
 *
 * @param sig_fifo    a pointer to the associated fifo being flushed
 * @return true on success, false otherwise
 */
bool BlockDvbSat::sendSigFrames(DvbFifo *sig_fifo)
{
	long max_cells;
	long time_of_sig;
	MacFifoElement *elem;
	int i;
	unsigned int carrier_id;

	carrier_id = sig_fifo->getCarrierId();

	// Get the maximum frames to send
	max_cells = sig_fifo->getCurrentSize();

	UTI_DEBUG_L3("send at most %ld signalling frames on satellite spot\n",
	             max_cells);

	// Retrieve the reference time in order to stop the algorithm
	// upon finding the first cell not ready
	time_of_sig = getCurrentTime();

	// Now send signaling frames up to max_cells
	for(i = 0; i < max_cells; i++)
	{
		DvbFrame *dvb_frame;

		// We must sent till we encounter a postdated frame
		if(sig_fifo->getTickOut() > time_of_sig)
		{
			UTI_DEBUG_L3("MAC FIFO element %d and following are not ready, "
			             "stop here\n", i + 1);
			break;
		}
		elem = sig_fifo->pop();
		assert(elem != NULL);

		if(elem->getType() != 0)
		{
			UTI_ERROR("MAC FIFO element type not corresponds to the signalling type\n");
			goto release_fifo_elem;
		}

		// The next cell can be sent, we ensure the necessary conditions to do it
		// Reminder: DVB frame is ready to be sent (carrier id already set)
		dvb_frame = elem->getFrame();
		if(!this->sendDvbFrame(dvb_frame, carrier_id))
		{
			UTI_ERROR("sendDvbFrame() failed, buffers preserved\n");
			goto release_fifo_elem;
		}

		// We succeeded in building and sending the frame we can release ressources
		delete elem;

		UTI_DEBUG_L3("sig msg sent (i = %d), fifo_id = %u, "
		             "carrier_id = %u", i, sig_fifo->getCarrierId(),
		             carrier_id);
	}

	return true;

release_fifo_elem:
	delete elem;
	return false;
}


void BlockDvbSat::updateStats()
{
	// Update stats and probes

	SpotMap::iterator spot_it;
	for (spot_it = this->spots.begin(); spot_it != spots.end(); ++spot_it)
	{
		SatSpot* spot = (*spot_it).second;
		unsigned int spot_id = spot->getSpotId();
		// Queue sizes
		mac_fifo_stat_context_t output_gw_fifo_stat;
		mac_fifo_stat_context_t output_st_fifo_stat;
		spot->data_out_gw_fifo.getStatsCxt(output_gw_fifo_stat);
		spot->data_out_st_fifo.getStatsCxt(output_st_fifo_stat);
		this->probe_sat_output_gw_queue_size[spot_id]->put(
			output_gw_fifo_stat.current_pkt_nbr);
		this->probe_sat_output_gw_queue_size_kb[spot_id]->put(
			((int) output_gw_fifo_stat.current_length_bytes * 8 / 1000));

		this->probe_sat_output_st_queue_size[spot_id]->put(
			output_st_fifo_stat.current_pkt_nbr);
		this->probe_sat_output_st_queue_size_kb[spot_id]->put(
			((int) output_st_fifo_stat.current_length_bytes * 8 / 1000));

		// Throughputs
		// PHY from ST
		this->probe_sat_phy_from_st[spot_id]->put(
			this->phy_from_st_bytes[spot_id] * 8 / this->stats_period_ms);
		this->phy_from_st_bytes[spot_id] = 0;

		// L2 from ST
		this->probe_sat_l2_from_st[spot_id]->put(
			this->l2_from_st_bytes[spot_id] * 8 / this->stats_period_ms);
		this->l2_from_st_bytes[spot_id] = 0;

		// L2 to ST
		this->probe_sat_l2_to_st[spot_id]->put(
			((int) output_st_fifo_stat.out_length_bytes * 8 /
			this->stats_period_ms));

		if(this->satellite_type == TRANSPARENT)
		{
			// PHY from GW
			this->probe_sat_phy_from_gw[spot_id]->put(
				this->phy_from_gw_bytes[spot_id] * 8 / this->stats_period_ms);
			this->phy_from_gw_bytes[spot_id] = 0;

			// L2 from GW
			this->probe_sat_l2_from_gw[spot_id]->put(
				this->l2_from_gw_bytes[spot_id] * 8 / this->stats_period_ms);
			this->l2_from_gw_bytes[spot_id] = 0;

			// L2 to GW
			this->probe_sat_l2_to_gw[spot_id]->put(
				((int) output_gw_fifo_stat.out_length_bytes * 8 /
				this->stats_period_ms));
		}

	}

	// PHY to ST
	this->probe_sat_phy_output->put(
		this->phy_to_sat_bytes * 8 / this->stats_period_ms);
	this->phy_to_sat_bytes = 0;

	// Send probes
	Output::sendProbes();
}

/**
 * @brief Update the probes
 *
 * @param burst        The burst to send
 * @param fifo         The fifo where was the packets
 * @param stat_fifo    The fifo statistiques
 */
// TODO we should add probes here !!
void BlockDvbSat::getProbe(NetBurst burst, DvbFifo fifo, spot_stats_t stat_fifo)
{
	const char *FUNCNAME = "[BlockDvbSat::getProbe]";
	clock_t this_tick;          // stats
	long double rate;           // stats

#define TICKS_INTERVAL  (1*sysconf(_SC_CLK_TCK))
#define NOTICE_INTERVAL (60*sysconf(_SC_CLK_TCK))

	stat_fifo.sum_data += burst.bytes();
	this_tick = times(NULL);
	if(this_tick - stat_fifo.previous_tick > NOTICE_INTERVAL)
	{
		rate = (stat_fifo.sum_data * TICKS_INTERVAL)
		       / (1024 * (this_tick - stat_fifo.previous_tick));
		stat_fifo.sum_data = 0;
		stat_fifo.previous_tick = this_tick;
		UTI_NOTICE("%s carrier#%d  %Lf Kb/sec.\n", FUNCNAME,
		           fifo.getCarrierId(), rate);
	}

}

// TODO merge with onForwardFrame, or rather replace onForwardFrame with this !
bool BlockDvbSat::forwardDvbFrame(DvbFifo *sig_fifo, DvbFrame *dvb_frame)
{
	MacFifoElement *elem;
	long l_current;

	l_current = this->getCurrentTime();

	// Get a room with timestamp in fifo
	elem = new MacFifoElement(dvb_frame, l_current,
	                          l_current + this->getNextDelay());
	if(!elem)
	{
		UTI_ERROR("failed to create a MAC FIFO element, "
		          "drop the signalling frame\n");
		delete dvb_frame;
		goto error;
	}

	// Fill the delayed queue
	if(!sig_fifo->push(elem))
	{
		UTI_ERROR("signalling FIFO full, drop signalling frame\n");
		delete dvb_frame;
		delete elem;
		goto error;
	}

	UTI_DEBUG_L3("signalling frame stored (tick_in = %ld, tick_out = %ld)",
	             elem->getTickIn(), elem->getTickOut());

	return true;

error:
	return false;
}


bool BlockDvbSat::onSendFrames(DvbFifo *fifo, long current_time)
{
	MacFifoElement *elem;

	while(fifo->getTickOut() <= current_time &&
	      fifo->getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;

		elem = fifo->pop();
		assert(elem != NULL);

		// check that we got a DVB frame in the SAT cell
		if(elem->getType() != 0)
		{
			UTI_ERROR("FIFO element does not contain a DVB or BB frame\n");
			goto error;
		}
		dvb_frame = elem->getFrame();

		// create a message for the DVB frame
		if(!this->sendDvbFrame(dvb_frame, fifo->getCarrierId()))
		{
			UTI_ERROR("failed to send message, drop the DVB or BB frame\n");
			goto error;
		}

		UTI_DEBUG("burst sent with a size of %zu\n",
		          dvb_frame->getTotalLength());

		delete elem;
	}

	return true;

error:
	delete elem;
	return false;
}

bool BlockDvbSat::onForwardFrame(DvbFifo *data_fifo,
                                 DvbFrame *dvb_frame,
                                 long current_time,
                                 int fifo_delay)
{
	MacFifoElement *elem;

	// sanity check
	if(dvb_frame == NULL)
	{
		UTI_ERROR("invalid DVB burst to forward to carrier ID %d\n",
		          data_fifo->getCarrierId());
		goto error;
	}

	// get a room with timestamp in fifo
	elem = new MacFifoElement(dvb_frame, current_time,
	                          current_time + fifo_delay);
	if(elem == NULL)
	{
		UTI_ERROR("cannot allocate FIFO element, drop packet\n");
		goto error;
	}

	// fill the delayed queue
	if(!data_fifo->push(elem))
	{
		UTI_ERROR("fifo full, drop the DVB frame\n");
		goto release_elem;
	}

	UTI_DEBUG("DVB/BB frame stored in FIFO for carrier ID %d "
	          "(tick_in = %ld, tick_out = %ld)\n", data_fifo->getCarrierId(),
	          elem->getTickIn(), elem->getTickOut());

	return true;

release_elem:
	delete elem;
error:
	delete dvb_frame;
	return false;
}



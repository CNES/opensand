/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
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

#include "sat_emulator_err.h"
#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "GenericSwitch.h"

#include <opensand_rt/Rt.h>

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
	scenario_timer(-1)
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

	// release the emission and reception DVB standards
	if(this->emissionStd != NULL)
	{
		delete this->emissionStd;
	}
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}
}


bool BlockDvbSat::onUpwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
			// message from lower layer: dvb frame
			T_DVB_META *dvb_meta;
			long carrier_id;
			unsigned char *frame;
			int len;

			dvb_meta = (T_DVB_META *)((MessageEvent *)event)->getData();
			carrier_id = dvb_meta->carrier_id;
			frame = (unsigned char *) dvb_meta->hdr;
			len = ((MessageEvent *)event)->getLength();

			if(!this->onRcvDvbFrame(frame, len, carrier_id))
			{
				UTI_ERROR("failed to handle received DVB frame "
				          "(len %d)\n", len);
				return false;
			}

			delete dvb_meta;
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

			if(this->satellite_type != REGENERATIVE_SATELLITE)
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
				if(this->emissionStd->onRcvEncapPacket(*pkt_it,
				   &this->spots[spot_id]->m_dataOutStFifo,
				   this->getCurrentTime(),
				   this->m_delay) != 0)
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return/
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
				UTI_DEBUG_L3("frame timer expired, send DVB frames\n");

				// send frame for every satellite spot
				for(SpotMap::iterator i_spot = this->spots.begin();
				    i_spot != this->spots.end(); i_spot++)
				{
					SatSpot *current_spot;

					current_spot = i_spot->second;

					UTI_DEBUG_L3("send logon frames on satellite spot %u\n",
					             i_spot->first);
					this->sendSigFrames(&current_spot->m_logonFifo);

					UTI_DEBUG_L3("send control frames on satellite spot %u\n",
					             i_spot->first);
					this->sendSigFrames(&current_spot->m_ctrlFifo);

					if(this->satellite_type == TRANSPARENT_SATELLITE)
					{
						bool status = true;
						// note: be careful that the reception standard
						// is also used to send frames because the reception
						// standard toward ST is the emission standard
						// toward GW (this should be reworked)

						UTI_DEBUG_L3("send data frames on satellite spot %u\n",
						             i_spot->first);
						if(!this->onSendFrames(&current_spot->m_dataOutGwFifo,
						                       this->getCurrentTime()))
						{
							status = false;
						}
						if(!this->onSendFrames(&current_spot->m_dataOutStFifo,
						                       this->getCurrentTime()))
						{
							status = false;
						}
						if(!status)
						{
							return false;
						}
					}
					else // REGENERATIVE_SATELLITE
					{
						if(this->emissionStd->scheduleEncapPackets(
						   &current_spot->m_dataOutStFifo,
						   this->getCurrentTime(),
						   &current_spot->complete_dvb_frames) != 0)
						{
							UTI_ERROR("failed to schedule packets "
							          "for satellite spot %u "
							          "on regenerative satellite\n",
							          i_spot->first);
							return false;
						}

						if(!this->sendBursts(&current_spot->complete_dvb_frames,
						                     current_spot->m_dataOutStFifo.getId()))
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
				UTI_DEBUG_L3("MODCOD/DRA scenario timer expired\n");

				if(this->satellite_type == REGENERATIVE_SATELLITE &&
				   this->emissionStd->type() == "DVB-S2")
				{
					UTI_DEBUG_L3("update modcod table\n");
					if(!this->emissionStd->goNextStScenarioStep())
					{
						UTI_ERROR("failed to update MODCOD IDs\n");
						return false;
					}
				}
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

	if(this->satellite_type == REGENERATIVE_SATELLITE)
	{
		// create the emission standard
		this->emissionStd = new DvbS2Std(this->down_forward_pkt_hdl);
		if(this->emissionStd == NULL)
		{
			UTI_ERROR("failed to create the emission standard\n");
			goto error;
		}

		// create the reception standard
		this->receptionStd = new DvbRcsStd(this->up_return_pkt_hdl);
		if(this->receptionStd == NULL)
		{
			UTI_ERROR("failed to create the reception standard\n");
			goto release_emission;
		}
	}
	else
	{
		// the packet_handler will depend on the case so we cannot fix it here
		// create the emission standard
		this->emissionStd = new DvbS2Std();
		if(this->emissionStd == NULL)
		{
			UTI_ERROR("failed to create the emission standard\n");
			goto error;
		}

		// create the reception standard
		this->receptionStd = new DvbRcsStd();
		if(this->receptionStd == NULL)
		{
			UTI_ERROR("failed to create the reception standard\n");
			goto release_emission;
		}
	}

	return true;

release_emission:
	delete this->emissionStd;
error:
	return false;
}


bool BlockDvbSat::initErrorGenerator()
{
	string err_generator;

	// Load a precalculated data file or use a default generator
	if(globalConfig.getValue(SAT_DVB_SECTION, SAT_ERR_GENERATOR, err_generator))
	{
		UTI_INFO("Section %s, %s missing. No error generator used.\n",
		         SAT_DVB_SECTION, SAT_ERR_GENERATOR);
		this->m_useErrorGenerator = 0;
	}
	else if(err_generator == SAT_ERR_GENERATOR_NONE)
	{
		// No error generator
		UTI_INFO("No error generator used\n");
		this->m_useErrorGenerator = 0;
	}
	else if(err_generator == SAT_ERR_GENERATOR_DEFAULT)
	{
		int err_ber;
		int err_mean;
		int err_delta;

		// Get values for default error generator
		if(!globalConfig.getValue(SAT_DVB_SECTION, SAT_ERR_BER, err_ber))
		{
			err_ber = 9;
			UTI_INFO("Section %s, %s missing setting it to default: "
			         "BER = 10 - %d\n", SAT_DVB_SECTION,
			         SAT_ERR_BER, err_ber);
		}

		if(!globalConfig.getValue(SAT_DVB_SECTION, SAT_ERR_MEAN, err_mean))
		{
			err_mean = 50;
			UTI_INFO("Section %s, %s missing setting it to "
			         "default: burst mean length = %d\n",
			         SAT_DVB_SECTION, SAT_ERR_MEAN, err_mean);
		}

		if(!globalConfig.getValue(SAT_DVB_SECTION, SAT_ERR_DELTA, err_delta))
		{
			err_delta = 10;
			UTI_INFO("Section %s, %s missing setting it to "
			         "default: burst delta = %d\n",
			         SAT_DVB_SECTION, SAT_ERR_DELTA, err_delta);
		}

		UTI_INFO("setting error generator to: BER = 10 - %d, burst mean "
		         "length = %d, burst delta = %d\n", err_ber,
		         err_mean, err_delta);

		SE_set_err_param(err_ber, err_mean, err_delta);
		this->m_useErrorGenerator = 1;
	}
	else
	{
		int index;

		// Load associated file
		index = SE_init_error_generator_from_file((char *) err_generator.c_str());
		if(index > 0)
		{
			SE_set_error_generator(index);
			UTI_INFO("loaded error data file %s\n",
			         err_generator.c_str());
		}
		else
		{
			int err_ber = 9;
			int err_mean = 50;
			int err_delta = 10;

			UTI_INFO("cannot load error data file %s. Setting it to default: "
			         "BER = 10 - %d, burst mean length = %d, burst delta = %d\n",
			         err_generator.c_str(), err_ber, err_mean,
			         err_delta);
			SE_set_err_param(err_ber, err_mean, err_delta);
		}

		this->m_useErrorGenerator = 1;
	}

	return true;
}


// TODO call in Downward::onInit()
bool BlockDvbSat::initDownwardTimers()
{
	// set frame duration in emission standard
	this->emissionStd->setFrameDuration(this->frame_duration);

	// launch frame timer
	this->frame_timer = this->downward->addTimerEvent("dvb_frame_timer",
	                                                  this->frame_duration);

	if(this->satellite_type == REGENERATIVE_SATELLITE)
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
	unsigned int i;

	// no need for switch in non-regenerative mode
	if(this->satellite_type != REGENERATIVE_SATELLITE)
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
		uint8_t spot_id = 0;
		uint8_t tal_id = 0;

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
	ConfigurationList spot_list;
	ConfigurationList::iterator iter;

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
		long ctrl_id;
		long data_in_id;
		long data_out_gw_id;
		long data_out_st_id;
		long log_id;
		int ret;
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
		// get the data_in_id
		if(!globalConfig.getAttributeValue(iter, DATA_IN_ID, data_in_id))
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
		UTI_INFO("satellite spot %u: logon = %ld, control = %ld, "
		         "data out ST = %ld, data out GW = %ld\n",
		         spot_id, log_id, ctrl_id, data_out_st_id, data_out_gw_id);
		ret = new_spot->init(spot_id, log_id, ctrl_id,
		                     data_in_id, data_out_st_id, data_out_gw_id);
		if(ret != 0)
		{
			UTI_ERROR("failed to init the new satellite spot\n");
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
		if(!this->emissionStd->doSatelliteTerminalExist(tal_id))
		{
			if(!this->emissionStd->addSatelliteTerminal(tal_id, column_nbr))
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

	// get the parameters of the error generator
	if(!this->initErrorGenerator())
	{
		UTI_ERROR("failed to complete the error generator part of the "
		          "initialisation");
		goto error;
	}

	// load the modcod files (regenerative satellite only)
	if(this->satellite_type == REGENERATIVE_SATELLITE)
	{
		if(!this->initModcodFiles())
		{
			UTI_ERROR("failed to complete the modcod part of the "
 			          "initialisation");
			goto error;
		}
		// initialize the MODCOD scheme ID
		if(!this->emissionStd->goNextStScenarioStep())
		{
			UTI_ERROR("failed to initialize MODCOD scheme IDs\n");
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

	return true;

error:
	return false;
}


/**
 * Get the satellite type
 * @return the satellite type
 */
string BlockDvbSat::getSatelliteType()
{
	return this->satellite_type;
}


bool BlockDvbSat::onRcvDvbFrame(unsigned char *frame,
                                unsigned int length, 
                                long carrier_id)
{
	bool status = true;
	SpotMap::iterator spot;
	T_DVB_HDR *hdr;

	// Get msg header
	hdr = (T_DVB_HDR *) frame;

	UTI_DEBUG_L3("DVB frame received from lower layer "
	             "(type = %ld, len %d)\n", hdr->msg_type, length);

	switch(hdr->msg_type)
	{
	case MSG_TYPE_DVB_BURST:
	{
		/* the DVB frame contains a burst of packets:
		 *  - if the satellite is a regenerative one, forward the burst to the
		 *    encapsulation layer,
		 *  - if the satellite is a transparent one, forward DVB burst as the
		 *    other DVB frames.
		 */
		if(this->satellite_type == TRANSPARENT_SATELLITE)
		{
			T_DVB_ENCAP_BURST *dvb_burst; // DVB burst received from lower layer

			dvb_burst = (T_DVB_ENCAP_BURST *) frame;

			if(dvb_burst->pkt_type != this->up_return_pkt_hdl->getEtherType())
			{
				UTI_ERROR("Bad packet type (0x%.4x) in DVB burst (expecting 0x%.4x)\n",
				          dvb_burst->pkt_type, this->up_return_pkt_hdl->getEtherType());
				status = false;
			}
			UTI_DEBUG("%ld %s packets received\n", dvb_burst->qty_element,
			          this->up_return_pkt_hdl->getName().c_str());

			// get the satellite spot from which the DVB frame comes from
			for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
			{
				SatSpot *current_spot = spot->second;

				if(current_spot->m_dataInId == carrier_id)
				{
					// satellite spot found, forward DVB frame on the same spot
					// TODO: forward according to a table
					UTI_DEBUG("DVB burst comes from spot %ld (carrier %ld) => "
					          "forward it to spot %ld (carrier %d)\n",
					          current_spot->getSpotId(),
					          current_spot->m_dataInId,
					          current_spot->getSpotId(),
					          current_spot->m_dataOutGwFifo.getId());

					if(this->receptionStd->onForwardFrame(
					   &current_spot->m_dataOutGwFifo,
					   frame,
					   length,
					   this->getCurrentTime(),
					   this->m_delay) != 0)
					{
						status = false;
					}

					// satellite spot found, abort the search
					break;
				}
			}
		}
		else // else satellite_type == REGENERATIVE_SATELLITE
		{
			/* The satellite is a regenerative one and the DVB frame contains
			 * a burst:
			 *  - extract the packets from the DVB frame,
			 *  - find the destination spot ID for each packet
			 *  - create a burst of encapsulation packets (NetBurst object)
			 *    with all the packets extracted from the DVB frame,
			 *  - send the burst to the upper layer.
			 */

			NetBurst *burst;

			if(this->receptionStd->onRcvFrame((unsigned char *) frame,
			                                  length, hdr->msg_type,
			                                  0 /* no used */, &burst) < 0)
			{
				UTI_ERROR("failed to handle received DVB frame "
				          "(regenerative satellite)\n");
				status = false;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				status = false;
			}
		}
	}
	break;

	/* forward the BB frame (and the burst that the frame contains) */
	case MSG_TYPE_BBFRAME:
	{
		T_DVB_BBFRAME *bbframe;

		/* we should not receive BB frame in regenerative mode */
		assert(this->satellite_type == TRANSPARENT_SATELLITE);

		bbframe = (T_DVB_BBFRAME *) frame;

		if(bbframe->pkt_type != this->down_forward_pkt_hdl->getEtherType())
		{
			UTI_ERROR("Bad packet type (0x%.4x) in BBFrame (expecting 0x%.4x)\n",
			          bbframe->pkt_type, this->down_forward_pkt_hdl->getEtherType());
			status = false;
		}

		UTI_DEBUG("%d packets received\n", bbframe->dataLength);

		// get the satellite spot from which the DVB frame comes from
		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			SatSpot *current_spot = spot->second;

			if(current_spot->m_dataInId == carrier_id)
			{
				// satellite spot found, forward BBframe on the same spot
				// TODO: forward according to a table
				UTI_DEBUG("BBFRAME burst comes from spot %ld (carrier %ld) => "
				          "forward it to spot %ld (carrier %d)\n",
				          current_spot->getSpotId(),
				          current_spot->m_dataInId,
				          current_spot->getSpotId(),
				          current_spot->m_dataOutStFifo.getId());

				this->emissionStd->onForwardFrame(
				          &current_spot->m_dataOutStFifo,
				          frame,
				          length,
				          this->getCurrentTime(),
				          this->m_delay);
				// TODO: check return code !

				// satellite spot found, abort the search
				break;
			}
		}
	}
	break;

	// Generic control frames (CR, TBTP, etc)
	case MSG_TYPE_CR:
	case MSG_TYPE_SOF:
	case MSG_TYPE_TBTP:
	case MSG_TYPE_SYNC:
	case MSG_TYPE_SESSION_LOGON_RESP:
	{
		UTI_DEBUG_L3("control frame (type = %ld) received, "
		             "forward it on all satellite spots\n",
		             hdr->msg_type);

		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			char *frame_copy;

			// create a copy of the frame
			frame_copy = (char *)malloc(MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX);
			if(frame_copy == NULL)
			{
				UTI_ERROR("[1] cannot allocate frame, aborting on spot %u\n",
				          spot->first);
				continue;
			}
			memcpy(frame_copy, frame, length);

			// forward the frame copy
			if(!this->forwardDvbFrame(&spot->second->m_ctrlFifo,
			                          frame_copy, length))
			{
				status = false;
			}
		}
		free(frame);
	}
	break;

	// Special case of logon frame with dedicated channel
	case MSG_TYPE_SESSION_LOGON_REQ:
	{
		UTI_DEBUG("ST logon request received, "
		          "forward it on all satellite spots\n");

		for(spot = this->spots.begin(); spot != this->spots.end(); spot++)
		{
			char *frame_copy;

			// create a copy of the frame
			frame_copy = (char *)malloc(MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX);
			if(frame_copy == NULL)
			{
				UTI_ERROR("[2] cannot allocate frame, aborting on spot %u\n",
				          spot->first);
				continue;
			}
			memcpy(frame_copy, frame, length);

			// forward the frame copy
			if(this->forwardDvbFrame(&spot->second->m_logonFifo,
			                         frame_copy, length) == false)
			{
				status = false;
			}
		}
		free(frame);
	}
	break;

	case MSG_TYPE_CORRUPTED:
		UTI_INFO("the message was corrupted by physical layer, drop it");
		free(frame);
		break;

	default:
	{
		UTI_ERROR("unknown type (%ld) of DVB frame\n", hdr->msg_type);
		free(frame);
	}
	break;
	}

	return status;
}


/**
 * Send Signalling frame for one specific carrier on the medium
 * DVB Messages in the fifo are ready to be sent
 * @param sigFifo    a pointer to the associated fifo being flushed
 * @return 0 on succes , -1 on failure
 */
int BlockDvbSat::sendSigFrames(DvbFifo * sigFifo)
{
	const char *FUNCNAME = "[sendSigFrames]";
	long max_cells;
	long time_of_sig;
	MacFifoElement *elem;
	std::string name = "sendSigFrames";
	int i;
	long carrier_id;


	carrier_id = sigFifo->getId();

	// Get the maximum frames to send
	max_cells = sigFifo->getCurrentSize();

	UTI_DEBUG_L3("send at most %ld signalling frames on satellite spot\n",
	             max_cells);

	// Retrieve the reference time in order to stop the algorithm
	// upon finding the first cell not ready
	time_of_sig = getCurrentTime();

	// Now send signaling frames up to max_cells
	for(i = 0; i < max_cells; i++)
	{
		unsigned char *frame;
		long frame_len;

		// We must sent till we encounter a postdated frame
		if(sigFifo->getTickOut() > time_of_sig)
		{
			UTI_DEBUG_L3("MAC FIFO element %d and following are not ready, "
			             "stop here\n", i + 1);
			break;
		}
		elem = sigFifo->pop();
		assert(elem != NULL);

		if(elem->getType() != 0)
		{
			UTI_ERROR("MAC FIFO element type not corresponds to the signalling type\n");
			goto release_fifo_elem;
		}

		// The next cell can be sent, we ensure the necessary conditions to do it
		// Reminder: DVB frame is ready to be sent (carrier id already set)
		frame = elem->getData();
		frame_len = elem->getDataLength();
		if(!sendDvbFrame((T_DVB_HDR *) frame, carrier_id, frame_len))
		{
			UTI_ERROR("%s sendDvbFrame() failed, buffers preserved\n", FUNCNAME);
			goto release_fifo_elem;
		}

		// We succeeded in building and sending the frame we can release ressources
		delete elem;

		UTI_DEBUG_L3("sig msg sent (i = %d), fifo_id = %d, "
		             "carrier_id = %ld\n", i, sigFifo->getId(), carrier_id);
	}

	return 0;

release_fifo_elem:
	delete elem;
	return -1;
}


/**
 * @brief Update the probes
 *
 * @param burst        The burst to send
 * @param fifo         The fifo where was the packets
 * @param m_stat_fifo  TODO
 */
void BlockDvbSat::getProbe(NetBurst burst, DvbFifo fifo, sat_StatBloc m_stat_fifo)
{
	const char *FUNCNAME = "[BlockDvbSat::getProbe]";
	clock_t this_tick;          // stats
	long double rate;           // stats

#define TICKS_INTERVAL  (1*sysconf(_SC_CLK_TCK))
#define NOTICE_INTERVAL (60*sysconf(_SC_CLK_TCK))

	m_stat_fifo.sum_data += burst.bytes();
	this_tick = times(NULL);
	if(this_tick - m_stat_fifo.previous_tick > NOTICE_INTERVAL)
	{
		rate = (m_stat_fifo.sum_data * TICKS_INTERVAL)
		       / (1024 * (this_tick - m_stat_fifo.previous_tick));
		m_stat_fifo.sum_data = 0;
		m_stat_fifo.previous_tick = this_tick;
		UTI_NOTICE("%s carrier#%d  %Lf Kb/sec.\n", FUNCNAME,
		           fifo.getId(), rate);
	}

}


bool BlockDvbSat::forwardDvbFrame(DvbFifo *sigFifo, char *ip_buf, int i_len)
{
	MacFifoElement *elem;
	long l_current;

	l_current = this->getCurrentTime();

	// Get a room with timestamp in fifo
	elem = new MacFifoElement((unsigned char *)ip_buf, i_len, l_current,
	                          l_current + this->getNextDelay());
	if(!elem)
	{
		UTI_ERROR("failed to create a MAC FIFO element, "
		          "drop the signalling frame\n");
		free(ip_buf);
		goto error;
	}

	// Fill the delayed queue
	if(!sigFifo->push(elem))
	{
		UTI_ERROR("signalling FIFO full, drop signalling frame\n");
		delete elem;
		free(ip_buf);
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
		elem = fifo->pop();
		assert(elem != NULL);

		// check that we got a DVB frame in the SAT cell
		if(elem->getType() != 0)
		{
			UTI_ERROR("FIFO element does not contain a DVB or BB frame\n");
			goto release_frame;
		}

		// create a message for the DVB frame
		if(!this->sendDvbFrame((T_DVB_HDR *) elem->getData(), fifo->getId(),
		                        elem->getDataLength()))
		{
			UTI_ERROR("failed to send message, drop the DVB or BB frame\n");
			goto error;
		}

		UTI_DEBUG("burst sent with a size of %d\n", elem->getDataLength());

		delete elem;
	}

	return true;

release_frame:
	free(elem->getData());
error:
	delete elem;
	return false;
}

/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file bloc_dvb_rcs_sat.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "bloc_dvb_rcs_sat.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>

#include "sat_emulator_err.h"
#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "GenericSwitch.h"

// environment plane
#include "platine_env_plane/EnvironmentAgent_e.h"
extern T_ENV_AGENT EnvAgent;

// Logging configuration
#define DBG_PACKAGE PKG_DVB_RCS_SAT
#include "platine_conf/uti_debug.h"


// BlocDVBRcsSat ctor
BlocDVBRcsSat::BlocDVBRcsSat(mgl_blocmgr *blocmgr,
                             mgl_id fatherid,
                             const char *name,
                             std::map<std::string, EncapPlugin *> &encap_plug):
	BlocDvb(blocmgr, fatherid, name, encap_plug),
	spots()
{
	this->initOk = false;

	// superframes and frames
	this->m_frameTimer = -1;

	// DVB-RCS/S2 emulation
	this->emissionStd = NULL;
	this->receptionStd = NULL;
	this->scenario_timer = -1;
}


// BlocDVBRcsSat dtor
BlocDVBRcsSat::~BlocDVBRcsSat()
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


/**
 * @brief The event handler
 *
 * @param event  the received event to handle
 * @return       mgl_ok if the event was correctly handled, mgl_ko otherwise
 */
mgl_status BlocDVBRcsSat::onEvent(mgl_event *event)
{
	mgl_status status = mgl_ko;
	SpotMap::iterator i_spot;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->initOk)
		{
			UTI_ERROR("bloc already initialized, ignore init event\n");
		}
		else if(this->onInit() < 0)
		{
			UTI_ERROR("bloc initialization failed\n");
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT);
		}
		else
		{
			this->initOk = true;
			status = mgl_ok;
		}
	}
	else if(!this->initOk)
	{
		UTI_ERROR("DVB-RCS SAT bloc not initialized, "
		          "ignore non-init event\n");
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		// message received from another bloc
		status = mgl_ok;

		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer() &&
		   MGL_EVENT_MSG_IS_TYPE(event, msg_dvb))
		{
			// message from lower layer: dvb frame
			T_DVB_META *dvb_meta;
			long carrier_id;
			unsigned char *frame;
			int len;

			dvb_meta = (T_DVB_META *) MGL_EVENT_MSG_GET_BODY(event);
			carrier_id = dvb_meta->carrier_id;
			frame = (unsigned char *) dvb_meta->hdr;
			len = MGL_EVENT_MSG_GET_BODYLEN(event);

			status = this->onRcvDVBFrame(frame, len, carrier_id);
			if(status != mgl_ok)
			{
				UTI_ERROR("failed to handle received DVB frame "
				          "(len %d)\n", len);
			}

			// TODO: release frame ?
			g_memory_pool_dvb_rcs.release((char *) dvb_meta);
		}
		else if(this->satellite_type == REGENERATIVE_SATELLITE &&
		        MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getUpperLayer() &&
		        MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
		{
			// message from upper layer: burst of encapsulation packets
			NetBurst *burst;
			uint8_t spot_id;
			NetBurst::iterator pkt_it;

			burst = (NetBurst *) MGL_EVENT_MSG_GET_BODY(event);

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
					status = mgl_ko;
					break;
				}
				if(this->emissionStd->onRcvEncapPacket(*pkt_it,
				   &this->spots[spot_id]->m_dataOutStFifo,
				   this->getCurrentTime(),
				   this->m_delay) != 0)
				{
					// a problem occured => trace it but
					// carry on simulation
					UTI_ERROR("unable to store packet: see log file\n");
				}
				(*pkt_it)->addTrace(HERE());
			}

			// avoid deteleting packets when deleting burst
			burst->clear();

			delete burst;
		}
		else
		{
			UTI_ERROR("unknown message event received\n");
		}
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		// receive a timer event
		if(MGL_EVENT_TIMER_IS_TIMER(event, this->m_frameTimer))
		{
			UTI_DEBUG_L3("frame timer expired, send DVB frames\n");
			status = mgl_ok;

			// restart the timer
			setTimer(this->m_frameTimer, this->frame_duration);

			// send frame for every satellite spot
			for(i_spot = this->spots.begin();
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
					// note: be careful that the reception standard
					// is also used to send frames because the reception
					// standard toward ST is the emission standard
					// toward GW (this should be reworked)

					UTI_DEBUG_L3("send data frames on satellite spot %u\n",
					             i_spot->first);
					if(this->onSendFrames(&current_spot->m_dataOutGwFifo,
					                      this->getCurrentTime()) != 0)
					{
						status = mgl_ko;
					}
					if(this->onSendFrames(&current_spot->m_dataOutStFifo,
					                      this->getCurrentTime()) != 0)
					{
						status = mgl_ko;
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
						status = mgl_ko;
					}

					if(status != mgl_ko)
					{
						if(this->sendBursts(&current_spot->complete_dvb_frames,
						                    current_spot->m_dataOutStFifo.getId()) != 0)
						{
							UTI_ERROR("failed to build and send "
							          "DVB/BB frames "
							          "for satellite spot %u "
							          "on regenerative satellite\n",
							          i_spot->first);
							status = mgl_ko;
						}
					}
				}
			}
		}
		else if(MGL_EVENT_TIMER_IS_TIMER(event, this->scenario_timer))
		{
			status = mgl_ok;

			UTI_DEBUG_L3("MODCOD/DRA scenario timer expired\n");

			setTimer(this->scenario_timer, this->dvb_scenario_refresh);

			if(this->satellite_type == REGENERATIVE_SATELLITE &&
			   this->emissionStd->type() == "DVB-S2")
			{
				UTI_DEBUG_L3("update modcod table\n");
				if(!this->emissionStd->goNextStScenarioStep())
				{
					UTI_ERROR("failed to update MODCOD IDs\n");
					status = mgl_ko;
				}
			}
		}
		else
		{
			UTI_ERROR("unknown timer event received\n");
		}
	}
	else
	{
		UTI_ERROR("unknown event received\n");
	}

	return status;
}


/**
 * @brief Initialize the transmission mode
 *
 * @return  0 in case of success, 0 otherwise
 */
int BlocDVBRcsSat::initMode()
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
		// TODO modify that
		this->emissionStd->setTalId(-1);

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
		// TODO modify that
		this->emissionStd->setTalId(-1);

		// create the reception standard
		this->receptionStd = new DvbRcsStd();
		if(this->receptionStd == NULL)
		{
			UTI_ERROR("failed to create the reception standard\n");
			goto release_emission;
		}
	}

	return 0;

release_emission:
	delete this->emissionStd;
error:
	return -1;
}


/**
 * @brief Initialize the error generator
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsSat::initErrorGenerator()
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

	return 0;
}


/**
 * Read configuration for the different timers
 *
 * @return  0 on success, -1 otherwise
 */
int BlocDVBRcsSat::initTimers()
{
	// set frame duration in emission standard
	this->emissionStd->setFrameDuration(this->frame_duration);

	// launch frame timer
	this->setTimer(this->m_frameTimer, this->frame_duration);

	if(this->satellite_type == REGENERATIVE_SATELLITE)
	{
		// launch the timer in order to retrieve the modcods
		this->setTimer(this->scenario_timer, this->dvb_scenario_refresh);
	}

	return 0;
}


/**
 * Retrieves switching table entries
 *
 * @return  0 on success, -1 otherwise
 */
int BlocDVBRcsSat::initSwitchTable()
{
	ConfigurationList switch_list;
	ConfigurationList::iterator iter;
	GenericSwitch *generic_switch = new GenericSwitch();
	unsigned int i;

	// no need for switch in non-regenerative mode
	if(this->satellite_type != REGENERATIVE_SATELLITE)
	{
		return 0;
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


	return 0;

release_switch:
	delete generic_switch;
error:
	return -1;
}


/**
 * @brief Retrieve the spots description from configuration
 *
 * @return  0 on success, -1 otherwise
 */
int BlocDVBRcsSat::initSpots()
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

	return 0;

error:
	return -1;
}


/**
 * @brief Read configuration for the list of STs
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsSat::initStList()
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

	return 0;

error:
	return -1;
}


/**
 * Read configuration when receive the init event
 *
 * @return 0 on success, -1 otherwise
 */
int BlocDVBRcsSat::onInit()
{
	int val;
	int ret;

	// get the common parameters
	if(!this->initCommon())
	{
		UTI_ERROR("failed to complete the common part of the initialisation");
		goto error;
	}

	ret = this->initMode();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the mode part of the "
		          "initialisation");
		goto error;
	}

	// get the parameters of the error generator
	ret = this->initErrorGenerator();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the error generator part of the "
		          "initialisation");
		goto error;
	}

	// load the modcod files (regenerative satellite only)
	if(this->satellite_type == REGENERATIVE_SATELLITE)
	{
		if(this->initModcodFiles() != 0)
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

		if(this->initStList() != 0)
		{
			UTI_ERROR("failed to complete the ST part of the"
 			          "initialisation");
			goto error;
		}

		// initialize the satellite internal switch
		if(this->initSwitchTable() != 0)
		{
			UTI_ERROR("failed to complete the switch part of the "
			          "initialisation");
			goto error;
		}

	}

	// read the frame duration, the super frame duration
	// and the second duration
	ret = this->initTimers();
	if(ret != 0)
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
	if(this->initSpots() != 0)
	{
		UTI_ERROR("failed to complete the spots part of the "
		          "initialisation");
		goto error;
	}

	return 0;

error:
	return -1;
}


/**
 * Get the satellite type
 * @return the satellite type
 */
string BlocDVBRcsSat::getSatelliteType()
{
	return this->satellite_type;
}


/**
 * Called upon reception event it is another layer (below on event) of demultiplexing
 * Do the appropriate treatment according to the type of the DVB message
 *
 * @param frame      the DVB or BB frame to forward
 * @param length     the length (in bytes) of the frame
 * @param carrier_id the carrier id of the frame
 * @return           mgl_ok on success, mgl_ko otherwise
 */
mgl_status BlocDVBRcsSat::onRcvDVBFrame(unsigned char *frame, unsigned int length, long carrier_id)
{
	mgl_status status = mgl_ok;
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
				status = mgl_ko;
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
						status = mgl_ko;
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
				status = mgl_ko;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				status = mgl_ko;
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
			status = mgl_ko;
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
			frame_copy = g_memory_pool_dvb_rcs.get(HERE());
			if(frame_copy == NULL)
			{
				UTI_ERROR("[1] dvb_rcs memory pool error, aborting on spot %u\n",
				          spot->first);
				continue;
			}
			memcpy(frame_copy, frame, length);

			// forward the frame copy
			if(this->forwardDVBFrame(&spot->second->m_ctrlFifo,
			                         frame_copy, length) == mgl_ko)
			{
				status = mgl_ko;
			}
		}
		g_memory_pool_dvb_rcs.release((char *) frame);
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
			frame_copy = g_memory_pool_dvb_rcs.get(HERE());
			if(frame_copy == NULL)
			{
				UTI_ERROR("[2] dvb_rcs memory pool error, aborting on spot %u\n",
				          spot->first);
				continue;
			}
			memcpy(frame_copy, frame, length);

			// forward the frame copy
			if(this->forwardDVBFrame(&spot->second->m_logonFifo,
			                         frame_copy, length) == mgl_ko)
			{
				status = mgl_ko;
			}
		}
		g_memory_pool_dvb_rcs.release((char *) frame);
	}
	break;

	default:
	{
		UTI_ERROR("unknown type (%ld) of DVB frame\n", hdr->msg_type);
		g_memory_pool_dvb_rcs.release((char *) frame);
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
int BlocDVBRcsSat::sendSigFrames(dvb_fifo * sigFifo)
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
	max_cells = sigFifo->getCount();

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
		elem = (MacFifoElement *)sigFifo->get(0);
		elem->addTrace(HERE());
		if(elem->getTickOut() > time_of_sig)
		{
			UTI_DEBUG_L3("MAC FIFO element %d and following are not ready, "
			             "stop here\n", i + 1);
			break;
		}

		if(elem->getType() != 0)
		{
			UTI_ERROR("MAC FIFO element type not corresponds to the signalling type\n");
			sigFifo->remove();
			goto release_fifo_elem;
		}

		// The next cell can be sent, we ensure the necessary conditions to do it
		// Reminder: DVB frame is ready to be sent (carrier id already set)
		frame = elem->getData();
		frame_len = elem->getDataLength();
		if(!sendDvbFrame((T_DVB_HDR *) frame, carrier_id))
		{
			UTI_ERROR("%s sendDvbFrame() failed, buffers preserved\n", FUNCNAME);
			goto release_fifo_elem;
		}

		// We succeeded in building and sending the frame we can release ressources
		sigFifo->remove();
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
void BlocDVBRcsSat::getProbe(NetBurst burst, dvb_fifo fifo, sat_StatBloc m_stat_fifo)
{
	const char *FUNCNAME = "[BlocDVBRcsSat::getProbe]";
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


// TODO
#if 0
/**
 * @brief Introduce error on packet
 *
 * @param encap_packet   The packet
 */
void BlocDVBRcsSat::errorGenerator(NetPacket * encap_packet)
{
	const char *FUNCNAME = "[BlocDVBRcsSat::errorGenerator]";
	unsigned char *buf;
	unsigned int len;
	len = encap_packet->totalLength();
	buf = (unsigned char *) calloc(len, sizeof(unsigned char));

	if(buf == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for introducing errors\n",
		          FUNCNAME);
	}
	else
	{
		memcpy(buf, encap_packet->data().c_str(), len);
		if(SE_errors_buf((char *) buf, len))
		{
			// error introduced => replace the encapsulation packet
			if(encap_packet->name() == "MPEG")
			{
				delete encap_packet;
				encap_packet = new MpegPacket(buf, len);
			}
			else if(encap_packet->name() == "ATM")
			{
				delete encap_packet;
				encap_packet = new AtmCell(buf, len);
			}
			else if(encap_packet->name() == "GSE")
			{
				delete encap_packet;
				encap_packet = new GsePacket(buf, len);
			}
			else
			{
				delete encap_packet;
				encap_packet = NULL;
			}
		}
		free(buf);
	}
}
#endif


mgl_status BlocDVBRcsSat::forwardDVBFrame(dvb_fifo *sigFifo, char *ip_buf, int i_len)
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
		g_memory_pool_dvb_rcs.release(ip_buf);
		goto error;
	}

	// Fill the delayed queue
	if(sigFifo->append(elem) < 0)
	{
		UTI_ERROR("signalling FIFO full, drop signalling frame\n");
		delete elem;
		g_memory_pool_dvb_rcs.release(ip_buf);
		goto error;
	}

	UTI_DEBUG_L3("signalling frame stored (tick_in = %ld, tick_out = %ld)",
	             elem->getTickIn(), elem->getTickOut());

	return mgl_ok;

error:
	return mgl_ko;
}

/**
 * Send the DVB frames stored in the given MAC FIFO by
 * \ref PhysicStd::onForwardFrame
 *
 * @param fifo          the MAC fifo which contains the DVB frames to send
 * @param current_time  the current time
 * @return              0 if successful, -1 otherwise
 */
int BlocDVBRcsSat::onSendFrames(dvb_fifo *fifo, long current_time)
{
	MacFifoElement *elem;

	while((elem = (MacFifoElement *) fifo->get()) != NULL &&
	      elem->getTickOut() <= current_time)
	{
		// check that we got a DVB frame in the SAT cell
		if(elem->getType() != 0)
		{
			UTI_ERROR("FIFO element does not contain a DVB or BB frame\n");
			goto release_frame;
		}

		// create a message for the DVB frame
		if(!this->sendDvbFrame((T_DVB_HDR *) elem->getData(), fifo->getId()))
		{
			UTI_ERROR("failed to send message, drop the DVB or BB frame\n");
			goto error;
		}

		UTI_DEBUG("burst sent with a size of %d\n", elem->getDataLength());

		fifo->remove();
		delete elem;
	}

	return 0;

release_frame:
	g_memory_pool_dvb_rcs.release((char *)elem->getData());
error:
	fifo->remove();
	delete elem;
	return -1;
}

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
 * @file BlockDvbNcc.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS_NCC
#include <opensand_conf/uti_debug.h>


#include "BlockDvbNcc.h"

#include "DamaCtrlRcsLegacy.h"

#include "msg_dvb_rcs.h"
#include "DvbRcsStd.h"
#include "DvbS2Std.h"

#include <opensand_output/Output.h>

#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/times.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>


/**
 * Constructor
 */
BlockDvbNcc::BlockDvbNcc(const string &name):
	BlockDvb(name),
	NccPepInterface(),
	dama_ctrl(NULL),
	m_carrierIdDvbCtrl(-1),
	m_carrierIdSOF(-1),
	m_carrierIdData(-1),
	super_frame_counter(-1),
	frame_counter(0),
	frame_timer(-1),
	macId(GW_TAL_ID),
	complete_dvb_frames(),
	scenario_timer(-1),
	fmt_groups(),
	pep_cmd_apply_timer(-1),
	pepAllocDelay(-1),
	event_file(NULL),
	stat_file(NULL),
	simu_file(NULL),
	simulate(none_simu),
	simu_st(-1),
	simu_rt(-1),
	simu_cr(-1),
	simu_interval(-1),
	event_logon_req(NULL),
	event_logon_resp(NULL)
{
	this->incoming_size = 0;
	this->probe_incoming_throughput = NULL;
	Output::registerProbe<int>("Physical incoming throughput",
	                           "Kbits/s", true, SAMPLE_AVG);
}


/**
 * Destructor
 */
BlockDvbNcc::~BlockDvbNcc()
{
	if(this->dama_ctrl != NULL)
		delete this->dama_ctrl;
	if(this->emissionStd != NULL)
		delete this->emissionStd;
	if(this->receptionStd != NULL)
		delete this->receptionStd;

	this->complete_dvb_frames.clear();

	if(this->m_bbframe != NULL)
		delete this->m_bbframe;

	if(this->event_file != NULL)
	{
		fflush(this->event_file);
		fclose(this->event_file);
	}
	if(this->stat_file != NULL)
	{
		fflush(this->stat_file);
		fclose(this->stat_file);
	}
	if(this->simu_file != NULL)
	{
		fclose(this->simu_file);
	}
	this->data_dvb_fifo.flush();
	// delete FMT groups here because they may be present in many carriers
	for(fmt_groups_t::iterator it = this->fmt_groups.begin();
	    it != this->fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}
}


bool BlockDvbNcc::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			NetBurst *burst;
			NetBurst::iterator pkt_it;

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			UTI_DEBUG("SF#%ld: encapsulation burst received "
			          "(%d packet(s))\n", this->super_frame_counter,
			          burst->length());

			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end();
				pkt_it++)
			{
				UTI_DEBUG("SF#%ld: store one encapsulation "
				          "packet\n", this->super_frame_counter);

				if(this->emissionStd->onRcvEncapPacket(*pkt_it,
				                                        &this->data_dvb_fifo,
				                                        this->getCurrentTime(),
				                                        0) != 0)
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					UTI_ERROR("SF#%ld: unable to store received "
					          "encapsulation packet "
					          "(see previous errors)\n",
					          this->super_frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

				UTI_DEBUG("SF#%ld: encapsulation packet is "
				          "successfully stored\n",
				          this->super_frame_counter);
				this->incoming_size += (*pkt_it)->getTotalLength();
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;
		}
		break;

		case evt_timer:
			// receive the frame Timer event
			UTI_DEBUG_L3("timer event received on downward channel");
			if(*event == this->frame_timer)
			{
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

					// send Start Of Frame (SOF)
					this->sendSOF();

					// run the allocation algorithms (DAMA)
					this->dama_ctrl->runOnSuperFrameChange(this->super_frame_counter);

					// send TTP computed by DAMA
					this->sendTTP();
				}

				// schedule encapsulation packets
				if(this->emissionStd->scheduleEncapPackets(&this->data_dvb_fifo,
				                                           this->getCurrentTime(),
				                                           &this->complete_dvb_frames) != 0)
				{
					UTI_ERROR("failed to schedule encapsulation "
							  "packets stored in DVB FIFO\n");
					return false;
				}

				if(!this->sendBursts(&this->complete_dvb_frames,
				                     this->m_carrierIdData))
				{
					UTI_ERROR("failed to build and send DVB/BB frames\n");
					return false;
				}

				this->updateStatsOnFrame();
			}
			else if(*event == this->scenario_timer)
			{
				// it's time to update MODCOD IDs
				UTI_DEBUG_L3("MODCOD scenario timer received\n");

				if(!this->fmt_simu.goNextScenarioStep())
				{
					UTI_ERROR("SF#%ld: failed to update MODCOD IDs\n",
							  this->super_frame_counter);
				}
				else
				{
					UTI_DEBUG_L3("SF#%ld: MODCOD IDs "
								 "successfully updated\n",
								 this->super_frame_counter);
				}
				// for each terminal in DamaCtrl update FMT
				this->dama_ctrl->updateFmt();
			}
			else if(*event == this->pep_cmd_apply_timer)
			{
				// it is time to apply the command sent by the external
				// PEP component

				PepRequest *pep_request;

				UTI_INFO("apply PEP requests now\n");
				while((pep_request = this->getNextPepRequest()) != NULL)
				{
					if(this->dama_ctrl->applyPepCommand(pep_request))
					{
						UTI_INFO("PEP request successfully "
						         "applied in DAMA\n");
					}
					else
					{
						UTI_ERROR("failed to apply PEP request "
						          "in DAMA\n");
						return false;
					}
				}
			}
			else
			{
				UTI_ERROR("unknown timer event received %s\n",
				          event->getName().c_str());
				return false;
			}
			break;

		case evt_net_socket:
			if(*event == this->getPepListenSocket())
			{
				int ret;

				// event received on PEP listen socket
				UTI_INFO("event received on PEP listen socket\n");

				// create the client socket to receive messages
				ret = acceptPepConnection();
				if(ret == 0)
				{
					UTI_INFO("NCC is now connected to PEP\n");
					// add a fd to handle events on the client socket
					this->downward->addNetSocketEvent("pep_client",
					                                  this->getPepClientSocket(),
					                                  200);
				}
				else if(ret == -1)
				{
					UTI_NOTICE("failed to accept new connection "
					           "request from PEP\n");
				}
				else if(ret == -2)
				{
					UTI_NOTICE("one PEP already connected: "
					           "reject new connection request\n");
				}
				else
				{
					UTI_ERROR("unknown status %d from "
					          "acceptPepConnection()\n", ret);
					return false;
				}
			}
			else if(*event == this->getPepClientSocket())
			{
				// event received on PEP client socket
				UTI_INFO("event received on PEP client socket\n");

				// read the message sent by PEP or delete socket
				// if connection is dead
				if(this->readPepMessage((NetSocketEvent *)event) == true)
				{
					// we have received a set of commands from the
					// PEP component, let's apply the resources
					// allocations/releases they contain

					// set delay for applying the commands
					if(this->getPepRequestType() == PEP_REQUEST_ALLOCATION)
					{
						if(!this->downward->startTimer(this->pep_cmd_apply_timer))
						{
							UTI_ERROR("cannot start pep timer");
							return false;
						}
						UTI_INFO("PEP Allocation request, apply a %dms delay\n", pepAllocDelay);
					}
					else if(this->getPepRequestType() == PEP_REQUEST_RELEASE)
					{
						// TODO find a way to raise timer directly (or update timer)
						this->downward->startTimer(this->pep_cmd_apply_timer);
						UTI_INFO("PEP Release request, no delay to apply\n");
					}
					else
					{
						UTI_ERROR("cannot determine request type!\n");
						return false;
					}
				}
				else
				{
					UTI_NOTICE("network problem encountered with PEP, "
					           "connection was therefore closed\n");
					this->downward->removeEvent(this->pep_cmd_apply_timer);
					return false;
				}
			}
		default:
			UTI_ERROR("unknown event received %s", event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockDvbNcc::onUpwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// messages from lower layer: dvb frames
			T_DVB_META *dvb_meta;
			//long carrier_id;
			unsigned char *frame;
			int l_len;

			dvb_meta = (T_DVB_META *)((MessageEvent *)event)->getData();
			//carrier_id = dvb_meta->carrier_id;
			frame = (unsigned char *) dvb_meta->hdr;
			l_len = ((MessageEvent *)event)->getLength();

			UTI_DEBUG("[onEvent] DVB frame received\n");
			if(!this->onRcvDvbFrame(frame, l_len))
			{
				free(dvb_meta);
				return false;
			}
			delete dvb_meta;
		}
		break;

		case evt_timer:
			if(*event == this->simu_timer)
			{
				switch(this->simulate)
				{
					case file_simu:
						if(!this->simulateFile())
						{
							UTI_ERROR("file simulation failed");
							fclose(this->simu_file);
							this->simu_file = NULL;
							this->simulate = none_simu;
							this->downward->removeEvent(this->simu_timer);
						}
						break;
					case random_simu:
						this->simulateRandom();
						break;
					default:
						break;
				}
				// flush files
				fflush(this->stat_file);
				fflush(this->event_file);
			}
			else
			{
				UTI_ERROR("unknown timer event received %s\n",
				          event->getName().c_str());
				return false;
			}
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}
	return true;
}


bool BlockDvbNcc::onInit()
{
	long simu_column_num;
	T_LINK_UP *link_is_up;

	// get the common parameters
	if(!this->initCommon())
	{
		UTI_ERROR("failed to complete the common part of the initialisation");
		goto error;
	}

	if(!this->initRequestSimulation())
	{
		UTI_ERROR("failed to complete the request simulation part of the "
		          "initialisation");
		goto error;
	}

	if(!this->initMode())
	{
		UTI_ERROR("failed to complete the mode part of the "
		          "initialisation");
		goto error;
	}

	// Get the carrier Ids
	if(!this->initCarrierIds())
	{
		UTI_ERROR("failed to complete the carrier IDs part of the "
		          "initialisation");
		goto error_mode;
	}

	// Get and open the files
	if(!this->initFiles())
	{
		UTI_ERROR("failed to complete the files part of the "
		          "initialisation");
		goto error_mode;
	}

	// get and launch the dama algorithm
	if(!this->initDama())
	{
		UTI_ERROR("failed to complete the DAMA part of the "
		          "initialisation");
		goto error_mode;
	}

	if(!this->initFifo())
	{
		UTI_ERROR("failed to complete the FIFO part of the "
		          "initialisation");
		goto release_dama;
	}

	if(!this->initOutput())
	{
		UTI_ERROR("failed to complete the initialization of statistics\n");
		goto release_dama;
	}

	// initialize the timers
	if(!this->initDownwardTimers())
	{
		UTI_ERROR("failed to complete the timers part of the "
		          "initialisation");
		goto release_dama;
	}

	// get the column number for GW in MODCOD simulation files
	if(!globalConfig.getValueInList(DVB_SIMU_COL, COLUMN_LIST,
	                                TAL_ID, toString(GW_TAL_ID),
                                    COLUMN_NBR, simu_column_num))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_SIMU_COL, COLUMN_LIST);
		goto release_dama;
	}
	if(simu_column_num <= 0)
	{
		UTI_ERROR("section '%s': invalid value %ld for parameter "
		          "'%s'\n", DVB_SIMU_COL, simu_column_num,
		          COLUMN_NBR);
		goto release_dama;
	}

	// declare the GW as one ST for the MODCOD scenarios
	if(!this->fmt_simu.addTerminal(GW_TAL_ID,
	                               simu_column_num))
	{
		UTI_ERROR("failed to define the GW as ST with ID %ld\n",
		          GW_TAL_ID);
		goto release_dama;
	}

	// allocate memory for the BB frame if DVB-S2 standard is used
	if(this->emissionStd->getType() == "DVB-S2" ||
	   this->receptionStd->getType() == "DVB-S2")
	{
		this->m_bbframe = new std::map<int, T_DVB_BBFRAME *>;
		if(this->m_bbframe == NULL)
		{
			UTI_ERROR("SF#%ld: failed to allocate memory for the BB "
			          "frame\n", this->super_frame_counter);
			goto release_dama;
		}
	}

	// create and send a "link is up" message to upper layer
	link_is_up = new T_LINK_UP;
	if(link_is_up == NULL)
	{
		UTI_ERROR("SF#%ld: failed to allocate memory for link_is_up "
		          "message\n", this->super_frame_counter);
		goto release_frames;
	}
	link_is_up->group_id = 0;
	link_is_up->tal_id = GW_TAL_ID;

	if(!this->sendUp((void **)(&link_is_up), sizeof(T_LINK_UP), msg_link_up))
	{
		UTI_ERROR("SF#%ld: failed to send link up message to upper layer",
		          this->super_frame_counter);
		delete link_is_up;
		goto release_frames;
	}
	UTI_DEBUG_L3("SF#%ld Link is up msg sent to upper layer\n",
	             this->super_frame_counter);

	// listen for connections from external PEP components
	if(!this->listenForPepConnections())
	{
		UTI_ERROR("failed to listen for PEP connections\n");
		goto release_frames;
	}
	this->downward->addNetSocketEvent("pep_listen", this->getPepListenSocket(), 200);

	// everything went fine
	return true;

release_frames:
	delete this->m_bbframe;
release_dama:
	delete this->dama_ctrl;
error_mode:
	delete this->emissionStd;
	delete this->receptionStd;
error:
	return false;
}


bool BlockDvbNcc::initRequestSimulation()
{
	string str_config;

	// Get and open the event file
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_EVENT_FILE, str_config))
	{
		UTI_ERROR("cannot load parameter %s from section %s\n",
		        DVB_EVENT_FILE, DVB_NCC_SECTION);
		goto error;
	}
	if(str_config ==  "stdout")
	{
		this->event_file = stdout;
	}
	else if(str_config == "stderr")
	{
		this->event_file = stderr;
	}
	else if(str_config != "none")
	{
		this->event_file = fopen(str_config.c_str(), "a");
		if(this->event_file == NULL)
		{
			UTI_ERROR("%s\n", strerror(errno));
		}
	}
	if(this->event_file == NULL && str_config != "none")
	{
		UTI_ERROR("no record file will be used for event\n");
	}
	else if(this->event_file != NULL)
	{
		UTI_INFO("events recorded in %s.\n", str_config.c_str());
	}

	// Get and open the stat file
	this->stat_file = NULL;
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_STAT_FILE, str_config))
	{
		UTI_ERROR("cannot load parameter %s from section %s\n",
		          DVB_STAT_FILE, DVB_NCC_SECTION);
		goto error;
	}
	if(str_config == "stdout")
	{
		this->stat_file = stdout;
	}
	else if(str_config == "stderr")
	{
		this->stat_file = stderr;
	}
	else if(str_config != "none")
	{
		this->stat_file = fopen(str_config.c_str(), "a");
		if(this->stat_file == NULL)
		{
			UTI_ERROR("%s\n", strerror(errno));
		}
	}
	if(this->stat_file == NULL && str_config != "none")
	{
		UTI_ERROR("no record file will be used for statistics\n");
	}
	else if(this->stat_file != NULL)
	{
		UTI_INFO("statistics recorded in %s.\n", str_config.c_str());
	}

	// Get and set simulation parameter
	//
	this->simulate = none_simu;
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIMU_MODE, str_config))
	{
		UTI_ERROR("cannot load parameter %s from section %s\n",
		          DVB_SIMU_MODE, DVB_NCC_SECTION);
		goto error;
	}

	if(str_config == "file")
	{
		if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIMU_FILE, str_config))
		{
			UTI_ERROR("cannot load parameter %s from section %s\n",
			          DVB_SIMU_FILE, DVB_NCC_SECTION);
			goto error;
		}
		if(str_config == "stdin")
		{
			this->simu_file = stdin;
		}
		else
		{
			this->simu_file = fopen(str_config.c_str(), "r");
		}
		if(this->simu_file == NULL && str_config != "none")
		{
			UTI_ERROR("%s\n", strerror(errno));
			UTI_ERROR("no simulation file will be used.\n");
		}
		else
		{
			UTI_INFO("events simulated from %s.\n",
			         str_config.c_str());
			this->simulate = file_simu;
			this->simu_timer = this->upward->addTimerEvent("simu_file",
			                                               this->frame_duration_ms);
		}
	}
	else if(str_config == "random")
	{
		int val;

		if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIMU_RANDOM, str_config))
		{
			UTI_ERROR("cannot load parameter %s from section %s\n",
			          DVB_SIMU_RANDOM, DVB_NCC_SECTION);
            goto error;
		}
		val = sscanf(str_config.c_str(), "%ld:%ld:%ld:%ld", &this->simu_st,
		             &this->simu_rt, &this->simu_cr, &this->simu_interval);
		if(val < 4)
		{
			UTI_ERROR("cannot load parameter %s from section %s\n",
			          DVB_SIMU_RANDOM, DVB_NCC_SECTION);
			goto error;
		}
		else
		{
			UTI_INFO("random events simulated for %ld terminals with "
			         "%ld kb/s bandwidth, a mean request of %ld kb/s and "
			         "a request amplitude of %ld kb/s)" ,
			         this->simu_st, this->simu_rt, this->simu_cr, this->simu_interval);
		}
		this->simulate = random_simu;
		this->simu_timer = this->upward->addTimerEvent("simu_random",
		                                               this->frame_duration_ms);
		srandom(times(NULL));
	}
	else
	{
		UTI_INFO("no event simulation\n");
	}

    return true;

error:
    return false;
}


bool BlockDvbNcc::initDownwardTimers()
{
	// TODO move in BlockDvbNcc::Downward::onInit
	int val;

	// read the pep allocation delay
	if(!globalConfig.getValue(NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY);
		goto error;
	}
	this->pepAllocDelay = val;
	UTI_INFO("pepAllocDelay set to %d ms\n", this->pepAllocDelay);
	// create timer
	this->pep_cmd_apply_timer = this->downward->addTimerEvent("pep_request",
	                                                          pepAllocDelay,
	                                                          false, // no rearm
	                                                          false // do not start
	                                                          );

	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	this->frame_timer = this->downward->addTimerEvent("frame",
	                                                  this->frame_duration_ms);

	// Launch the timer in order to retrieve the modcods
	this->scenario_timer = this->downward->addTimerEvent("scenario",
	                                                     this->dvb_scenario_refresh);

	return true;

error:
	return false;
}


bool BlockDvbNcc::initMode()
{
	freq_mhz_t bandwidth_mhz;
	double roll_off;

	// Get the value of the bandwidth
	if(!globalConfig.getValue(DOWN_FORWARD_BAND, BANDWIDTH,
	                          bandwidth_mhz))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DOWN_FORWARD_BAND, BANDWIDTH);
		goto error;
	}

	// Get the value of the roll off
	if(!globalConfig.getValue(DOWN_FORWARD_BAND, ROLL_OFF,
	                          roll_off))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DOWN_FORWARD_BAND, ROLL_OFF);
		goto error;
	}

	// initialize the emission and reception standards depending
	// on the satellite type
	// TODO new Scheduling !
	if(this->satellite_type == TRANSPARENT)
	{
		this->emissionStd = new DvbS2Std(this->down_forward_pkt_hdl,
		                                 &this->fmt_simu,
		                                 this->frame_duration_ms,
		                                 bandwidth_mhz * 1000);
		this->receptionStd = new DvbRcsStd(this->up_return_pkt_hdl);
	}
	else if(this->satellite_type == REGENERATIVE)
	{
		this->emissionStd = new DvbRcsStd(this->up_return_pkt_hdl);
		// no need to set fmt_simu, frame duration and bandwidth for reception
		this->receptionStd = new DvbS2Std(this->down_forward_pkt_hdl,
		                                  NULL, 0, 0);
	}
	else
	{
		UTI_ERROR("unknown value '%u' for satellite type ", this->satellite_type);
		goto error;

	}
	if(this->emissionStd == NULL)
	{
		UTI_ERROR("failed to create the emission standard\n");
		goto release_standards;
	}
	if(this->receptionStd == NULL)
	{
		UTI_ERROR("failed to create the reception standard\n");
		goto release_standards;
	}

	return true;

release_standards:
	if(this->emissionStd != NULL)
	  delete this->emissionStd;
	if(this->receptionStd != NULL)
	  delete this->receptionStd;
error:
	return false;
}


bool BlockDvbNcc::initCarrierIds()
{
	int val;

	// Get the carrier Id m_carrierIdDvbCtrl
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_CTRL_CAR, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_CTRL_CAR);
		goto error;
	}
	this->m_carrierIdDvbCtrl = val;
	UTI_INFO("carrierIdDvbCtrl set to %ld\n", this->m_carrierIdDvbCtrl);

	// Get the carrier Id m_carrierIdSOF
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SOF_CAR, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_SOF_CAR);
		goto error;
	}
	this->m_carrierIdSOF = val;
	UTI_INFO("carrierIdSOF set to %ld\n", this->m_carrierIdSOF);

	// Get the carrier Id m_carrierIdData
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_DATA_CAR, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_DATA_CAR);
	goto error;
	}
	this->m_carrierIdData = val;
	UTI_INFO("carrierIdData set to %ld\n", this->m_carrierIdData);

	return true;

error:
	return false;
}


bool BlockDvbNcc::initFiles()
{
	// we need up/return MODCOD simulation in these cases
	if((this->satellite_type == TRANSPARENT &&
	    this->receptionStd->getType() == "DVB-RCS") ||
	   (this->satellite_type == REGENERATIVE &&
	    this->emissionStd->getType() == "DVB-RCS"))
	{
		if(!this->initReturnModcodFiles())
		{
			UTI_ERROR("failed to initialize the up/return MODCOD files\n");
			goto error;
		}
	}

	// we need forward MODCOD emulation in this cases
	if((this->satellite_type == TRANSPARENT &&
	    this->emissionStd->getType() == "DVB-S2"))
	{
		if(!this->initForwardModcodFiles())
		{
			UTI_ERROR("failed to initialize the forward MODCOD files\n");
			goto error;
		}
	}

	// initialize the MODCOD IDs
	if(!this->fmt_simu.goNextScenarioStep())
	{
		UTI_ERROR("failed to initialize MODCOD scheme IDs\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvbNcc::initDama()
{
	string up_return_encap_proto;
	bool cra_decrease;
	rate_kbps_t max_rbdc_kbps;
	time_sf_t rbdc_timeout_sf;
	vol_pkt_t min_vbdc_pkt;
	rate_kbps_t fca_kbps;
	string dama_algo;

	double roll_off;
	freq_mhz_t bandwidth_mhz = 0;
	TerminalCategories categories;
	TerminalCategories::const_iterator cat_iter;
	TerminalMapping terminal_affectation;
	ConfigurationList conf_list;
	ConfigurationList aff_list;
	unsigned int carrier_id = 0;
	int i;
	string default_category_name;
	TerminalCategory *default_category;

	// Retrieving the cra decrease parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_CRA_DECREASE, cra_decrease))
	{
		UTI_ERROR("missing %s parameter", DC_CRA_DECREASE);
		goto error;
	}
	UTI_INFO("cra_decrease = %s\n", cra_decrease == true ? "true" : "false");

	// Retrieving the free capacity assignement parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_FREE_CAP, fca_kbps))
	{
		UTI_ERROR("missing %s parameter", DC_FREE_CAP);
		goto error;
	}
	UTI_INFO("fca = %d kb/s\n", fca_kbps);

	// Retrieving the max RBDC parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_MAX_RBDC, max_rbdc_kbps))
	{
		UTI_INFO("Missing %s parameter\n", DC_MAX_RBDC);
		return false;
	}
	UTI_INFO("RBDC max(kb/s): %d\n", max_rbdc_kbps);

	// Retrieving the rbdc timeout parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_RBDC_TIMEOUT, rbdc_timeout_sf))
	{
		UTI_ERROR("missing %s parameter", DC_RBDC_TIMEOUT);
		goto error;
	}
	UTI_INFO("rbdc_timeout = %d superframes\n", rbdc_timeout_sf);

	// Retrieving the min VBDC parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_MIN_VBDC, min_vbdc_pkt))
	{
		UTI_ERROR("missing %s parameter", DC_MIN_VBDC);
		goto error;
	}
	UTI_INFO("min_vbdc = %d packets\n", min_vbdc_pkt);

	// Get the value of the bandwidth for return link
	if(!globalConfig.getValue(UP_RETURN_BAND, BANDWIDTH,
	                          bandwidth_mhz))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          UP_RETURN_BAND, BANDWIDTH);
		goto error;
	}

	// Get the value of the roll off
	if(!globalConfig.getValue(UP_RETURN_BAND, ROLL_OFF,
	                          roll_off))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          UP_RETURN_BAND, ROLL_OFF);
		goto error;
	}

	// get the FMT groups
	if(!globalConfig.getListItems(UP_RETURN_BAND,
	                              FMT_GROUP_LIST,
	                              conf_list))
	{
		UTI_ERROR("Section %s, %s missing\n",
		          UP_RETURN_BAND, FMT_GROUP_LIST);
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
			UTI_ERROR("Problem retrieving %s in FMT groups\n",
			          GROUP_ID);
			goto error;
		}

		// Get FMT IDs
		if(!globalConfig.getAttributeValue(iter, FMT_ID, fmt_id))
		{
			UTI_ERROR("Problem retrieving %s in FMT groups\n", FMT_ID);
			goto error;
		}

		if(this->fmt_groups.find(group_id) != this->fmt_groups.end())
		{
			UTI_ERROR("Duplicated FMT group %u\n", group_id);
			goto error;
		}
		group = new FmtGroup(group_id, fmt_id);
		this->fmt_groups[group_id] = group;
	}

	conf_list.clear();
	// get the carriers distribution
	if(!globalConfig.getListItems(UP_RETURN_BAND, CARRIERS_DISTRI_LIST, conf_list))
	{
		UTI_ERROR("Section %s, %s missing\n",
		          UP_RETURN_BAND,
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
			UTI_ERROR("Problem retrieving %s in carriers distribution table "
			          "entry %u\n", CATEGORY, i);
			goto error;
		}

		// Get carriers' ratio
		if(!globalConfig.getAttributeValue(iter, RATIO, ratio))
		{
			UTI_ERROR("Problem retrieving %s in carriers distribution table "
			          "entry %u\n", RATIO, i);
			goto error;
		}

		// Get carriers' symbol ratge
		if(!globalConfig.getAttributeValue(iter, SYMBOL_RATE, symbol_rate_symps))
		{
			UTI_ERROR("Problem retrieving %s in carriers distribution table "
			          "entry %u\n", SYMBOL_RATE, i);
			goto error;
		}

		// Get carriers' FMT id
		if(!globalConfig.getAttributeValue(iter, FMT_GROUP, group_id))
		{
			UTI_ERROR("Problem retrieving %s in carriers distribution table "
			          "entry %u\n", FMT_GROUP, i);
			goto error;
		}

		// Get carriers' access type
		if(!globalConfig.getAttributeValue(iter, ACCESS_TYPE, access_type))
		{
			UTI_ERROR("Problem retrieving %s in carriers distribution table "
			          "entry %u\n", ACCESS_TYPE, i);
			goto error;
		}

		UTI_INFO("New carriers: category=%s, Rs=%G, FMT group=%u, "
		         "ratio=%G, access type=%s\n", name.c_str(),
		         symbol_rate_symps, group_id, ratio, access_type.c_str());
		if(access_type != "DAMA")
		{
			UTI_ERROR("%s access type is not supported\n", access_type.c_str());
			goto error;
		}

		group_it = this->fmt_groups.find(group_id);
		if(group_it == this->fmt_groups.end())
		{
			UTI_ERROR("No entry for FMT group with ID %u\n", group_id);
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
	if(!globalConfig.getValue(UP_RETURN_BAND, DEFAULT_AFF,
	                          default_category_name))
	{
		UTI_ERROR("Missing %s parameter\n", DEFAULT_AFF);
		goto error;
	}

	// Look for associated category
	default_category = NULL;
	cat_iter = categories.find(default_category_name);
	if(cat_iter != categories.end())
	{
		default_category = (*cat_iter).second;
	}
	if(default_category == NULL)
	{
		UTI_ERROR("Could not find categorie %s\n",
		          default_category_name.c_str());
		goto error;
	}
	UTI_INFO("ST default category: %s\n",
	         default_category->getLabel().c_str());

	// get the terminal affectations
	if(!globalConfig.getListItems(UP_RETURN_BAND, TAL_AFF_LIST, aff_list))
	{
		UTI_INFO("Missing %s parameter\n", TAL_AFF_LIST);
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
			UTI_ERROR("Problem retrieving %s in terminal affection table"
			          "entry %u\n", TAL_ID, i);
			goto error;
		}
		if(!globalConfig.getAttributeValue(iter, CATEGORY, name))
		{
			UTI_ERROR("Problem retrieving %s in terminal affection table"
			          "entry %u\n", CATEGORY, i);
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
			UTI_ERROR("Could not find category %s", name.c_str());
			goto error;
		}

		terminal_affectation[tal_id] = category;
	}

	// dama algorithm
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO,
	                          dama_algo))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	/* select the specified DAMA algorithm */
	// TODO create one DAMA per spot and add spot_id as param ?
	if(dama_algo == "Legacy")
	{
		UTI_INFO("creating Legacy DAMA controller\n");
		this->dama_ctrl = new DamaCtrlRcsLegacy();

	}
	else
	{
		UTI_ERROR("section '%s': bad value for parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	if(this->dama_ctrl == NULL)
	{
		UTI_ERROR("failed to create the DAMA controller\n");
		goto error;
	}

	// Initialize the DamaCtrl parent class
	if(!this->dama_ctrl->initParent(this->frame_duration_ms,
	                                this->frames_per_superframe,
	                                this->up_return_pkt_hdl->getFixedLength(),
	                                cra_decrease,
	                                max_rbdc_kbps,
	                                rbdc_timeout_sf,
	                                min_vbdc_pkt,
	                                fca_kbps,
	                                bandwidth_mhz * 1000,
	                                roll_off,
	                                categories,
	                                terminal_affectation,
	                                default_category,
	                                &this->fmt_simu))
	{
		UTI_ERROR("Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		UTI_ERROR("failed to initialize the DAMA controller\n");
		goto release_dama;
	}
	this->dama_ctrl->setRecordFile(this->event_file);

	return true;

release_dama:
	delete this->dama_ctrl;
error:
	return false;
}


bool BlockDvbNcc::initFifo()
{
	int val;

	// retrieve and set FIFO size
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIZE_FIFO, val))
	{
		UTI_ERROR("section '%s': bad value for parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_SIZE_FIFO);
		goto error;
	}
	this->data_dvb_fifo.init(m_carrierIdData, val, "GW_Fifo");

	return true;

error:
	return false;
}

bool BlockDvbNcc::initOutput(void)
{
	this->event_logon_req = Output::registerEvent("BlockDvbNCC:logon_request",
	                                              LEVEL_INFO);
	this->event_logon_resp = Output::registerEvent("BlockDvbNCC:logon_response",
	                                               LEVEL_INFO);
	// initialize probes
	this->probe_incoming_throughput =
		Output::registerProbe<float>("Physical incoming throughput",
		                             "Kbits/s", true, SAMPLE_AVG);

	return true;
}


/******************* EVENT MANAGEMENT *********************/


bool BlockDvbNcc::onRcvDvbFrame(unsigned char *data, int len)
{
	const char *FUNCNAME = DBG_PREFIX "[onRcvDVBFrame]";
	T_DVB_HDR *dvb_hdr;

	// get DVB header
	dvb_hdr = (T_DVB_HDR *) data;

	switch(dvb_hdr->msg_type)
	{
		// burst
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_BBFRAME:
		{
			// ignore BB frames in transparent scenario
			// (this is required because the GW may receive BB frames
			//  in transparent scenario due to carrier emulation)

			NetBurst *burst;

			if(this->receptionStd->getType() == "DVB-RCS" &&
			   dvb_hdr->msg_type == MSG_TYPE_BBFRAME)
			{
				UTI_DEBUG("ignore received BB frame in transparent scenario\n");
				goto drop;
			}
			if(this->receptionStd->onRcvFrame(data, len, dvb_hdr->msg_type,
			                                  this->macId, &burst) < 0)
			{
				UTI_ERROR("failed to handle DVB frame or BB frame\n");
				goto error;
			}
			if(burst && this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}
		}
		break;

		case MSG_TYPE_CR:
		{
			this->capacity_request.parse(data, len);

			UTI_DEBUG_L3("handle received Capacity Request (CR)\n");

			if(!this->dama_ctrl->hereIsCR(this->capacity_request))
			{
				UTI_ERROR("failed to handle Capacity Request "
				          "(CR) frame\n");
				goto error;
			}
			free(data);
		}
		break;

		case MSG_TYPE_SACT:
			UTI_DEBUG_L3("%s SACT\n", FUNCNAME);
			this->dama_ctrl->hereIsSACT(data, len);
			free(data);
			break;

		case MSG_TYPE_SESSION_LOGON_REQ:
			UTI_DEBUG("%s Logon Req\n", FUNCNAME);
			onRcvLogonReq(data, len);
			break;

		case MSG_TYPE_SESSION_LOGOFF:
			UTI_DEBUG_L3("%s Logoff Req\n", FUNCNAME);
			onRcvLogoffReq(data, len);
			break;

		case MSG_TYPE_TTP:
		case MSG_TYPE_SESSION_LOGON_RESP:
		case MSG_TYPE_SOF:
			// nothing to do in this case
			UTI_DEBUG_L3("ignore TTP, logon response or SOF frame "
			             "(type = %d)\n", dvb_hdr->msg_type);
			free(data);
			break;

		case MSG_TYPE_CORRUPTED:
			UTI_INFO("the message was corrupted by physical layer, drop it");
			free(data);
			break;

		default:
			UTI_ERROR("unknown type (%d) of DVB frame\n",
			          dvb_hdr->msg_type);
			free(data);
			break;
	}

	return true;

drop:
	free(data);
	return true;

error:
	UTI_ERROR("Treatments failed at SF# %ld\n",
	          this->super_frame_counter);
	return false;
}


/**
 * Send a start of frame
 */
void BlockDvbNcc::sendSOF()
{
	T_DVB_HDR *lp_ptr;
	long l_size;
	T_DVB_HDR *lp_hdr;
	T_DVB_SOF *lp_sof;


	// Get a dvb frame
	lp_ptr = (T_DVB_HDR *)calloc(MSG_DVB_RCS_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX,
	                             sizeof(unsigned char));
	if(!lp_ptr)
	{
		UTI_ERROR("[sendSOF] Failed to allocate memory for SoF\n");
		return;
	}

	// Set DVB header
	l_size = sizeof(T_DVB_SOF);
	lp_hdr = (T_DVB_HDR *) lp_ptr;
	lp_hdr->msg_length = l_size;
	lp_hdr->msg_type = MSG_TYPE_SOF;

	// Set frame number
	lp_sof = (T_DVB_SOF *) lp_ptr;
	lp_sof->frame_nr = this->super_frame_counter;

	// Send it
	if(!this->sendDvbFrame(lp_ptr, m_carrierIdSOF, l_size))
	{
		UTI_ERROR("[sendSOF] Failed to call sendDvbFrame()\n");
		delete lp_ptr;
		return;
	}

	UTI_DEBUG_L3("SF%ld: SOF sent\n", this->super_frame_counter);
}


void BlockDvbNcc::onRcvLogonReq(unsigned char *ip_buf, int l_len)
{
	T_DVB_LOGON_REQ *lp_logon_req;
	T_DVB_LOGON_RESP *lp_logon_resp;
	int l_size;
	std::list<long>::iterator list_it;

	lp_logon_req = (T_DVB_LOGON_REQ *) ip_buf;
	UTI_DEBUG("Logon request from %d\n", lp_logon_req->mac);

	// get context for this mac address
	// (could receive multiple logon request for same mac due to delay)

	// Sanity check of the buffer
	if(lp_logon_req->hdr.msg_type != MSG_TYPE_SESSION_LOGON_REQ)
	{
		UTI_ERROR("wrong packet data type (%d)\n",
		          lp_logon_req->hdr.msg_type);
		goto release;
	}

	// Sanity check of the length of the buffer
	// TODO use size_t
	if(lp_logon_req->hdr.msg_length > (unsigned)l_len)
	{
		UTI_ERROR("buffer len (%d) < msg_length (%d)\n",
		          l_len, lp_logon_req->hdr.msg_length);
		goto release;
	}

	// refuse to register a ST with same MAC ID as the NCC
	if(lp_logon_req->mac == this->macId)
	{
		UTI_ERROR("a ST wants to register with the MAC ID of the NCC "
		          "(%d), reject its request!\n", lp_logon_req->mac);
		goto release;
	}

	// send the corresponding event
	Output::sendEvent(event_logon_req, "Logon request received from %d",
	                  lp_logon_req->mac);

	// register the new ST
	if(this->fmt_simu.doTerminalExist(lp_logon_req->mac))
	{
		// ST already registered once
		UTI_ERROR("request to register ST with ID %d that is already "
		          "registered, resend logon response\n",
		          lp_logon_req->mac);
	}
	else
	{
		// ST was not registered yet
		UTI_INFO("register ST with MAC ID %d\n", lp_logon_req->mac);
		if(!this->fmt_simu.addTerminal(lp_logon_req->mac,
		                                lp_logon_req->nb_row))
		{
			UTI_ERROR("failed to register ST with MAC ID %d\n",
			          lp_logon_req->mac);
		}
	}

	// Get a dvb frame
	lp_logon_resp = (T_DVB_LOGON_RESP *)calloc(sizeof(T_DVB_LOGON_RESP),
	                                           sizeof(unsigned char));
	if(!lp_logon_resp)
	{
		UTI_ERROR("Failed to allocate memory for logon response\n");
		goto release;
	}

	// Inform the Dama controler (for its own context)
	if(this->dama_ctrl->hereIsLogon(*lp_logon_req))
	{
		// Set DVB header
		l_size = sizeof(T_DVB_LOGON_RESP);
		lp_logon_resp->hdr.msg_length = l_size;
		lp_logon_resp->hdr.msg_type = MSG_TYPE_SESSION_LOGON_RESP;
		lp_logon_resp->mac = lp_logon_req->mac;
		lp_logon_resp->nb_row = lp_logon_req->nb_row;
		lp_logon_resp->group_id = 0;
		lp_logon_resp->logon_id = lp_logon_req->mac;
		lp_logon_resp->return_vci = 0;
		lp_logon_resp->return_vpi = 0;
		lp_logon_resp->traffic_burst_type = 0;

		// Send it
		if(!sendDvbFrame((T_DVB_HDR *) lp_logon_resp, m_carrierIdDvbCtrl, l_size))
		{
			UTI_ERROR("Failed send message\n");
			goto release;
		}


		UTI_DEBUG_L3("SF%ld: logon response sent to lower layer\n",
		             this->super_frame_counter);


		// send the corresponding event
		Output::sendEvent(event_logon_resp, "Logon response send to %d",
		                  lp_logon_req->mac);

	}

release:
	free(ip_buf);
}

void BlockDvbNcc::onRcvLogoffReq(unsigned char *ip_buf, int l_len)
{
	T_DVB_LOGOFF *lp_logoff;
	std::list<long>::iterator list_it;

	lp_logoff = (T_DVB_LOGOFF *) ip_buf;

	// Packet type sanity check
	if(lp_logoff->hdr.msg_type != MSG_TYPE_SESSION_LOGOFF)
	{
		UTI_ERROR("wrong dvb packet type (%d)\n",
		          lp_logoff->hdr.msg_type);
		goto release;
	}

	// Length sanity check
	// TODO use size_t
	if(lp_logoff->hdr.msg_length > (unsigned)l_len)
	{
		UTI_ERROR("pkt length (%d) > buffer len (%d)\n",
		          lp_logoff->hdr.msg_length, l_len);
		goto release;
	}

	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->fmt_simu.delTerminal(lp_logoff->mac))
	{
		UTI_ERROR("failed to delete the ST with ID %d\n",
		          lp_logoff->mac);
		goto release;
	}

	this->dama_ctrl->hereIsLogoff(*lp_logoff);
	UTI_DEBUG_L3("SF%ld: logoff request from %d\n",
	             this->super_frame_counter, lp_logoff->mac);

release:
	free(ip_buf);
}

void BlockDvbNcc::sendTTP()
{
	unsigned char *frame;
	size_t length;

	// Build TTP
	if(!this->dama_ctrl->buildTTP(this->ttp))
	{
		UTI_DEBUG_L3("Dama didn't build TTP\bn");
		return;
	};

	// Get a dvb frame
	frame = (unsigned char *)calloc(MSG_DVB_RCS_SIZE_MAX,
	                                sizeof(unsigned char));
	if(!frame)
	{
		UTI_ERROR("failed to allocate DVB frame\n");
		return;
	}

	// Set DVB frame data
	if(!this->ttp.build(this->super_frame_counter, frame, length))
	{
		free(frame);
		UTI_ERROR("Failed to set TTP data\n");
		return;
	}
	if(!this->sendDvbFrame((T_DVB_HDR *) frame, m_carrierIdDvbCtrl, length))
	{
		free(frame);
		UTI_ERROR("Failed to send TTP\n");
		return;
	}

	UTI_DEBUG_L3("SF%ld: TTP sent\n", this->super_frame_counter);
}

bool BlockDvbNcc::simulateFile()
{
	static bool simu_eof = false;
	static char buffer[255] = "";
	static T_DVB_LOGON_REQ sim_logon_req;
	static T_DVB_LOGOFF sim_logoff;
	enum
	{ none, cr, logon, logoff } event_selected;

	int resul;
	long sf_nr;
	int st_id;
	long st_request;
	long st_rt;
	int cr_type;

	if(simu_eof)
	{
		UTI_DEBUG_L3("End of file.\n");
		goto error;
	}

	sf_nr = -1;
	while(sf_nr <= this->super_frame_counter)
	{
		if(4 ==
		   sscanf(buffer, "SF%ld CR st%d cr=%ld type=%d", &sf_nr, &st_id,
		   &st_request, &cr_type))
		{
			event_selected = cr;
		}
		else if(3 ==
		        sscanf(buffer, "SF%ld LOGON st%d rt=%ld", &sf_nr, &st_id, &st_rt))
		{
			event_selected = logon;
		}
		else if(2 == sscanf(buffer, "SF%ld LOGOFF st%d", &sf_nr, &st_id))
		{
			event_selected = logoff;
		}
		else
		{
			event_selected = none;
		}
		// TODO fix to avoid sending probe for the simulated ST
		//      remove once environment plane will be modified
		if(st_id <= 100)
		{
			st_id += 100;
		}
		if(event_selected == none)
			goto loop_step;
		if(sf_nr < this->super_frame_counter)
			goto loop_step;
		if(sf_nr > this->super_frame_counter)
			break;
		switch (event_selected)
		{
		case cr:
		{
			CapacityRequest *cr;

			cr = new CapacityRequest(st_id);

			cr->addRequest(0, cr_type, st_request);
			UTI_DEBUG("SF%ld: send a simulated CR of type %u with value = %ld "
			          "for ST %d\n", this->super_frame_counter,
			          cr_type, st_request, st_id);
			this->dama_ctrl->hereIsCR(*cr);
			delete cr;
			break;
		}
		case logon:
			sim_logon_req.hdr.msg_length = sizeof(T_DVB_LOGON_REQ);
			sim_logon_req.hdr.msg_type = MSG_TYPE_SESSION_LOGON_REQ;
			sim_logon_req.mac = st_id;
			sim_logon_req.rt_bandwidth = st_rt;
			UTI_DEBUG("SF%ld: send a simulated logon for ST %d\n",
			          this->super_frame_counter, st_id);
			this->dama_ctrl->hereIsLogon(sim_logon_req);
			break;
		case logoff:
			sim_logoff.hdr.msg_type = MSG_TYPE_SESSION_LOGOFF;
			sim_logoff.hdr.msg_length = sizeof(T_DVB_LOGOFF);
			sim_logoff.mac = st_id;
			UTI_DEBUG("SF%ld: send a simulated logoff for ST %d\n",
			          this->super_frame_counter, st_id);
			this->dama_ctrl->hereIsLogoff(sim_logoff);
			break;
		default:
			break;
		}
	 loop_step:
		resul = -1;
		while(resul < 1)
		{
			resul = fscanf(this->simu_file, "%254[^\n]\n", buffer);
			if(resul == 0)
			{
				int ret;
				// No conversion occured, we simply skip the line
				ret = fscanf(this->simu_file, "%*s");
				if ((ret == 0) || (ret == EOF))
				{
					goto error;
				}
			}
			UTI_DEBUG_L3("fscanf resul=%d: %s", resul, buffer);
			//fprintf (stderr, "frame %d\n", this->super_frame_counter);
			UTI_DEBUG_L3("frame %ld\n", this->super_frame_counter);
			if(resul == -1)
			{
				simu_eof = true;
				UTI_DEBUG_L3("End of file.\n");
				goto error;
			}
		}
	}

	return true;

 error:
	return false;
}


void BlockDvbNcc::simulateRandom()
{
	static bool initialized = false;
	static T_DVB_LOGON_REQ sim_logon_req;

	int i;

	if(!initialized)
	{
		for(i = 0; i < this->simu_st; i++)
		{
			sim_logon_req.hdr.msg_length = sizeof(T_DVB_LOGON_REQ);
			sim_logon_req.hdr.msg_type = MSG_TYPE_SESSION_LOGON_REQ;
			// BROADCAST_TAL_ID is maximum tal_id
			sim_logon_req.mac = BROADCAST_TAL_ID + i + 1;
			sim_logon_req.rt_bandwidth = this->simu_rt;
			this->dama_ctrl->hereIsLogon(sim_logon_req);
		}
		initialized = true;
	}

	for(i = 0; i < this->simu_st; i++)
	{
		CapacityRequest *cr;
		uint32_t val;

		cr = new CapacityRequest(BROADCAST_TAL_ID + i + 1);

		val = this->simu_cr - this->simu_interval / 2 +
		      random() % this->simu_interval;
		cr->addRequest(0, cr_rbdc, val);

		this->dama_ctrl->hereIsCR(*cr);
		delete cr;
	}
}

void BlockDvbNcc::updateStatsOnFrame()
{
	// TODO remove stats context
//	const dc_stat_context_t &dama_stat = this->dama_ctrl->getStatsContext()
//	DamaTerminalList::iterator it;

	// update custom DAMA statistics
	// TODO add this function or a reset function for stats in DAMA
//	this->dama_ctrl->updateStatisticsOnFrame();
	// update common DAMA statistics
/*	this->probe_incoming_throughput->put(this->incoming_size * 8 / this->frame_duration_ms);
	this->incoming_size = 0;
	this->probe_gw_rdbc_req_num->put(dama_stat.rbdc_requests_number);
	this->probe_gw_rdbc_req_capacity->put(dama_stat.rbdc_requests_sum_kbps);
	this->probe_gw_uplink_fair_share->put(dama_stat.fair_share);
	DC_RECORD_STAT("RBDC REQUEST NB %d", rbdc_request_number);
	DC_RECORD_STAT("RBDC REQUEST SUM %d kbits/s",
	               (int) Converter->
	               ConvertFromCellsPerFrameToKbits((double) rbdc_request_sum));
	DC_RECORD_STAT("VBDC REQUEST NB %d", vbdc_request_number);
	DC_RECORD_STAT("VBDC REQUEST SUM %d slot(s)", vbdc_request_sum);
	DC_RECORD_STAT("ALLOC RBDC %d kbits/s",
	               (int) Converter->
	               		ConvertFromCellsPerFrameToKbits((double) total_capacity -
	               		                                remaining_capacity));
	DC_RECORD_STAT("ALLOC VBDC %d kbits/s",
	               (int) Converter->
	               		ConvertFromCellsPerFrameToKbits((double) total_capacity -
	               		                                remaining_capacity));
	DC_RECORD_STAT("ALLOC FCA %d kbits/s",
	               (int) Converter->
	               		ConvertFromCellsPerFrameToKbits((double) total_capacity -
	               		                                remaining_capacity));


	// TODO
	terminal_number
	rbdc_requests_number
	vbdc_requests_number
	total_capacity
	total_cra_kbps
	rbdc_requests_sum_kbps
	vbdc_requests_sum_kb
	rbdc_allocation_kbps
	vbdc_allocation_kb
	max_rbdc_total_kbps
	fair_share
	//TODO + stats per ST
	// as long as the frame is changing, send all probes and event
	// sync environment plane
	Output::sendProbes();
	// TODO stat on real frame duration*/
}

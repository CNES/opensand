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


#include "BlockDvbNcc.h"

#include "DamaCtrlRcsLegacy.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Sof.h"
#include "ForwardSchedulingS2.h"
#include "UplinkSchedulingRcs.h"

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
	scheduling(NULL),
	frame_timer(-1),
	fwd_timer(-1),
	fwd_frame_counter(0),
	macId(GW_TAL_ID),
	complete_dvb_frames(),
	scenario_timer(-1),
	categories(),
	terminal_affectation(),
	fwd_fmt_groups(),
	ret_fmt_groups(),
	pep_cmd_apply_timer(-1),
	pepAllocDelay(-1),
	event_file(NULL),
	stat_file(NULL),
	simu_file(NULL),
	simulate(none_simu),
	simu_st(-1),
	simu_rt(-1),
	simu_max_rbdc(-1),
	simu_max_vbdc(-1),
	simu_cr(-1),
	simu_interval(-1),
	event_logon_req(NULL),
	event_logon_resp(NULL),
	probe_gw_l2_to_sat_before_sched(NULL),
	probe_gw_l2_to_sat_after_sched(NULL),
	probe_gw_l2_from_sat(NULL),
	probe_frame_interval(NULL),
	probe_gw_queue_size(NULL),
	probe_gw_queue_size_kb(NULL),
	probe_received_modcod(NULL),
	probe_rejected_modcod(NULL),
	probe_used_modcod(NULL)
{
	// TODO we need a mutex here because some parameters are used in upward and downward
	this->enableChannelMutex();
}


/**
 * Destructor
 */
BlockDvbNcc::~BlockDvbNcc()
{
	if(this->dama_ctrl)
		delete this->dama_ctrl;
	if(this->scheduling)
		delete this->scheduling;

	this->complete_dvb_frames.clear();

	if(this->event_file)
	{
		fflush(this->event_file);
		fclose(this->event_file);
	}
	if(this->stat_file)
	{
		fflush(this->stat_file);
		fclose(this->stat_file);
	}
	if(this->simu_file)
	{
		fclose(this->simu_file);
	}
	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->fwd_fmt_groups.begin();
	    it != this->fwd_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}
	for(fmt_groups_t::iterator it = this->ret_fmt_groups.begin();
	    it != this->ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	if(this->satellite_type == TRANSPARENT)
	{
		for(TerminalCategories::iterator it = this->categories.begin();
		    it != this->categories.end(); ++it)
		{
			delete (*it).second;
		}
		this->categories.clear();
	}
	// in regenerative mode categories is also owned and released by DAMA
	delete this->data_dvb_fifo;

	this->terminal_affectation.clear();

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

			Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
			                "SF#%u: encapsulation burst received "
			                "(%d packet(s))\n", this->super_frame_counter,
			                burst->length());

			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end(); ++pkt_it)
			{
				Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
				                "SF#%u: store one encapsulation "
				                "packet\n", this->super_frame_counter);

				if(!((DvbDownward *)this->downward)->onRcvEncapPacket(
										*pkt_it, this->data_dvb_fifo, 0))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
					                "SF#%u: unable to store received "
					                "encapsulation packet "
					                "(see previous errors)\n",
					                this->super_frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

				Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
				                "SF#%u: encapsulation packet is "
				                "successfully stored\n",
				                this->super_frame_counter);
				this->l2_to_sat_bytes_before_sched += (*pkt_it)->getTotalLength();
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;
		}
		break;

		case evt_timer:
			// receive the frame Timer event
			Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
			                "timer event received on downward channel");
			if(*event == this->frame_timer)
			{
				if(this->probe_frame_interval->isEnabled())
				{
					timeval time = event->getAndSetCustomTime();
					float val = time.tv_sec * 1000000L + time.tv_usec;
					this->probe_frame_interval->put(val/1000);
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

					// send Start Of Frame (SOF)
					this->sendSOF();

					if(this->with_phy_layer)
					{
						// for each terminal in DamaCtrl update FMT because in this case
						// this it not done with scenario timer and FMT is updated
						// each received frame but we only need it for allocation
						this->dama_ctrl->updateFmt();
					}

					// run the allocation algorithms (DAMA)
					this->dama_ctrl->runOnSuperFrameChange(this->super_frame_counter);

					// send TTP computed by DAMA
					this->sendTTP();
				}
			}
			else if(*event == this->fwd_timer)
			{
				uint32_t remaining_alloc_sym = 0;

				this->fwd_frame_counter++;

				// schedule encapsulation packets
				// TODO loop on categories (see todo in initMode)
				if(!this->scheduling->schedule(this->fwd_frame_counter,
				                               0,
				                               ((DvbDownward *)this->downward)->getCurrentTime(),
				                               &this->complete_dvb_frames,
				                               remaining_alloc_sym))
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
					                "failed to schedule encapsulation "
					                "packets stored in DVB FIFO\n");
					return false;
				}
				if(this->satellite_type == REGENERATIVE &&
				   this->complete_dvb_frames.size() > 0)
				{
					// we can do that because we have only one MODCOD per allocation
					// TODO this may change in the future...
					uint8_t modcod_id;
					modcod_id = 
						((DvbRcsFrame *)this->complete_dvb_frames.front())->getModcodId();
					this->probe_used_modcod->put(modcod_id);
				}
				Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
				                "SF#%u: frame %u: %u symbols remaining after "
				                "scheduling\n", this->super_frame_counter,
				                this->frame_counter, remaining_alloc_sym);
				if(!((DvbDownward *)this->downward)->sendBursts(&this->complete_dvb_frames,
				                                                this->data_carrier_id))
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
					                "failed to build and send DVB/BB "
					                "frames\n");
					return false;
				}
			}
			else if(*event == this->scenario_timer)
			{
				// if regenerative satellite and physical layer scenario,
				// send ACM parameters
				if(this->satellite_type == REGENERATIVE &&
				   this->with_phy_layer)
				{
					return this->sendAcmParameters();
				}

				// it's time to update MODCOD IDs
				Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
				                "MODCOD scenario timer received\n");

				if(!this->ret_fmt_simu.goNextScenarioStep(false) ||
				   !this->fwd_fmt_simu.goNextScenarioStep(true))
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
					                "SF#%u: failed to update MODCOD IDs\n",
					                this->super_frame_counter);
				}
				else
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
					                "SF#%u: MODCOD IDs successfully updated\n",
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

				Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
				                "apply PEP requests now\n");
				while((pep_request = this->getNextPepRequest()) != NULL)
				{
					if(this->dama_ctrl->applyPepCommand(pep_request))
					{
						Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
						                "PEP request successfully "
						                "applied in DAMA\n");
					}
					else
					{
						Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
						                "failed to apply PEP request "
						                "in DAMA\n");
						return false;
					}
				}
			}
			else if(*event == this->stats_timer)
			{
				this->updateStats();
			}
			else
			{
				Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
				                "unknown timer event received %s\n",
				                event->getName().c_str());
				return false;
			}
			break;

		case evt_net_socket:
			if(*event == this->getPepListenSocket())
			{
				int ret;

				// event received on PEP listen socket
				Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
				                "event received on PEP listen socket\n");

				// create the client socket to receive messages
				ret = acceptPepConnection();
				if(ret == 0)
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
					                "NCC is now connected to PEP\n");
					// add a fd to handle events on the client socket
					this->downward->addNetSocketEvent("pep_client",
					                                  this->getPepClientSocket(),
					                                  200);
				}
				else if(ret == -1)
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_WARNING,
					                "failed to accept new connection "
					                "request from PEP\n");
				}
				else if(ret == -2)
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_WARNING,
					                "one PEP already connected: "
					                "reject new connection request\n");
				}
				else
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
					                "unknown status %d from "
					                "acceptPepConnection()\n", ret);
					return false;
				}
			}
			else if(*event == this->getPepClientSocket())
			{
				// event received on PEP client socket
				Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
				                "event received on PEP client socket\n");

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
							Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
							                "cannot start pep timer");
							return false;
						}
						Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
						                "PEP Allocation request, apply a %dms"
						                " delay\n", pepAllocDelay);
					}
					else if(this->getPepRequestType() == PEP_REQUEST_RELEASE)
					{
						this->downward->raiseTimer(this->pep_cmd_apply_timer);
						Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
						                "PEP Release request, no delay to "
						                "apply\n");
					}
					else
					{
						Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
						                "cannot determine request type!\n");
						return false;
					}
				}
				else
				{
					Output::sendLog(this->log_rcv_from_down, LEVEL_WARNING,
					                "network problem encountered with PEP, "
					                "connection was therefore closed\n");
					this->downward->removeEvent(this->pep_cmd_apply_timer);
					return false;
				}
			}
		default:
			Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
			                "unknown event received %s",
			                event->getName().c_str());
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
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			Output::sendLog(this->log_rcv_from_up, LEVEL_INFO,
			                "[onEvent] DVB frame received\n");
			if(!this->onRcvDvbFrame(dvb_frame))
			{
				delete dvb_frame;
				return false;
			}
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
							Output::sendLog(this->log_rcv_from_up, LEVEL_ERROR,
							                "file simulation failed");
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
				Output::sendLog(this->log_rcv_from_up, LEVEL_ERROR,
				                "unknown timer event received %s\n",
				                 event->getName().c_str());
				return false;
			}
			break;

		default:
			Output::sendLog(this->log_rcv_from_up, LEVEL_ERROR,
			                "unknown event received %s",
			                event->getName().c_str());
			return false;
	}
	return true;
}


bool BlockDvbNcc::onInit()
{
	T_LINK_UP *link_is_up;

	// get the common parameters
	if(!this->initCommon())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the common part of the "
		                "initialisation");
		goto error;
	}

	if(!this->initRequestSimulation())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the request simulation part of "
		                "the initialisation");
		goto error;
	}

	// Get the carrier Ids
	if(!this->initCarrierIds())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the carrier IDs part of the "
		                "initialisation");
		goto error_mode;
	}

	if(!this->initFifo())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the FIFO part of the "
		                "initialisation");
		goto release_dama;
	}

	if(!this->initMode())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the mode part of the "
		                "initialisation");
		goto error;
	}

	// Get and open the files
	if(!this->initFiles())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the files part of the "
		                "initialisation");
		goto error_mode;
	}

	// get and launch the dama algorithm
	if(!this->initDama())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the DAMA part of the "
		                "initialisation");
		goto error_mode;
	}

	if(!this->initOutput())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the initialization of "
		                "statistics\n");
		goto release_dama;
	}

	// initialize the timers
	if(!this->initDownwardTimers())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to complete the timers part of the "
		                "initialisation");
		goto release_dama;
	}

	// initialize the column ID for FMT simulation
	if(!this->initColumns())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to initialize the columns ID for FMT "
		                "simulation\n");
		goto release_dama;
	}

	// create and send a "link is up" message to upper layer
	link_is_up = new T_LINK_UP;
	if(link_is_up == NULL)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "SF#%u: failed to allocate memory for link_is_up "
		                "message\n", this->super_frame_counter);
		goto release_dama;
	}
	link_is_up->group_id = 0;
	link_is_up->tal_id = GW_TAL_ID;

	if(!this->sendUp((void **)(&link_is_up), sizeof(T_LINK_UP), msg_link_up))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "SF#%u: failed to send link up message to upper layer",
		                this->super_frame_counter);
		delete link_is_up;
		goto release_dama;
	}
	Output::sendLog(this->log_init, LEVEL_DEBUG,
	                "SF#%u Link is up msg sent to upper layer\n",
	                this->super_frame_counter);

	// listen for connections from external PEP components
	if(!this->listenForPepConnections())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to listen for PEP connections\n");
		goto release_dama;
	}
	this->downward->addNetSocketEvent("pep_listen", this->getPepListenSocket(), 200);

	// everything went fine
	return true;

release_dama:
	delete this->dama_ctrl;
error_mode:
	delete ((DvbUpward *)this->upward)->receptionStd;
error:
	return false;
}


bool BlockDvbNcc::initRequestSimulation()
{
	string str_config;

	// Get and open the event file
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_EVENT_FILE, str_config))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot load parameter %s from section %s\n",
		                DVB_EVENT_FILE, DVB_NCC_SECTION);
		goto error;
	}
	if(str_config != "none" && this->with_phy_layer)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot use simulated request with physical layer "
		                "because we need to add cni parameters in SAC "
		                "(TBD!)\n");
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
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "%s\n", strerror(errno));
		}
	}
	if(this->event_file == NULL && str_config != "none")
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "no record file will be used for event\n");
	}
	else if(this->event_file != NULL)
	{
		Output::sendLog(this->log_init, LEVEL_NOTICE,
		                "events recorded in %s.\n", str_config.c_str());
	}

	// Get and open the stat file
	// TODO it would be better to register probes for simulated ST and
	//      use probes
	this->stat_file = NULL;
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_STAT_FILE, str_config))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot load parameter %s from section %s\n",
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
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "%s\n", strerror(errno));
		}
	}
	if(this->stat_file == NULL && str_config != "none")
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "no record file will be used for statistics\n");
	}
	else if(this->stat_file != NULL)
	{
		Output::sendLog(this->log_init, LEVEL_NOTICE,
		                "statistics recorded in %s.\n", str_config.c_str());
	}

	// Get and set simulation parameter
	//
	this->simulate = none_simu;
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIMU_MODE, str_config))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "cannot load parameter %s from section %s\n",
		                DVB_SIMU_MODE, DVB_NCC_SECTION);
		goto error;
	}

	// TODO is we use probes we need to register here so we need to known the number
	//      of terminals (easy in random mode, need parsing in file mode,
	//      may need a ST number parameter for stdin)
	// TODO for stdin use FileEvent for simu_timer ?
	if(str_config == "file")
	{
		if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIMU_FILE, str_config))
		{
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "cannot load parameter %s from section %s\n",
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
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "%s\n", strerror(errno));
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "no simulation file will be used.\n");
		}
		else
		{
			Output::sendLog(this->log_init, LEVEL_NOTICE,
			                "events simulated from %s.\n",
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
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "cannot load parameter %s from section %s\n",
			                DVB_SIMU_RANDOM, DVB_NCC_SECTION);
            goto error;
		}
		val = sscanf(str_config.c_str(), "%ld:%ld:%ld:%ld:%ld:%ld",
		             &this->simu_st, &this->simu_rt, &this->simu_max_rbdc,
		             &this->simu_max_vbdc, &this->simu_cr, &this->simu_interval);
		if(val < 4)
		{
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "cannot load parameter %s from section %s\n",
			                DVB_SIMU_RANDOM, DVB_NCC_SECTION);
			goto error;
		}
		else
		{
			Output::sendLog(this->log_init, LEVEL_NOTICE,
			                "random events simulated for %ld terminals with "
			                "%ld kb/s bandwidth, %ld kb/s max RBDC, "
			                "%ld kb max VBDC, a mean request of %ld kb/s "
			                "and a request amplitude of %ld kb/s)",
			                this->simu_st, this->simu_rt, this->simu_max_rbdc,
			                this->simu_max_vbdc, this->simu_cr,
			                this->simu_interval);
		}
		this->simulate = random_simu;
		this->simu_timer = this->upward->addTimerEvent("simu_random",
		                                               this->frame_duration_ms);
		srandom(times(NULL));
	}
	else
	{
		Output::sendLog(this->log_init, LEVEL_NOTICE,
		                "no event simulation\n");
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
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY);
		goto error;
	}
	this->pepAllocDelay = val;
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "pepAllocDelay set to %d ms\n", this->pepAllocDelay);
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
	this->fwd_timer = this->downward->addTimerEvent("fwd_timer",
	                                                this->fwd_timer_ms);

	// Launch the timer in order to retrieve the modcods if there is no physical layer
	// or to send SAC with ACM parameters in regenerative mode
	if(!this->with_phy_layer || this->satellite_type == REGENERATIVE)
	{
		this->scenario_timer = this->downward->addTimerEvent("scenario",
		                                                     this->dvb_scenario_refresh);
	}

	return true;

error:
	return false;
}

bool BlockDvbNcc::initColumns()
{
	int i = 0;
	ConfigurationList columns;
	ConfigurationList::iterator iter;

	// Get the list of STs
	if(!globalConfig.getListItems(SAT_SIMU_COL_SECTION, COLUMN_LIST,
	                              columns))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s, %s': problem retrieving simulation "
		                "column list\n", SAT_SIMU_COL_SECTION, COLUMN_LIST);
		goto error;
	}

	for(iter = columns.begin(); iter != columns.end(); iter++)
	{
		i++;
		uint16_t tal_id;
		uint16_t column_nbr;

		// Get the Tal ID
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "problem retrieving %s in simulation column "
			                "entry %d\n", TAL_ID, i);
			goto error;
		}
		// Get the column nbr
		if(!globalConfig.getAttributeValue(iter, COLUMN_NBR, column_nbr))
		{
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "problem retrieving %s in simulation column "
			                "entry %d\n", COLUMN_NBR, i);
			goto error;
		}

		this->column_list[tal_id] = column_nbr;
	}

	if(this->column_list.find(GW_TAL_ID) == this->column_list.end())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		        "GW is not declared in column IDs\n");
		goto error;
	}

	// declare the GW as one ST for the MODCOD scenarios
	// TODO check that we need both
	if(!this->ret_fmt_simu.addTerminal(GW_TAL_ID,
	                                   this->column_list[GW_TAL_ID]) ||
	   !this->fwd_fmt_simu.addTerminal(GW_TAL_ID,
	                                   this->column_list[GW_TAL_ID]))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to define the GW as ST with ID %ld\n",
		                GW_TAL_ID);
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvbNcc::initMode()
{
	// TODO remove that once data fifo will be a map
	fifos_t fifos;
	fifos[this->data_dvb_fifo->getCarrierId()] = this->data_dvb_fifo;

	// initialize the emission and reception standards and scheduling
	// depending on the satellite type
	if(this->satellite_type == TRANSPARENT)
	{
		if(!this->initBand(DOWN_FORWARD_BAND,
		                   this->fwd_timer_ms,
		                   this->categories,
		                   this->terminal_affectation,
		                   &this->default_category,
		                   this->fwd_fmt_groups))
		{
			return false;
		}

		if(this->categories.size() != 1)
		{
			// TODO at the moment we use only one category
			// To implement more than one category we will need to create one (a group of)
			// fifo(s) per category and schedule per (group of) fifo(s).
			// The packets would the pushed in the correct (group of) fifo(s) according to
			// the category the destination terminal ID belongs
			// this is why we have categories, terminal_affectation and default_category
			// as attributes
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "cannot support more than one category for "
			                "down/forward band\n");
			return false;
		}

		((DvbUpward *)this->upward)->receptionStd = new DvbRcsStd(this->up_return_pkt_hdl);
		this->scheduling = new ForwardSchedulingS2(this->down_forward_pkt_hdl,
		                                           fifos,
		                                           &this->fwd_fmt_simu,
		                                           this->categories.begin()->second);
	}
	else if(this->satellite_type == REGENERATIVE)
	{
		TerminalCategory *cat;

		if(!this->initBand(UP_RETURN_BAND,
		                   this->frame_duration_ms * this->frames_per_superframe,
		                   this->categories,
		                   this->terminal_affectation,
		                   &this->default_category,
		                   this->fwd_fmt_groups))
		{
			return false;
		}

		((DvbUpward *)this->upward)->receptionStd = new DvbS2Std(this->down_forward_pkt_hdl);
		// here we need the category to which the GW belongs
		if(this->terminal_affectation.find(GW_TAL_ID) != this->terminal_affectation.end())
		{
			cat = this->terminal_affectation[GW_TAL_ID];
		}
		else
		{
			cat = this->default_category;
		}
		this->scheduling = new UplinkSchedulingRcs(this->up_return_pkt_hdl,
		                                           fifos,
		                                           this->frames_per_superframe,
		                                           &this->ret_fmt_simu,
		                                           cat);
	}
	else
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "unknown value '%u' for satellite type ",
		                this->satellite_type);
		goto error;

	}
	if(!((DvbUpward *)this->upward)->receptionStd)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to create the reception standard\n");
		goto release_standards;
	}
	if(!this->scheduling)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to create the scheduling\n");
		goto release_standards;
	}

	return true;

release_standards:
	if(this->scheduling)
		delete this->scheduling;
	if(((DvbUpward *)this->upward)->receptionStd)
	  delete ((DvbUpward *)this->upward)->receptionStd;
error:
	return false;
}


bool BlockDvbNcc::initCarrierIds()
{
	// Get the ID for DVB control carrier
	if(!globalConfig.getValue(DVB_NCC_SECTION,
	                          DVB_CTRL_CAR,
	                          this->ctrl_carrier_id))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                DVB_NCC_SECTION, DVB_CTRL_CAR);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "DVB control carrier ID set to %u\n",
	                this->ctrl_carrier_id);

	// Get the ID for SOF carrier
	if(!globalConfig.getValue(DVB_NCC_SECTION,
	                          DVB_SOF_CAR,
	                          this->sof_carrier_id))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                DVB_NCC_SECTION, DVB_SOF_CAR);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "SoF carrier ID set to %u\n", this->sof_carrier_id);

	// Get the ID for data carrier
	if(!globalConfig.getValue(DVB_NCC_SECTION,
	                          DVB_DATA_CAR,
	                          this->data_carrier_id))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                DVB_NCC_SECTION, DVB_DATA_CAR);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "Data carrier ID set to %u\n", this->data_carrier_id);

	return true;

error:
	return false;
}


bool BlockDvbNcc::initFiles()
{
	// we need up/return MODCOD simulation in these cases
	if((this->satellite_type == TRANSPARENT &&
	    ((DvbUpward *)this->upward)->receptionStd->getType() == "DVB-RCS") ||
	   (this->satellite_type == REGENERATIVE)) // DVB-RCS emission in regenerative mode
	{
		if(!this->initReturnModcodFiles())
		{
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "failed to initialize the up/return MODCOD "
			                "files\n");
			goto error;
		}
	}

	// we need forward MODCOD emulation in this cases
	// in regenerative the satellite handles downlink MODCOD emulation
	if(this->satellite_type == TRANSPARENT)
	{
		if(!this->initForwardModcodFiles())
		{
			Output::sendLog(this->log_init, LEVEL_ERROR,
			                "failed to initialize the forward MODCOD files\n");
			goto error;
		}
	}

	// initialize the MODCOD IDs
	if(!this->ret_fmt_simu.goNextScenarioStep(false) ||
	   !this->fwd_fmt_simu.goNextScenarioStep(true))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to initialize MODCOD scheme IDs\n");
		goto error;
	}

	return true;

error:
	return false;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool BlockDvbNcc::initDama()
{
	string up_return_encap_proto;
	bool cra_decrease;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;

	TerminalCategories dc_categories;
	TerminalCategories::const_iterator cat_iter;
	TerminalMapping dc_terminal_affectation;
	TerminalCategory *dc_default_category;

	// Retrieving the cra decrease parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_CRA_DECREASE, cra_decrease))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "missing %s parameter", DC_CRA_DECREASE);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE, "cra_decrease = %s\n",
	                cra_decrease == true ? "true" : "false");

	// Retrieving the free capacity assignement parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_FREE_CAP, fca_kbps))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "missing %s parameter", DC_FREE_CAP);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "fca = %d kb/s\n", fca_kbps);

	// Retrieving the rbdc timeout parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_RBDC_TIMEOUT, rbdc_timeout_sf))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "missing %s parameter", DC_RBDC_TIMEOUT);
		goto error;
	}
	Output::sendLog(this->log_init, LEVEL_NOTICE,
	                "rbdc_timeout = %d superframes\n", rbdc_timeout_sf);

	if(this->satellite_type == TRANSPARENT)
	{
		if(!this->initBand(UP_RETURN_BAND,
		                   this->frame_duration_ms * this->frames_per_superframe,
		                   dc_categories,
		                   dc_terminal_affectation,
		                   &dc_default_category,
		                   this->ret_fmt_groups))
		{
			return false;
		}
	}
	else
	{
		// band already initialized in initMode
		dc_categories = this->categories;
		dc_terminal_affectation = this->terminal_affectation;
		dc_default_category = this->default_category;
	}

	// dama algorithm
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO,
	                          dama_algo))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': missing parameter '%s'\n",
		                DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	/* select the specified DAMA algorithm */
	// TODO create one DAMA per spot and add spot_id as param ?
	if(dama_algo == "Legacy")
	{
		Output::sendLog(this->log_init, LEVEL_NOTICE,
		                "creating Legacy DAMA controller\n");
		                this->dama_ctrl = new DamaCtrlRcsLegacy();

	}
	else
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': bad value for parameter '%s'\n",
		                DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	if(this->dama_ctrl == NULL)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to create the DAMA controller\n");
		goto error;
	}

	// Initialize the DamaCtrl parent class
	if(!this->dama_ctrl->initParent(this->frame_duration_ms,
	                                this->frames_per_superframe,
	                                this->with_phy_layer,
	                                this->up_return_pkt_hdl->getFixedLength(),
	                                cra_decrease,
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                // TODO only ret would be much more better
	                                &this->ret_fmt_simu,
	                                &this->fwd_fmt_simu))
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "failed to initialize the DAMA controller\n");
		goto release_dama;
	}
	this->dama_ctrl->setRecordFile(this->event_file, this->stat_file);

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
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "section '%s': bad value for parameter '%s'\n",
		                DVB_NCC_SECTION, DVB_SIZE_FIFO);
		return false;
	}
	this->data_dvb_fifo = new DvbFifo(this->data_carrier_id, val, "GWFifo");
	if(!this->data_dvb_fifo)
	{
		Output::sendLog(this->log_init, LEVEL_ERROR,
		                "Cannot create DVB fifo\n");
		return false;
	}

	return true;
}

bool BlockDvbNcc::initOutput(void)
{
	// Events
	this->event_logon_req = Output::registerEvent("BlockDvbNCC:logon_request");
	this->event_logon_resp = Output::registerEvent("BlockDvbNCC:logon_response");

	// Output probes and stats
	this->probe_gw_l2_to_sat_before_sched =
		Output::registerProbe<int>("Throughputs.L2_to_SAT.before_sched",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->l2_to_sat_bytes_before_sched = 0;

	this->probe_gw_l2_to_sat_after_sched =
		Output::registerProbe<int>("Throughputs.L2_to_SAT.after_sched",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->l2_to_sat_bytes_after_sched = 0;

	this->probe_gw_l2_from_sat=
		Output::registerProbe<int>("Throughputs.L2_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->l2_from_sat_bytes = 0;

	this->probe_frame_interval = Output::registerProbe<float>("Perf.Frames_interval",
	                                                          "ms", true,
	                                                          SAMPLE_LAST);
	this->probe_gw_queue_size = Output::registerProbe<int>("Queue size.packets",
	                                                       "Packets", true,
	                                                       SAMPLE_LAST);
	this->probe_gw_queue_size_kb = Output::registerProbe<int>("Queue size.kbits",
	                                                          "kbits", true,
	                                                          SAMPLE_LAST);

	if(this->satellite_type == REGENERATIVE)
	{
		this->probe_received_modcod = Output::registerProbe<int>("ACM.Received_modcod",
		                                                         "modcod index",
		                                                         true, SAMPLE_LAST);
		this->probe_rejected_modcod = Output::registerProbe<int>("ACM.Rejected_modcod",
		                                                         "modcod index",
		                                                         true, SAMPLE_LAST);
		this->probe_used_modcod = Output::registerProbe<int>("ACM.Used_modcod",
		                                                     "modcod index",
		                                                     true, SAMPLE_LAST);
	}
	return true;
}


/******************* EVENT MANAGEMENT *********************/


bool BlockDvbNcc::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		// burst
		case MSG_TYPE_BBFRAME:
			// ignore BB frames in transparent scenario
			// (this is required because the GW may receive BB frames
			//  in transparent scenario due to carrier emulation)
			if(((DvbUpward *)this->upward)->receptionStd->getType() == "DVB-RCS") 
			{
				Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
				                "ignore received BB frame in transparent "
				                "scenario\n");
				goto drop;
			}
			// breakthrough
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_CORRUPTED:
		{
			NetBurst *burst = NULL;

			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getPayloadLength();
			if(this->with_phy_layer)
			{
				// regenerative case : get downlink ACM parameters to inform
				//                     satellite ith a SAC
				this->cni = dvb_frame->getCn();

				// transparent case : update return modcod for terminal
				if(this->satellite_type == TRANSPARENT &&
				   ((DvbUpward *)this->upward)->receptionStd->getType() == "DVB-RCS")
				{
					DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
					tal_id_t tal_id;
					// decode the first packet in frame to be able to get source terminal ID
					if(!this->up_return_pkt_hdl->getSrc(frame->getPayload(), tal_id))
					{
						Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
						                "unable to read source terminal ID in"
						                " frame, won't be able to update C/N"
						                " value\n");
					}
					else
					{
						this->ret_fmt_simu.setRequiredModcod(tal_id, this->cni);
					}
				}
			}

			if(!((DvbUpward *)this->upward)->receptionStd->onRcvFrame(dvb_frame,
			                                                          this->macId,
			                                                          &burst))
			{
				Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
				                "failed to handle DVB frame or BB frame\n");
				goto error;
			}
			if(((DvbUpward *)this->upward)->receptionStd->getType() == "DVB-S2")
			{
				DvbS2Std *std = (DvbS2Std *)((DvbUpward *)this->upward)->receptionStd;
				if(msg_type != MSG_TYPE_CORRUPTED)
				{
					this->probe_received_modcod->put(
							std->getReceivedModcod());
				}
				else
				{
					this->probe_rejected_modcod->put(
							std->getReceivedModcod());
				}
			}


			if(burst && !this->SendNewMsgToUpperLayer(burst))
			{
				Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
				                "failed to send burst to upper layer\n");
				goto error;
			}
		}
		break;

		case MSG_TYPE_SAC:
		{
			// TODOSac *sac = dynamic_cast<Sac *>(dvb_frame);
			Sac *sac = (Sac *)dvb_frame;

			Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
			                "handle received SAC\n");

			if(!this->dama_ctrl->hereIsSAC(sac, this->satellite_type))
			{
				Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
				                "failed to handle SAC frame\n");
				goto error;
			}
			delete dvb_frame;
		}
		break;

		case MSG_TYPE_SESSION_LOGON_REQ:
			Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
			                "Logon Req\n");
			this->onRcvLogonReq(dvb_frame);
			break;

		case MSG_TYPE_SESSION_LOGOFF:
			Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
			                "Logoff Req\n");
			this->onRcvLogoffReq(dvb_frame);
			break;

		case MSG_TYPE_TTP:
		case MSG_TYPE_SESSION_LOGON_RESP:
		case MSG_TYPE_SOF:
			// nothing to do in this case
			Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
			                "ignore TTP, logon response or SOF frame "
			                "(type = %d)\n", dvb_frame->getMessageType());
			delete dvb_frame;
			break;

		default:
			Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
			                "unknown type (%d) of DVB frame\n",
			                dvb_frame->getMessageType());
			delete dvb_frame;
			break;
	}

	return true;

drop:
	delete dvb_frame;
	return true;

error:
	Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
	                "Treatments failed at SF#%u\n",
	                this->super_frame_counter);
	return false;
}


/**
 * Send a start of frame
 */
void BlockDvbNcc::sendSOF()
{
	Sof *sof = new Sof(this->super_frame_counter);

	// Send it
	if(!((DvbDownward *)this->downward)->sendDvbFrame((DvbFrame *)sof, this->sof_carrier_id))
	{
		Output::sendLog(this->log_send_down, LEVEL_ERROR,
		                "Failed to call sendDvbFrame() for SOF\n");
		return;
	}

	Output::sendLog(this->log_send_down, LEVEL_DEBUG,
	                "SF#%u: SOF sent\n", this->super_frame_counter);
}


void BlockDvbNcc::onRcvLogonReq(DvbFrame *dvb_frame)
{
	//TODO find why dynamic cast fail here !?
//	LogonRequest *logon_req = dynamic_cast<LogonRequest *>(dvb_frame);
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();
	std::list<long>::iterator list_it;

	Output::sendLog(this->log_rcv_from_down, LEVEL_INFO,
	                "Logon request from ST%u\n", mac);

	// refuse to register a ST with same MAC ID as the NCC
	if(mac == this->macId)
	{
		Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
		                "a ST wants to register with the MAC ID of the NCC "
		                "(%d), reject its request!\n", mac);
		goto release;
	}

	// send the corresponding event
	Output::sendEvent(this->event_logon_req, "Logon request received from %u",
	                  mac);

	// register the new ST
	if(this->ret_fmt_simu.doTerminalExist(mac) &&
	   this->fwd_fmt_simu.doTerminalExist(mac))
	{
		// ST already registered once
		Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
		                "request to register ST with ID %u that is already "
		                "registered, resend logon response\n", mac);
	}
	else
	{
		// ST was not registered yet
		Output::sendLog(this->log_rcv_from_down, LEVEL_NOTICE,
		                "register ST with MAC ID %u\n", mac);
		if(this->column_list.find(mac) == this->column_list.end() ||
		   !this->ret_fmt_simu.addTerminal(mac, this->column_list[mac]) ||
		   !this->fwd_fmt_simu.addTerminal(mac, this->column_list[mac]))
		{
			Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
			                "failed to register ST with MAC ID %u\n", mac);
			goto release;
		}
	}

	// Inform the Dama controler (for its own context)
	if(this->dama_ctrl->hereIsLogon(logon_req))
	{
		LogonResponse *logon_resp = new LogonResponse(mac, 0, mac);

		// Send it
		if(!((DvbDownward *)this->downward)->sendDvbFrame((DvbFrame *)logon_resp,
		                 this->ctrl_carrier_id))
		{
			Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
			                "Failed send message\n");
			goto release;
		}

		Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
		                "SF#%u: logon response sent to lower layer\n",
		                this->super_frame_counter);


		// send the corresponding event
		Output::sendEvent(this->event_logon_resp, "Logon response send to %u",
		                  mac);

	}

release:
	delete dvb_frame;
}

void BlockDvbNcc::onRcvLogoffReq(DvbFrame *dvb_frame)
{
	std::list<long>::iterator list_it;
// TODO	Logoff *logoff = dynamic_cast<Logoff *>(dvb_frame);
	Logoff *logoff = (Logoff *)dvb_frame;


	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->ret_fmt_simu.delTerminal(logoff->getMac()) ||
	   !this->fwd_fmt_simu.delTerminal(logoff->getMac()))
	{
		Output::sendLog(this->log_rcv_from_down, LEVEL_ERROR,
		                "failed to delete the ST with ID %d\n",
		                logoff->getMac());
		goto release;
	}

	this->dama_ctrl->hereIsLogoff(logoff);
	Output::sendLog(this->log_rcv_from_down, LEVEL_DEBUG,
	                "SF#%u: logoff request from %d\n",
	                this->super_frame_counter, logoff->getMac());

release:
	delete dvb_frame;
}

void BlockDvbNcc::sendTTP()
{
	Ttp *ttp = new Ttp(0, this->super_frame_counter);
	// Build TTP
	if(!this->dama_ctrl->buildTTP(ttp))
	{
		delete ttp;
		Output::sendLog(this->log_send_down, LEVEL_DEBUG,
		                "Dama didn't build TTP\bn");
		return;
	};

	if(!((DvbDownward *)this->downward)->sendDvbFrame((DvbFrame *)ttp, this->ctrl_carrier_id))
	{
		delete ttp;
		Output::sendLog(this->log_send_down, LEVEL_ERROR,
		                "Failed to send TTP\n");
		return;
	}

	Output::sendLog(this->log_send_down, LEVEL_DEBUG,
	                "SF#%u: TTP sent\n", this->super_frame_counter);
}

bool BlockDvbNcc::simulateFile()
{
	static bool simu_eof = false;
	static char buffer[255] = "";
	enum
	{ none, cr, logon, logoff } event_selected;

	int resul;
	time_sf_t sf_nr;
	tal_id_t st_id;
	uint32_t st_request;
	rate_kbps_t st_rt;
	rate_kbps_t st_rbdc;
	vol_kb_t st_vbdc;
	int cr_type;

	if(simu_eof)
	{
		Output::sendLog(this->log_request_simulation, LEVEL_DEBUG,
		                "End of file\n");
		goto error;
	}

	sf_nr = -1;
	while(sf_nr <= this->super_frame_counter)
	{
		if(4 ==
		   sscanf(buffer, "SF#%hu CR st%hu cr=%u type=%d", &sf_nr, &st_id,
		   &st_request, &cr_type))
		{
			event_selected = cr;
		}
		else if(5 ==
		        sscanf(buffer, "SF#%hu LOGON st%hu rt=%hu rbdc=%hu vbdc=%hu",
		               &sf_nr, &st_id, &st_rt, &st_rbdc, &st_vbdc))
		{
			event_selected = logon;
		}
		else if(2 == sscanf(buffer, "SF#%hu LOGOFF st%hu", &sf_nr, &st_id))
		{
			event_selected = logoff;
		}
		else
		{
			event_selected = none;
		}
		if(st_id <= BROADCAST_TAL_ID)
		{
			Output::sendLog(this->log_request_simulation, LEVEL_ERROR,
			                "Simulated ST%u ignored, IDs smaller than %u "
			                "reserved for emulated terminals\n",
			                st_id, BROADCAST_TAL_ID);
			goto loop_step;
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
			Sac *sac = new Sac(st_id);

			sac->addRequest(0, cr_type, st_request);
			Output::sendLog(this->log_request_simulation, LEVEL_INFO,
			                "SF#%u: send a simulated CR of type %u with "
			                "value = %u for ST %hu\n",
			                this->super_frame_counter, cr_type,
			                st_request, st_id);
			if(!this->dama_ctrl->hereIsSAC(sac))
			{
				goto error;
			}
			break;
		}
		case logon:
		{
			LogonRequest *sim_logon_req = new LogonRequest(st_id,
			                                               st_rt,
			                                               st_rbdc,
			                                               st_vbdc);
			bool ret = false;

			Output::sendLog(this->log_request_simulation, LEVEL_INFO,
			                "SF#%u: send a simulated logon for ST %d\n",
			                this->super_frame_counter, st_id);
			// check for column in FMT simulation list
			if(this->column_list.find(st_id) == this->column_list.end())
			{
				Output::sendLog(this->log_request_simulation, LEVEL_NOTICE,
				                "no column ID for simulated terminal, use the"
				                " terminal ID\n");
				ret = this->ret_fmt_simu.addTerminal(st_id, st_id) ||
				      this->fwd_fmt_simu.addTerminal(st_id, st_id);
			}
			else
			{
				ret = this->ret_fmt_simu.addTerminal(st_id, this->column_list[st_id]) ||
				      this->fwd_fmt_simu.addTerminal(st_id, this->column_list[st_id]);
			}
			if(!ret)
			{
				Output::sendLog(this->log_request_simulation, LEVEL_ERROR,
				                "failed to register simulated ST with MAC "
				                "ID %u\n", st_id);
				goto error;
			}

			if(!this->dama_ctrl->hereIsLogon(sim_logon_req))
			{
				goto error;
			}
		}
		break;
		case logoff:
		{
			Logoff *sim_logoff = new Logoff(st_id);
			Output::sendLog(this->log_request_simulation, LEVEL_INFO,
			                "SF#%u: send a simulated logoff for ST %d\n",
			                this->super_frame_counter, st_id);
			if(!this->dama_ctrl->hereIsLogoff(sim_logoff))
			{
				goto error;
			}
		}
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
			Output::sendLog(this->log_request_simulation, LEVEL_DEBUG,
			                "fscanf resul=%d: %s", resul, buffer);
			//fprintf (stderr, "frame %d\n", this->super_frame_counter);
			Output::sendLog(this->log_request_simulation, LEVEL_DEBUG,
			                "frame %u\n", this->super_frame_counter);
			if(resul == -1)
			{
				simu_eof = true;
				Output::sendLog(this->log_request_simulation, LEVEL_DEBUG,
				                "End of file.\n");
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

	int i;
	// BROADCAST_TAL_ID is maximum tal_id for emulated terminals
	tal_id_t sim_tal_id = BROADCAST_TAL_ID + 1;

	if(!initialized)
	{
		for(i = 0; i < this->simu_st; i++)
		{
			tal_id_t tal_id = sim_tal_id + i;
			LogonRequest *sim_logon_req = new LogonRequest(tal_id, this->simu_rt,
			                                               this->simu_max_rbdc,
			                                               this->simu_max_vbdc);
			bool ret = false;

			// check for column in FMT simulation list
			if(this->column_list.find(tal_id) == this->column_list.end())
			{
				Output::sendLog(this->log_request_simulation, LEVEL_NOTICE,
				                "no column ID for simulated terminal, use the"
				                " terminal ID\n");
				ret = this->ret_fmt_simu.addTerminal(tal_id, tal_id) ||
				      this->fwd_fmt_simu.addTerminal(tal_id, tal_id);
			}
			else
			{
				ret = this->ret_fmt_simu.addTerminal(tal_id, this->column_list[tal_id]) ||
				      this->fwd_fmt_simu.addTerminal(tal_id, this->column_list[tal_id]);
			}
			if(!ret)
			{
				Output::sendLog(this->log_request_simulation, LEVEL_ERROR,
				                "failed to register simulated ST with MAC"
				                " ID %u\n", tal_id);
				return;
			}

			this->dama_ctrl->hereIsLogon(sim_logon_req);
		}
		initialized = true;
	}

	for(i = 0; i < this->simu_st; i++)
	{
		uint32_t val;
		Sac *sac = new Sac(sim_tal_id + i);

		val = this->simu_cr - this->simu_interval / 2 +
		      random() % this->simu_interval;
		sac->addRequest(0, cr_rbdc, val);

		this->dama_ctrl->hereIsSAC(sac);
	}
}

void BlockDvbNcc::updateStats()
{

	// Update stats on the GW
	this->dama_ctrl->updateStatistics(this->stats_period_ms);

	// Update common DAMA statistics
	mac_fifo_stat_context_t fifo_stat;
	this->data_dvb_fifo->getStatsCxt(fifo_stat);
	this->l2_to_sat_bytes_after_sched = fifo_stat.out_length_bytes;

	this->probe_gw_l2_to_sat_before_sched->put(
		this->l2_to_sat_bytes_before_sched * 8.0 / this->stats_period_ms);
	this->l2_to_sat_bytes_before_sched = 0;

	this->probe_gw_l2_to_sat_after_sched->put(
		this->l2_to_sat_bytes_after_sched * 8.0 / this->stats_period_ms);
	this->l2_to_sat_bytes_after_sched = 0;

	this->probe_gw_l2_from_sat->put(
		this->l2_from_sat_bytes * 8.0 / this->stats_period_ms);
	this->l2_from_sat_bytes = 0;

	// Mac fifo stats
	this->probe_gw_queue_size->put(fifo_stat.current_pkt_nbr);
	this->probe_gw_queue_size_kb->put(fifo_stat.current_length_bytes * 8 / 1000); //TODO

	// Send probes
	Output::sendProbes();

}

bool BlockDvbNcc::sendAcmParameters()
{
	Sac *send_sac = new Sac(GW_TAL_ID);
	send_sac->setAcm(this->cni);
	Output::sendLog(this->log_send_down, LEVEL_DEBUG,
	                "Send SAC with CNI = %.2f\n", this->cni);

	// Send message
	if(!((DvbDownward *)this->downward)->sendDvbFrame((DvbFrame *)send_sac,
	                                                  this->ctrl_carrier_id))
	{
		Output::sendLog(this->log_send_down, LEVEL_ERROR,
		                "SF#%u frame %u: failed to send SAC\n",
		                this->super_frame_counter, this->frame_counter);
		delete send_sac;
		return false;
	}
	return true;
}


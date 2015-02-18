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

/*
 * REMINDER:
 *  // in transparent mode
 *        - downward => forward link
 *        - upward => return link
 *  // in regenerative mode
 *        - downward => uplink
 *        - upward => downlink
 */

/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/


BlockDvbNcc::BlockDvbNcc(const string &name):
	BlockDvb(name)
{
}

BlockDvbNcc::~BlockDvbNcc()
{
}

bool BlockDvbNcc::onInit(void)
{
	return true;
}


bool BlockDvbNcc::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}


bool BlockDvbNcc::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}

/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/


// TODO lot of duplicated code for fifos between ST and GW

BlockDvbNcc::Downward::Downward(Block *const bl):
	DvbDownward(bl),
	NccPepInterface(),
	fwd_timer(-1),
	up_ret_fmt_simu(),
	down_fwd_fmt_simu(),
	scenario_timer(-1),
	column_list(),
	probe_frame_interval(NULL)
{
}

BlockDvbNcc::Downward::~Downward()
{
	
	list<spot_id_t>::iterator spot_iter;
	for(spot_iter = this->spot_list.begin(); (spot_iter != this->spot_list.end()); spot_iter++)
	{
		spot_id_t s_id = *spot_iter;
		delete this->spot_downward_map[s_id];
	}
}


bool BlockDvbNcc::Downward::onInit(void)
{
	bool result = true;
	const char *scheme;
	list<spot_id_t>::iterator spot_iter;

	if(!this->initMap())
	{
		LOG(this->log_init, LEVEL_ERROR,
			"failed to init carrier and terminal "
			"spot id map");
		goto error;
	}
	
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		goto error;
	}

	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed get satellite type\n");
		goto error;
	}

	// get the common parameters
	if(this->satellite_type == TRANSPARENT)
	{
		scheme = FORWARD_DOWN_ENCAP_SCHEME_LIST;
	}
	else
	{
		scheme = RETURN_UP_ENCAP_SCHEME_LIST;
	}

	if(!this->initCommon(scheme))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		goto error;
	}

	// initialize the timers
	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the timers part of the "
		    "initialisation\n");
	}

	// Get and open the files
	if(!this->initModcodSimu())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the files part of the "
		    "initialisation\n");
		goto error;
	}


//	this->addNetSocketEvent("pep_listen", this->getPepListenSocket(), 200);

	for(spot_iter = this->spot_list.begin(); spot_iter != this->spot_list.end(); spot_iter++)
	{
		spot_id_t spot_id = *spot_iter;
		this->spot_downward_map[spot_id] = new SpotDownward(this->fwd_down_frame_duration_ms,
		                                              this->ret_up_frame_duration_ms,
		                                              this->stats_period_ms,
		                                              this->up_ret_fmt_simu,
		                                              this->down_fwd_fmt_simu,
		                                              this->satellite_type,
		                                              this->pkt_hdl,
		                                              this->with_phy_layer);
		this->spot_downward_map[spot_id]->setSpotId(spot_id);
		result = this->spot_downward_map[spot_id]->onInit();
	}

	// Output probes and stats
	this->probe_frame_interval = Output::registerProbe<float>("ms", true,
	                                                          SAMPLE_LAST,
	                                                          "Perf.Frames_interval");

	// everything went fine
	return result;

error:
	return false;

}

bool BlockDvbNcc::Downward::initTimers(void)
{
	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	this->frame_timer = this->addTimerEvent("frame",
	                                        this->ret_up_frame_duration_ms);
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                      this->fwd_down_frame_duration_ms);

	// Launch the timer in order to retrieve the modcods if there is no physical layer
	// or to send SAC with ACM parameters in regenerative mode
	if(!this->with_phy_layer || this->satellite_type == REGENERATIVE)
	{
		this->scenario_timer = this->addTimerEvent("scenario",
		                                           this->dvb_scenario_refresh);
	}

	// read the pep allocation delay
	/*if(!Conf::getValue(NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY,
	                   this->pep_alloc_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "pep_alloc_delay set to %d ms\n", this->pep_alloc_delay);
	// create timer
	this->pep_cmd_apply_timer = this->addTimerEvent("pep_request",
	                                                pep_alloc_delay,
	                                                false, // no rearm
	                                                false // do not start
	                                                );*/

	return true;

/*error:
	return false;*/
}

bool BlockDvbNcc::Downward::initModcodSimu(void)
{
	if(!this->initModcodFiles(RETURN_UP_MODCOD_DEF_RCS,
	                          RETURN_UP_MODCOD_TIME_SERIES,
	                          this->up_ret_fmt_simu))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD files\n");
		goto error;
	}
	if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2,
	                          FORWARD_DOWN_MODCOD_TIME_SERIES,
	                          this->down_fwd_fmt_simu))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD files\n");
		goto error;
	}

	// initialize the MODCOD IDs
	if(!this->down_fwd_fmt_simu.goNextScenarioStep(true) ||
	   !this->up_ret_fmt_simu.goNextScenarioStep(false))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize MODCOD scheme IDs\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvbNcc::Downward::onEvent(const RtEvent *const event)
{
	DvbFrame *dvb_frame = NULL;
	list<spot_id_t>::iterator spot_iter;
	// for each spot
	
	switch(event->getType())
	{
		case evt_message:
		{
			// first handle specific messages
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
				
				LogonResponse *logonResp;
				uint8_t ctrlCarrierId;
				spot_id_t dest_spot = dvb_frame->getSpot();
				uint8_t msg_type = dvb_frame->getMessageType();

				if(this->spot_downward_map[dest_spot] == NULL)
				{
					LOG(this->log_init_channel, LEVEL_ERROR,
							"spot_downward of spot %d does'nt exist",
							dest_spot);
					break;
				}

				switch(msg_type)
				{
					case MSG_TYPE_BBFRAME:
					case MSG_TYPE_DVB_BURST:
					case MSG_TYPE_CORRUPTED:
						{
							double curr_cni = dvb_frame->getCn();
							if(this->satellite_type == REGENERATIVE)
							{
								// regenerative case : we need downlink ACM parameters to inform
								//                     satellite with a SAC so inform opposite channel
								this->spot_downward_map[dest_spot]->setCni(curr_cni);
							}
							else
							{
								// transparent case : update return modcod for terminal
								DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
								tal_id_t tal_id;
								// decode the first packet in frame to be able to get source terminal ID
								if(!this->spot_downward_map[dest_spot]->getUpReturnPktHdl()->getSrc(frame->getPayload(), 
											tal_id))
								{
									LOG(this->log_receive, LEVEL_ERROR,
											"unable to read source terminal ID in"
											" frame, won't be able to update C/N"
											" value\n");
								}
								else
								{
									this->up_ret_fmt_simu.setRequiredModcod(tal_id, curr_cni);
								}
							}
						}
						break;

					case MSG_TYPE_SAC: // when physical layer is enabled
						{
							// TODO Sac *sac = dynamic_cast<Sac *>(dvb_frame);
							Sac *sac = (Sac *)dvb_frame;

							LOG(this->log_receive, LEVEL_DEBUG,
									"handle received SAC\n");

							if(!this->spot_downward_map[dest_spot]->getDamaCtrl()->hereIsSAC(sac))
							{
								LOG(this->log_receive, LEVEL_ERROR,
										"failed to handle SAC frame\n");
								delete dvb_frame;
								goto error;
							}

							if(this->with_phy_layer)
							{
								// transparent : the C/N0 of forward link
								// regenerative : the C/N0 of uplink (updated by sat)
								double cni = sac->getCni();
								tal_id_t tal_id = sac->getTerminalId();
								if(this->satellite_type == TRANSPARENT)
								{
									this->down_fwd_fmt_simu.setRequiredModcod(tal_id, cni);
								}
								else
								{
									this->up_ret_fmt_simu.setRequiredModcod(tal_id, cni);
								}
							}
						}
						break;

					case MSG_TYPE_SESSION_LOGON_REQ:
						if(!this->spot_downward_map[dest_spot]->handleLogonReq(dvb_frame, 
									&logonResp,
									ctrlCarrierId,
									this->super_frame_counter))
						{
							goto error;
						}
						if(!this->sendDvbFrame((DvbFrame *)logonResp,
									ctrlCarrierId))
						{
							LOG(this->log_receive, LEVEL_ERROR,
									"Failed send logon response\n");
							return false;
						}

						break;

					case MSG_TYPE_SESSION_LOGOFF:
						if(!this->spot_downward_map[dest_spot]->handleLogoffReq(dvb_frame,
									this->super_frame_counter))
						{
							goto error;
						}
						break;

					default:
						LOG(this->log_receive, LEVEL_ERROR,
								"SF#%u: unknown type of DVB frame (%u), ignore\n",
								this->super_frame_counter,
								dvb_frame->getMessageType());
						delete dvb_frame;
						goto error;
				}

				delete dvb_frame;
				return true;
			}
			else if(((MessageEvent *)event)->getMessageType() == msg_saloha)
			{
				list<DvbFrame *> *ack_frames;
				ack_frames = (list<DvbFrame *> *)((MessageEvent *)event)->getData();
				
				spot_id_t spot = ack_frames->front()->getSpot();
				this->spot_downward_map[spot]->handleMsgSaloha(ack_frames);
				delete ack_frames;
			}
			else
			{
				NetBurst *burst;
				NetBurst::iterator pkt_it;

				burst = (NetBurst *)((MessageEvent *)event)->getData();

				LOG(this->log_receive_channel, LEVEL_INFO,
						"SF#%u: encapsulation burst received "
						"(%d packet(s))\n", super_frame_counter,
						burst->length());

				// set each packet of the burst in MAC FIFO
				for(pkt_it = burst->begin(); pkt_it != burst->end(); ++pkt_it)
				{
					// TODO get terminal ID en débuire le spot
					int tal_id = (*pkt_it)->getDstTalId();
					map<int, spot_id_t>::iterator spot_it;
					spot_it = this->terminal_map.find(tal_id);
					if(spot_it == this->terminal_map.end())
					{
						LOG(this->log_receive, LEVEL_ERROR,
								"cannot find terminal %u' spot\n",
								tal_id);
						break;
					}
					spot_id_t spot = this->terminal_map[tal_id];

					if(!this->spot_downward_map[spot]->handleBurst(pkt_it,
						                                       super_frame_counter))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "cannot push burst into fifo\n");
						burst->clear(); // avoid deteleting packets when deleting burst
						delete burst;
						goto error;
					}
				}
				burst->clear(); // avoid deteleting packets when deleting burst
				delete burst;

			}
			break;
		}
		case evt_timer:
		{
			// receive the frame Timer event
			LOG(this->log_receive, LEVEL_DEBUG,
				"timer event received on downward channel");
			if(*event == this->frame_timer)
			{
				if(this->probe_frame_interval->isEnabled())
				{
					timeval time = event->getAndSetCustomTime();
					float val = time.tv_sec * 1000000L + time.tv_usec;
					this->probe_frame_interval->put(val/1000);
				}

				// we reached the end of a superframe
				// beginning of a new one, send SOF and run allocation
				// algorithms (DAMA)
				// increase the superframe number and reset
				// counter of frames per superframe
				this->super_frame_counter++;

				for(spot_iter = this->spot_list.begin(); spot_iter != this->spot_list.end(); spot_iter++)
				{
					spot_id_t spot_id = *spot_iter;
				
					// sendSOF appelé dans spot (idem senTTP)
					// send Start Of Frame (Spot)
					this->sendSOF(spot_downward_map[spot_id]);


					// fonction sur chaque spot
					if(!this->spot_downward_map[spot_id]->getDamaCtrl())
					{
						// stop here
						return true;
					}

					if(this->with_phy_layer)
					{
						// for each terminal in DamaCtrl update FMT because in this case
						// this it not done with scenario timer and FMT is updated
						// each received frame but we only need it for allocation
						this->spot_downward_map[spot_id]->getDamaCtrl()->updateFmt();
					}

					// run the allocation algorithms (DAMA)
					this->spot_downward_map[spot_id]->getDamaCtrl()->runOnSuperFrameChange(this->super_frame_counter);
					// send TTP computed by DAMA
					this->sendTTP(spot_downward_map[spot_id]);

					// **************
					// Simulation
					// **************
					switch(this->spot_downward_map[spot_id]->getSimulate())
					{
						case file_simu:
							if(!this->spot_downward_map[spot_id]->getSimuFile())
							{
								LOG(spot_downward_map[spot_id]->getLogRequestSimulation(), LEVEL_ERROR,
										"file simulation failed");
								fclose(this->spot_downward_map[spot_id]->getSimuFile());
								this->spot_downward_map[spot_id]->setSimuFile(NULL);
								this->spot_downward_map[spot_id]->setSimulate(none_simu);
							}
							break;
						case random_simu:
							this->spot_downward_map[spot_id]->simulateRandom();
							break;
						default:
							break;
					}
					// flush files
					fflush(this->spot_downward_map[spot_id]->getEventFile());

				}

			}
			else if(*event == this->fwd_timer)
			{
				uint32_t remaining_alloc_sym = 0;

				// for each spot
				for(spot_iter = this->spot_list.begin(); spot_iter != this->spot_list.end(); spot_iter++)
				{
					spot_id_t spot_id = *spot_iter;
					
					this->spot_downward_map[spot_id]->updateStatistics();
					this->spot_downward_map[spot_id]->setFwdFrameCounter(
					    this->spot_downward_map[spot_id]->getFwdFrameCounter()+1);

					// schedule encapsulation packets
					// TODO loop on categories (see todo in initMode)
					// TODO In regenerative mode we should schedule in frame_timer ??
					if(!this->spot_downward_map[spot_id]->schedule(getCurrentTime(),
					                                         remaining_alloc_sym))
					{
						LOG(this->log_receive, LEVEL_ERROR,
							"failed to schedule encapsulation "
							"packets stored in DVB FIFO\n");
						return false;
					}
					if(this->satellite_type == REGENERATIVE &&
					   spot_downward_map[spot_id]->getCompleteDvbFrames().size() > 0)
					{
						// we can do that because we have only one MODCOD per allocation
						// TODO THIS IS NOT TRUE ! we schedule for each carriers, if
						// desired modcod is low we can send on many carriers
						uint8_t modcod_id;
						modcod_id = ((DvbRcsFrame *)this->spot_downward_map[spot_id]->getCompleteDvbFrames().front())->getModcodId();
						this->spot_downward_map[spot_id]->getProbeUsedModcod()->put(modcod_id);
					}
					LOG(this->log_receive, LEVEL_INFO,
						"SF#%u: %u symbols remaining after "
						"scheduling\n", this->super_frame_counter,
						remaining_alloc_sym);

					// get complete_dvb.. get data_carrier
					if(!this->sendBursts(&this->spot_downward_map[spot_id]->getCompleteDvbFrames(),
					                     spot_downward_map[spot_id]->getDataCarrierId()))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "failed to build and send DVB/BB "
						    "frames\n");
						return false;
					}
				}
				//------------------------
			}
			else if(*event == this->scenario_timer)
			{
				//TODO boucle globale
				// for each spot
				for(spot_iter = this->spot_list.begin(); spot_iter != this->spot_list.end(); spot_iter++)
				{
					spot_id_t spot_id = *spot_iter;

					// if regenerative satellite and physical layer scenario,
					// send ACM parameters
					if(this->satellite_type == REGENERATIVE &&
					   this->with_phy_layer)
					{
						this->sendAcmParameters(spot_downward_map[spot_id]);
					}

					// it's time to update MODCOD IDs
					LOG(this->log_receive, LEVEL_DEBUG,
					    "MODCOD scenario timer received\n");

					if(!this->up_ret_fmt_simu.goNextScenarioStep(false) ||
					   !this->down_fwd_fmt_simu.goNextScenarioStep(true))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "SF#%u: failed to update MODCOD IDs\n",
						    this->super_frame_counter);
					}
					else
					{
						LOG(this->log_receive, LEVEL_DEBUG,
						    "SF#%u: MODCOD IDs successfully updated\n",
						    this->super_frame_counter);
					}
					if(spot_downward_map[spot_id]->getDamaCtrl())
					{
						// TODO FMT in slotted aloha should be handled on ST
						//  => so remove return fmt simu !
						//  => keep this todo in order to think of it on ST

						// for each terminal in DamaCtrl update FMT
						spot_downward_map[spot_id]->getDamaCtrl()->updateFmt();
					}
				}
			}
			/*else if(*event == this->pep_cmd_apply_timer)
			{
				// it is time to apply the command sent by the external
				// PEP component

				PepRequest *pep_request;

				LOG(this->log_receive, LEVEL_NOTICE,
				    "apply PEP requests now\n");
				while((pep_request = this->getNextPepRequest()) != NULL)
				{
					if(this->dama_ctrl->applyPepCommand(pep_request))
					{
						LOG(this->log_receive, LEVEL_NOTICE,
						    "PEP request successfully "
						    "applied in DAMA\n");
					}
					else
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "failed to apply PEP request "
						    "in DAMA\n");
						return false;
					}
				}
			}*/
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unknown timer event received %s\n",
				    event->getName().c_str());
				return false;
			}
			break;
		}
		case evt_net_socket:
		{
			if(*event == this->getPepListenSocket())
			{
				int ret;

				// event received on PEP listen socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP listen socket\n");

				// create the client socket to receive messages
				ret = acceptPepConnection();
				if(ret == 0)
				{
					LOG(this->log_receive, LEVEL_NOTICE,
					    "NCC is now connected to PEP\n");
					// add a fd to handle events on the client socket
					this->addNetSocketEvent("pep_client",
					                        this->getPepClientSocket(),
					                        200);
				}
				else if(ret == -1)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "failed to accept new connection "
					    "request from PEP\n");
				}
				else if(ret == -2)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "one PEP already connected: "
					    "reject new connection request\n");
				}
				else
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "unknown status %d from "
					    "acceptPepConnection()\n", ret);
					return false;
				}
			}
			/*else if(*event == this->getPepClientSocket())
			{
				// event received on PEP client socket
				LOG(this->log_receive, LEVEL_NOTICE,
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
						if(!this->startTimer(this->pep_cmd_apply_timer))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "cannot start pep timer");
							return false;
						}
						LOG(this->log_receive, LEVEL_NOTICE,
						    "PEP Allocation request, apply a %dms"
						    " delay\n", pep_alloc_delay);
					}
					else if(this->getPepRequestType() == PEP_REQUEST_RELEASE)
					{
						this->raiseTimer(this->pep_cmd_apply_timer);
						LOG(this->log_receive, LEVEL_NOTICE,
						    "PEP Release request, no delay to "
						    "apply\n");
					}
					else
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "cannot determine request type!\n");
						return false;
					}
				}
				else
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "network problem encountered with PEP, "
					    "connection was therefore closed\n");
					this->removeEvent(this->pep_cmd_apply_timer);
					return false;
				}
			}*/
		}
		default:
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
		}
	}

	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u\n",
	    this->super_frame_counter);
	return false;

}

void BlockDvbNcc::Downward::sendSOF(SpotDownward *spot_downward)
{
	Sof *sof = new Sof(this->super_frame_counter);

	// Send it
	if(!this->sendDvbFrame((DvbFrame *)sof, spot_downward->getSofCarrierId()))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to call sendDvbFrame() for SOF\n");
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: SOF sent\n", this->super_frame_counter);
}

void BlockDvbNcc::Downward::sendTTP(SpotDownward *spot_downward)
{
	Ttp *ttp = new Ttp(0, this->super_frame_counter);
	// Build TTP
	if(!spot_downward->getDamaCtrl()->buildTTP(ttp))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_DEBUG,
		    "Dama didn't build TTP\bn");
		return;
	};

	if(!this->sendDvbFrame((DvbFrame *)ttp, spot_downward->getCtrlCarrierId()))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send TTP\n");
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: TTP sent\n", this->super_frame_counter);
}

bool BlockDvbNcc::Downward::sendAcmParameters(SpotDownward *spot_downward)
{
	Sac	*send_sac = new Sac(GW_TAL_ID);
	send_sac->setAcm(spot_downward->getCni());
	LOG(this->log_send, LEVEL_DEBUG,
	    "Send SAC with CNI = %.2f\n", spot_downward->getCni());

	// Send message
	if(!this->sendDvbFrame((DvbFrame *)send_sac,
	                       spot_downward->getCtrlCarrierId()))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u: failed to send SAC\n",
		    this->super_frame_counter);
		delete send_sac;
		return false;
	}
	return true;
}

 void BlockDvbNcc::Downward::updateStats(void)
{

}


/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbNcc::Upward::Upward(Block *const bl):
	DvbUpward(bl),
	log_saloha(NULL)
{
}


BlockDvbNcc::Upward::~Upward()
{
	list<spot_id_t>::iterator spot_iter;
	for(spot_iter = this->spot_list.begin(); (spot_iter != this->spot_list.end()); spot_iter++)
	{
		spot_id_t spot_id = *spot_iter;
		delete this->spot_upward_map[spot_id];
	}
}


bool BlockDvbNcc::Upward::onInit(void)
{
	bool result = true;
	list<spot_id_t>::iterator spot_iter;
	
	if(!this->initMap())
	{
		LOG(this->log_init, LEVEL_ERROR,
			"failed to init carrier and terminal "
			"spot id map");
		return false;
	}

	for(spot_iter = this->spot_list.begin(); spot_iter != this->spot_list.end(); spot_iter++)
	{
		spot_id_t spot_id = *spot_iter;
		this->spot_upward_map[spot_id] = new SpotUpward();
		this->spot_upward_map[spot_id]->setSpotId(spot_id);

		result = this->spot_upward_map[spot_id]->onInit();
	}

	if(result)
	{
		T_LINK_UP *link_is_up;
		// create and send a "link is up" message to upper layer
		link_is_up = new T_LINK_UP;
		if(link_is_up == NULL)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "SF#%u: failed to allocate memory for link_is_up "
			    "message\n", this->super_frame_counter);
		}
		link_is_up->group_id = 0;
		link_is_up->tal_id = GW_TAL_ID;

		if(!this->enqueueMessage((void **)(&link_is_up),
		                         sizeof(T_LINK_UP),
		                         msg_link_up))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "SF#%u: failed to send link up message to upper layer",
			    this->super_frame_counter);
			delete link_is_up;
		}
	}
	// everything went fine
	return result;
}

bool BlockDvbNcc::Upward::onEvent(const RtEvent *const event)
{
	DvbFrame *dvb_frame = NULL;

	switch(event->getType())
	{
		case evt_message:
		{
			dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
			spot_id_t dest_spot = dvb_frame->getSpot();

			if(this->spot_upward_map[dest_spot] == NULL)
			{
				LOG(this->log_receive, LEVEL_ERROR,
						"spot_upward of spot %d does'nt exist",
						dest_spot);
				break;
			}
			uint8_t msg_type = dvb_frame->getMessageType();
			LOG(this->log_receive, LEVEL_INFO,
			    "DVB frame received with type %u\n", msg_type);
			switch(msg_type)
			{
				// burst
				case MSG_TYPE_BBFRAME:
					// ignore BB frames in transparent scenario
					// (this is required because the GW may receive BB frames
					//  in transparent scenario due to carrier emulation)
					if(this->spot_upward_map[dest_spot]->getReceptionStd()->getType() == "DVB-RCS")
					{
						LOG(this->log_receive, LEVEL_INFO,
						    "ignore receINFO frame in transparent "
						    "scenario\n");
						goto drop;
					}
					// breakthrough
				case MSG_TYPE_DVB_BURST:
				case MSG_TYPE_CORRUPTED:
				{
					NetBurst *burst = NULL;

					// Update stats
					this->spot_upward_map[dest_spot]->setL2FromSatBytes(
					      this->spot_upward_map[dest_spot]->getL2FromSatBytes() +
					      dvb_frame->getPayloadLength());

					if(this->with_phy_layer)
					{
						DvbFrame *copy = new DvbFrame(dvb_frame);
						this->shareFrame(copy);
					}

					if(!this->spot_upward_map[dest_spot]->getReceptionStd()->onRcvFrame(
					          dvb_frame,
					          this->spot_upward_map[dest_spot]->getMacId(),
					          &burst))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "failed to handle DVB frame or BB frame\n");
						goto error;
					}
					if(this->spot_upward_map[dest_spot]->getReceptionStd()->getType() == "DVB-S2")
					{
						DvbS2Std *std = (DvbS2Std *)this->spot_upward_map[dest_spot]->getReceptionStd();
						if(msg_type != MSG_TYPE_CORRUPTED)
						{
							this->spot_upward_map[dest_spot]->getProbeReceivedModcod()->put(
									std->getReceivedModcod());
						}
						else
						{
							this->spot_upward_map[dest_spot]->getProbeRejectedModcod()->put(
									std->getReceivedModcod());
						}
					}

					// send the message to the upper layer
					if(burst && !this->enqueueMessage((void **)&burst))
					{
						LOG(this->log_send, LEVEL_ERROR,
						    "failed to send burst of packets to upper layer\n");
						delete burst;
						goto error;
					}
					LOG(this->log_send, LEVEL_INFO,
							"burst sent to the upper layer\n");
				}
				break;

				case MSG_TYPE_SAC:
				{
					if(!this->shareFrame(dvb_frame))
					{
						goto error;
					}
					break;

					case MSG_TYPE_SESSION_LOGON_REQ:
					LOG(this->log_receive, LEVEL_INFO,
							"Logon Req\n");
					if(!this->spot_upward_map[dest_spot]->onRcvLogonReq(dvb_frame))
					{
						goto error;
					}

					if(!this->shareFrame(dvb_frame))
					{
						goto error;
					}
					break;

					case MSG_TYPE_SESSION_LOGOFF:
					LOG(this->log_receive, LEVEL_INFO,
							"Logoff Req\n");
					if(!this->shareFrame(dvb_frame))
					{
						goto error;
					}
					break;
				}

				case MSG_TYPE_TTP:
				case MSG_TYPE_SESSION_LOGON_RESP:
					// nothing to do in this case
					LOG(this->log_receive, LEVEL_DEBUG,
					    "ignore TTP, logon response or SOF frame "
					    "(type = %d)\n", dvb_frame->getMessageType());
					delete dvb_frame;
					break;

				case MSG_TYPE_SOF:
					{
						this->spot_upward_map[dest_spot]->updateStats();
						// if Slotted Aloha is enabled handled Slotted Aloha scheduling here.
						if(this->spot_upward_map[dest_spot]->getSaloha())
						{
							uint16_t sfn;
							// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
							Sof *sof = (Sof *)dvb_frame;

							sfn = sof->getSuperFrameNumber();

							list<DvbFrame *> *ack_frames = new list<DvbFrame *>();
							// increase the superframe number and reset
							// counter of frames per superframe
							this->super_frame_counter++;
							if(this->super_frame_counter != sfn)
							{
								LOG(this->log_receive, LEVEL_WARNING,
								    "superframe counter (%u) is not the same as in SoF (%u)\n",
								    this->super_frame_counter, sfn);
								this->super_frame_counter = sfn;
							}

							// Slotted Aloha
							NetBurst* sa_burst = NULL;
							if(!this->spot_upward_map[dest_spot]->getSaloha()->schedule(&sa_burst,
								                                                        *ack_frames,
								                                                        this->super_frame_counter))
							{
								LOG(this->log_saloha, LEVEL_ERROR,
								    "failed to schedule Slotted Aloha\n");
								delete ack_frames;
								return false;
							}
							if(sa_burst &&
									!this->enqueueMessage((void **)&sa_burst))
							{
								LOG(this->log_saloha, LEVEL_ERROR,
								    "Failed to send encapsulation packets to upper layer\n");
								delete ack_frames;
								return false;
							}
							if(ack_frames->size() &&
									!this->shareMessage((void **)&ack_frames,
										sizeof(ack_frames),
										msg_saloha))
							{
								LOG(this->log_saloha, LEVEL_ERROR,
								    "Failed to send Slotted Aloha acks to opposite layer\n");
								delete ack_frames;
								return false;
							}
							// delete ack_frames if they are emtpy, else shareMessage
							// would set ack_frames == NULL
							if(ack_frames)
							{
								delete ack_frames;
							}
						}
						else
						{
							// nothing to do in this case
							LOG(this->log_receive, LEVEL_DEBUG,
							    "ignore SOF frame (type = %d)\n",
							    dvb_frame->getMessageType());
						}
						delete dvb_frame;
					}
					break;

					// Slotted Aloha
				case MSG_TYPE_SALOHA_DATA:
					// Update stats
					this->spot_upward_map[dest_spot]->setL2FromSatBytes(
							this->spot_upward_map[dest_spot]->getL2FromSatBytes() +
							dvb_frame->getPayloadLength());

					if(!this->spot_upward_map[dest_spot]->getSaloha()->onRcvFrame(dvb_frame))
					{
						LOG(this->log_saloha, LEVEL_ERROR,
						    "failed to handle Slotted Aloha frame\n");
						goto error;
					}
					break;

				case MSG_TYPE_SALOHA_CTRL:
					delete dvb_frame;
					break;

				default:
					LOG(this->log_receive, LEVEL_ERROR,
					    "unknown type (%d) of DVB frame\n",
					    dvb_frame->getMessageType());
					delete dvb_frame;
					goto error;
					break;
			}

		}
		break;

		default:
		LOG(this->log_receive, LEVEL_ERROR,
		    "unknown event received %s",
		    event->getName().c_str());
		return false;
	}

	return true;

drop:
	delete dvb_frame;
	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u\n",
	    this->super_frame_counter);
	return false;


}

bool BlockDvbNcc::Upward::shareFrame(DvbFrame *frame)
{
	if(!this->shareMessage((void **)&frame, sizeof(*frame), msg_sig))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit frame to opposite channel\n");
		delete frame;
		return false;
	}
	return true;
}


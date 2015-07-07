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
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 */


#include "BlockDvbNcc.h"

#include "SpotUpwardTransp.h"
#include "SpotDownwardTransp.h"
#include "SpotUpwardRegen.h"
#include "SpotDownwardRegen.h"
#include "DvbRcsFrame.h"
#include "Sof.h"

#include <errno.h>

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


BlockDvbNcc::BlockDvbNcc(const string &name, tal_id_t UNUSED(mac_id)):
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

BlockDvbNcc::Downward::Downward(Block *const bl, tal_id_t mac_id):
	DvbDownward(bl),
	NccPepInterface(),
	mac_id(mac_id),
	fwd_frame_counter(0),
	fwd_timer(-1),
	probe_frame_interval(NULL)
{
}

BlockDvbNcc::Downward::~Downward()
{
}


bool BlockDvbNcc::Downward::onInit(void)
{
	bool result = true;
	const char *scheme;
	map<spot_id_t, DvbChannel *>::iterator spot_iter;

	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed get satellite type\n");
		return false;
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

	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		return false;
	}

	if(!this->initCommon(scheme))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}
	
	for(spot_iter = this->spots.begin(); 
	    spot_iter != this->spots.end(); ++spot_iter)
	{
		SpotDownward *spot;
		spot_id_t spot_id = (*spot_iter).first;
		LOG(this->log_init, LEVEL_DEBUG,
		    "Create spot with ID %u\n", spot_id);
		if(this->satellite_type == TRANSPARENT)
		{	
			spot = new SpotDownwardTransp(spot_id, this->mac_id,
			                              this->fwd_down_frame_duration_ms,
			                              this->ret_up_frame_duration_ms,
			                              this->stats_period_ms,
			                              this->satellite_type,
			                              this->pkt_hdl,
			                              this->with_phy_layer);

		}
		else
		{
			spot = new SpotDownwardRegen(spot_id, this->mac_id,
			                             this->fwd_down_frame_duration_ms,
			                             this->ret_up_frame_duration_ms,
			                             this->stats_period_ms,
			                             this->satellite_type,
			                             this->pkt_hdl,
			                             this->with_phy_layer);

			
		}
		(*spot_iter).second = spot;
		result &= spot->onInit();
	}

	// initialize the timers
	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the timers part of the "
		    "initialisation\n");
		return false;
	}
	for(spot_iter = this->spots.begin(); 
	    spot_iter != this->spots.end(); ++spot_iter)
	{
		SpotDownward *spot;
		spot = dynamic_cast<SpotDownward *>((*spot_iter).second);
		DFLTLOG(LEVEL_ERROR, "je suis la\n");
		//this->setDuration(spot->getModcodTimer(), 8000);
		this->raiseTimer(spot->getModcodTimer());
		DFLTLOG(LEVEL_ERROR, "je suis la\n");
	}

	// listen for connections from external PEP components
	if(!this->initPepSocket())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to listen for PEP connections\n");
		return false;
	}
	this->addTcpListenEvent("pep_listen",
	                        this->getPepListenSocket(), 200);

	// Output probes and stats
	this->probe_frame_interval = Output::registerProbe<float>("ms", true,
	                                                          SAMPLE_LAST,
	                                                          "Perf.Frames_interval");

	return result;
}

bool BlockDvbNcc::Downward::initTimers(void)
{
	map<spot_id_t, DvbChannel *>::iterator spot_iter;
	
	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	this->frame_timer = this->addTimerEvent("frame",
	                                        this->ret_up_frame_duration_ms);
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                      this->fwd_down_frame_duration_ms);


	// read the pep allocation delay
	if(!Conf::getValue(Conf::section_map[NCC_SECTION_PEP], DVB_NCC_ALLOC_DELAY,
	                   this->pep_alloc_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY);
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "pep_alloc_delay set to %d ms\n", this->pep_alloc_delay);
	// create timer
	
	for(spot_iter = this->spots.begin(); 
		spot_iter != this->spots.end(); ++spot_iter)
	{
		SpotDownward *spot;
		spot = dynamic_cast<SpotDownward *>((*spot_iter).second);
		if(!spot)
		{
			LOG(this->log_receive, LEVEL_WARNING,
		        "Error when getting spot\n");
			return false;
		}
		spot->setPepCmdApplyTimer(this->addTimerEvent("pep_request",
		                                              pep_alloc_delay,
		                                              false, // no rearm
		                                              false // do not start
		                                              ));

		// Launch the timer in order to retrieve the modcods if there is no physical layer
		// or to send SAC with ACM parameters in regenerative mode
		if(!this->with_phy_layer || this->satellite_type == REGENERATIVE)
		{
			spot->setModcodTimer(this->addTimerEvent("scenario",
			                                         5000, // the duration will be change when started
			                                         false, // no rearm
			                                         false // do not start
			                                         ));
		}
	}

	return true;
}


bool BlockDvbNcc::Downward::onEvent(const RtEvent *const event)
{
	map<spot_id_t, DvbChannel *>::iterator spot_iter;

	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = NULL;
			// first handle specific messages
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
				
				spot_id_t dest_spot = dvb_frame->getSpot();
				uint8_t msg_type = dvb_frame->getMessageType();
				SpotDownward *spot;
				spot = dynamic_cast<SpotDownward *>(this->getSpot(dest_spot));
				if(!spot)
				{
			        LOG(this->log_receive, LEVEL_WARNING,
        		        "Error when getting spot\n");
					delete dvb_frame;
					return false;
				}

				switch(msg_type)
				{
					case MSG_TYPE_BBFRAME:
					case MSG_TYPE_DVB_BURST:
					case MSG_TYPE_CORRUPTED:
						if(!spot->handleCorruptedFrame(dvb_frame))
						{
							goto error;
						}
						break;

					case MSG_TYPE_SAC: // when physical layer is enabled
						if(!spot->handleSac(dvb_frame))
						{
							goto error;
						}
						break;

					case MSG_TYPE_SESSION_LOGON_REQ:
						if(!this->handleLogonReq(dvb_frame, spot))
						{
							goto error;
						}
						break;

					case MSG_TYPE_SESSION_LOGOFF:
						if(!spot->handleLogoffReq(dvb_frame))
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
				spot_id_t spot_id = ack_frames->front()->getSpot();
				SpotDownward *spot;
				spot = dynamic_cast<SpotDownward *>(this->getSpot(spot_id));
				if(!spot)
				{
			        LOG(this->log_receive, LEVEL_WARNING,
        		        "Error when getting spot\n");
					delete ack_frames;
					return false;
				}

				spot->handleSalohaAcks(ack_frames);
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
					tal_id_t tal_id = (*pkt_it)->getDstTalId();
					spot_id_t spot_id = 0;
					SpotDownward *spot;
					list<SpotDownward*> spot_list;
					list<SpotDownward*>::iterator spot_list_iter;

					if((tal_id == BROADCAST_TAL_ID) and 
					   (this->satellite_type != REGENERATIVE))
					{
						for(spot_iter = this->spots.begin(); 
						    spot_iter != this->spots.end(); ++spot_iter)
						{
							spot = dynamic_cast<SpotDownward *>(
							            this->getSpot((*spot_iter).first));
            				if(!spot)
            				{
            			        LOG(this->log_receive, LEVEL_WARNING,
                    		        "Error when getting spot\n");
            					return false;
            				}
							spot_list.push_back(spot);
						}
					}
					else
					{ 
						if(OpenSandConf::spot_table.find(tal_id) ==
								OpenSandConf::spot_table.end())
						{
							spot_id = this->default_spot;
						}
						else
						{
							spot_id = OpenSandConf::spot_table[tal_id];
						}
						spot = dynamic_cast<SpotDownward *>(this->getSpot(spot_id));
        				if(!spot)
        				{
        			        LOG(this->log_receive, LEVEL_WARNING,
                		        "Error when getting spot\n");
        					return false;
        				}
						spot_list.push_back(spot);
					}

					for(spot_list_iter = spot_list.begin() ; 
					    spot_list_iter != spot_list.end() ;
					    ++spot_list_iter)
					{
						NetPacket *pkt_copy;
						spot = *spot_list_iter;
						if(spot_list.size() > 1 )
						{
							pkt_copy = new NetPacket(*pkt_it);
						}
						else
						{
							pkt_copy = *pkt_it;
						}

						if(!spot->handleEncapPacket(pkt_copy))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "cannot push burst into fifo\n");
							continue;
						}
					}
					// free the iterator that was not send if there is
					// more than on spot
					if(spot_list.size() > 1)
					{
						delete *pkt_it;
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
			}

    		bool find_pep = false; 
			for(spot_iter = this->spots.begin(); 
			    spot_iter != this->spots.end(); ++spot_iter)
			{
				SpotDownward *spot;
				spot = dynamic_cast<SpotDownward *>((*spot_iter).second);
				if(!spot)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					       "Error when getting spot\n");
				    return false;
				}
				
			    if(*event == this->frame_timer)
    			{
					// send Start Of Frame
					this->sendSOF(spot->getSofCarrierId());
					
					if(spot->checkDama())
					{
						continue;
					}

					if(!spot->handleFrameTimer(this->super_frame_counter))
					{
						// do not quit if this fail in one spot
						continue;
					}

					// send TTP computed by DAMA
					this->sendTTP(spot);
				}

				else if(*event == this->fwd_timer)
				{
					this->fwd_frame_counter++;
					if(!spot->handleFwdFrameTimer(this->fwd_frame_counter))
					{
						// do not break if this fail in one spot
						continue;
					}

					// send the scheduled frames
					if(!this->sendBursts(&spot->getCompleteDvbFrames(),
					                     spot->getDataCarrierId()))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "failed to build and send DVB/BB "
						    "frames\n");
						// do not break if this fail in one spot
						continue;
					}
				}
				else if(*event == spot->getModcodTimer())
				{
					// if regenerative satellite and physical layer scenario,
					// send ACM parameters
					if(this->satellite_type == REGENERATIVE &&
					   this->with_phy_layer)
					{
						this->sendAcmParameters(spot);
					}

					// it's time to update MODCOD IDs
					LOG(this->log_receive, LEVEL_ERROR,
					    "MODCOD scenario timer received\n");
					LOG(this->log_receive, LEVEL_DEBUG,
					    "MODCOD scenario timer received\n");
					
					double duration_up_ret, duration_down_fwd;
					if(spot->goNextScenarioStep(duration_up_ret, duration_down_fwd))
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
					spot->updateFmt();
					DFLTLOG(LEVEL_ERROR, "duration = %f\n",
					        duration_up_ret);
					if(duration_up_ret <= 0)
					{
						// we hare reach the end of the file (of it is malformed)
						// so we keep the modcod as they are
						this->removeEvent(spot->getModcodTimer());
					}
					else
					{
						this->setDuration(spot->getModcodTimer(), duration_up_ret);
						this->startTimer(spot->getModcodTimer());
					}
				}
    			else if(*event == spot->getPepCmdApplyTimer())
				{
					// it is time to apply the command sent by the external
					// PEP component
	
					PepRequest *pep_request;

					LOG(this->log_receive, LEVEL_NOTICE,
					    "apply PEP requests now\n");
					while((pep_request = this->getNextPepRequest()) != NULL)
					{
						spot->applyPepCommand(pep_request);
					}
					find_pep = true;
					break;
				}

	    		if(*event == spot->getPepCmdApplyTimer() && !find_pep)
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "unknown timer event received %s\n",
					    event->getName().c_str());
					return false;
				}
			}
			break;
		}
		case evt_net_socket:
		{
			if(*event == this->getPepClientSocket())
			{
				// event received on PEP client socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP client socket\n");

				tal_id_t tal_id;
				spot_id_t spot_id;
				if(OpenSandConf::spot_table.find(tal_id) == OpenSandConf::spot_table.end())
				{
					spot_id = this->default_spot;
				}
				else
				{
					spot_id = OpenSandConf::spot_table[tal_id];
				}

				spot_iter = spots.find(spot_id);
				if(spot_iter == spots.end())
				{
					LOG(this->log_receive, LEVEL_ERROR, 
					    "couldn't find spot %d", 
					    OpenSandConf::spot_table[tal_id]);
					return false;
				}
				SpotDownward *spot;
				spot = dynamic_cast<SpotDownward *>((*spot_iter).second);
				if(!spot)
				{
				    LOG(this->log_receive, LEVEL_WARNING,
				        "Error when getting spot\n");
					return false;
				}

				// read the message sent by PEP or delete socket
				// if connection is dead
				if(this->readPepMessage((NetSocketEvent *)event, tal_id) == true)
				{
					// we have received a set of commands from the
					// PEP component, let's apply the resources
					// allocations/releases they contain

					// set delay for applying the commands
					if(this->getPepRequestType() == PEP_REQUEST_ALLOCATION)
					{
						if(!this->startTimer(spot->getPepCmdApplyTimer()))
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
						this->raiseTimer(spot->getPepCmdApplyTimer());
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
					// Free the socket
					if(shutdown(this->getPepClientSocket(), SHUT_RDWR) != 0)
					{
						LOG(this->log_init, LEVEL_ERROR,
						    "failed to clase socket: "
						    "%s (%d)\n", strerror(errno), errno);
					}
					this->removeEvent(this->getPepClientSocket());
				}
				else
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "network problem encountered with PEP, "
					    "connection was therefore closed\n");
					// Free the socket
					if(shutdown(this->getPepClientSocket(), SHUT_RDWR) != 0)
					{
						LOG(this->log_init, LEVEL_ERROR,
						    "failed to clase socket: "
						    "%s (%d)\n", strerror(errno), errno);
					}
					this->removeEvent(this->getPepClientSocket());
					return false;
				}
			}
			break;
		}
		case evt_tcp_listen:
		{
			if(*event == this->getPepListenSocket())
			{
				this->setSocketClient(((TcpListenEvent *)event)->getSocketClient());
				this->setIsConnected(true);

				// event received on PEP listen socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP listen socket\n");

				LOG(this->log_receive, LEVEL_NOTICE,
				    "NCC is now connected to PEP\n");
				// add a fd to handle events on the client socket
				this->addNetSocketEvent("pep_client",
				                        this->getPepClientSocket(),
				                        200);
			}
			break;
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

void BlockDvbNcc::Downward::sendSOF(unsigned int carrier_id)
{
	Sof *sof = new Sof(this->super_frame_counter);

	// Send it
	if(!this->sendDvbFrame((DvbFrame *)sof, carrier_id))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to call sendDvbFrame() for SOF\n");
		delete sof;
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: SOF sent\n", this->super_frame_counter);
}

void BlockDvbNcc::Downward::sendTTP(SpotDownward *spot)
{
	Ttp *ttp = new Ttp(this->mac_id, this->super_frame_counter);
	// Build TTP
	if(!spot->buildTtp(ttp))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_DEBUG,
		    "Dama didn't build TTP\bn");
		return;
	};

	if(!this->sendDvbFrame((DvbFrame *)ttp, spot->getCtrlCarrierId()))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send TTP\n");
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: TTP sent\n", this->super_frame_counter);
}

bool BlockDvbNcc::Downward::handleLogonReq(DvbFrame *dvb_frame,
                                           SpotDownward *spot)
{
	//TODO find why dynamic cast fail here and each time we do that on frames !?
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();
	LogonResponse *logon_resp;

	// Inform the Dama controller (for its own context)
	if(!spot->handleLogonReq(logon_req))
	{
		goto release;
	}
	
	logon_resp = new LogonResponse(mac, this->mac_id, mac);

	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u: logon response sent to lower layer\n",
	    this->super_frame_counter);


	if(!this->sendDvbFrame((DvbFrame *)logon_resp,
	                       spot->getCtrlCarrierId()))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed send logon response\n");
		return false;
	}

	return true;

release:
	delete dvb_frame;
	return false;
}


bool BlockDvbNcc::Downward::sendAcmParameters(SpotDownward *spot_downward)
{
	Sac *send_sac = new Sac(this->mac_id);
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

BlockDvbNcc::Upward::Upward(Block *const bl, tal_id_t mac_id):
	DvbUpward(bl),
	mac_id(mac_id),
	log_saloha(NULL)
{
}


BlockDvbNcc::Upward::~Upward()
{
}


bool BlockDvbNcc::Upward::onInit(void)
{
	bool result = true;
	map<spot_id_t, DvbChannel *>::iterator spot_iter;
	
	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed get satellite type\n");
		return false;
	}
	
	if(!this->initSpots())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the spot "
		    "initialisation\n");
		return false;
	}
	
	for(spot_iter = this->spots.begin(); 
	    spot_iter != this->spots.end(); ++spot_iter)
	{
		spot_id_t spot_id = (*spot_iter).first;
		SpotUpward *spot;
		if(this->satellite_type == TRANSPARENT)
		{	
			spot = new SpotUpwardTransp(spot_id, this->mac_id);
		}
		else
		{
			spot = new SpotUpwardRegen(spot_id, this->mac_id);
		}
		LOG(this->log_init, LEVEL_DEBUG,
		    "Create spot with ID %u\n", spot_id);

		(*spot_iter).second = spot;

		result &= spot->onInit();

	}

	if(result)
	{
		T_LINK_UP *link_is_up;
		// create and send a "link is up" message to upper layer
		link_is_up = new T_LINK_UP;
		if(link_is_up == NULL)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed to allocate memory for link_is_up "
			    "message\n");
			return false;
		}
		link_is_up->group_id = this->mac_id;
		link_is_up->tal_id = this->mac_id;

		if(!this->enqueueMessage((void **)(&link_is_up),
		                         sizeof(T_LINK_UP),
		                         msg_link_up))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to send link up message to upper layer\n");
			delete link_is_up;
		}
	}

	LOG(this->log_init_channel, LEVEL_DEBUG,
	    "Link is up msg sent to upper layer\n");

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
			SpotUpward *spot;
			spot = dynamic_cast<SpotUpward *>(this->getSpot(dest_spot));
			if(!spot)
			{
				LOG(this->log_receive, LEVEL_WARNING,
			        "Error when getting spot\n");
				delete dvb_frame;
				return false;
			}
			uint8_t msg_type = dvb_frame->getMessageType();
			LOG(this->log_receive, LEVEL_INFO,
			    "DVB frame received with type %u\n", msg_type);
			switch(msg_type)
			{
				// burst
				case MSG_TYPE_BBFRAME:
				case MSG_TYPE_DVB_BURST:
				case MSG_TYPE_CORRUPTED:
				{
					NetBurst *burst = NULL;
					if(!spot->handleFrame(dvb_frame, &burst))
					{
						return false;
					}

					// Transmit frame to opposite block for physical layer
					// C/N0 updates
					if(this->with_phy_layer)
					{
						this->shareFrame(dvb_frame);
					}

					// send the message to the upper layer
					if(burst && !this->enqueueMessage((void **)&burst))
					{
						LOG(this->log_send, LEVEL_ERROR,
						    "failed to send burst of packets to upper layer\n");
						delete burst;
						return false;
					}
					LOG(this->log_send, LEVEL_INFO,
					    "burst sent to the upper layer\n");
				}
				break;

				case MSG_TYPE_SAC:
					if(!this->shareFrame(dvb_frame))
					{
						return false;
					}
					break;

				case MSG_TYPE_SESSION_LOGON_REQ:
					LOG(this->log_receive, LEVEL_INFO,
					    "Logon Req\n");
					if(!spot->onRcvLogonReq(dvb_frame))
					{
						return false;
					}

					if(!this->shareFrame(dvb_frame))
					{
						return false;
					}
					break;

				case MSG_TYPE_SESSION_LOGOFF:
					LOG(this->log_receive, LEVEL_INFO,
					    "Logoff Req\n");
					if(!this->shareFrame(dvb_frame))
					{
						return false;
					}
					break;

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
					spot->updateStats();
					list<DvbFrame *> *ack_frames = NULL;
					NetBurst *sa_burst = NULL;

					if(!spot->scheduleSaloha(dvb_frame, ack_frames, &sa_burst))
					{
						return false;
					}
					delete dvb_frame;
					
					if(!ack_frames && !sa_burst)
					{
						// No slotted Aloha
						break;
					}
					if(sa_burst && !this->enqueueMessage((void **)&sa_burst))
					{
						LOG(this->log_saloha, LEVEL_ERROR,
						    "Failed to send encapsulation packets to upper"
						    " layer\n");
						if(ack_frames)
						{
							delete ack_frames;
						}
						return false;
					}
					if(ack_frames->size() &&
					   !this->shareMessage((void **)&ack_frames,
					                       sizeof(ack_frames),
					                       msg_saloha))
					{
						LOG(this->log_saloha, LEVEL_ERROR,
						    "Failed to send Slotted Aloha acks to opposite"
						    " layer\n");
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
				break;

					// Slotted Aloha
				case MSG_TYPE_SALOHA_DATA:
					if(!spot->handleSlottedAlohaFrame(dvb_frame))
					{
						return false;
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
					return false;
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



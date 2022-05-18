/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */


#include "BlockDvbNcc.h"

#include "OpenSandModelConf.h"
#include "SpotUpward.h"
#include "SpotDownward.h"
#include "SpotUpwardTransp.h"
#include "SpotDownwardTransp.h"
#include "DvbRcsFrame.h"
#include "Sof.h"

#include <errno.h>
#include <opensand_rt/TcpListenEvent.h>


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/

BlockDvbNcc::BlockDvbNcc(const string &name, tal_id_t mac_id):
	BlockDvb(name),
	mac_id(mac_id),
	output_sts_list(),
	input_sts_list()
{
}

BlockDvbNcc::~BlockDvbNcc()
{
	map<spot_id_t, StFmtSimuList *>::iterator it;

	for(it = this->output_sts_list.begin();
	    it != this->output_sts_list.end(); ++it)
	{
		delete (*it).second;
	}

	for(it = this->input_sts_list.begin();
	    it != this->input_sts_list.end(); ++it)
	{
		delete (*it).second;
	}
	this->output_sts_list.clear();
	this->input_sts_list.clear();
}

void BlockDvbNcc::generateConfiguration()
{
	SpotDownward::generateConfiguration();
	SpotDownwardTransp::generateConfiguration();
	SpotUpwardTransp::generateConfiguration();
}

bool BlockDvbNcc::onInit(void)
{
	if(!this->initListsSts())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize the lists of Sts\n");
		return false;
	}

	return true;
}


bool BlockDvbNcc::initListsSts()
{
	// TODO: Remove obsolete lists (from multi-spot)
	if(this->input_sts_list.find(this->mac_id) == this->input_sts_list.end())
	{
		this->input_sts_list[this->mac_id] = new StFmtSimuList("in");
	}
	if(this->input_sts_list[this->mac_id] == NULL)
	{
		return false;
	}

	if(this->output_sts_list.find(this->mac_id) == this->output_sts_list.end())
	{
		this->output_sts_list[this->mac_id] = new StFmtSimuList("out");
	}
	if(this->output_sts_list[this->mac_id] == NULL)
	{
		return false;
	}

	// input and output sts are shared between up and down
	// and protected by a mutex
	((Upward *)this->upward)->setStFmt(this->output_sts_list,
	                                   this->input_sts_list);
	((Downward *)this->downward)->setStFmt(this->output_sts_list,
	                                       this->input_sts_list);

	return true;
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/


// TODO lot of duplicated code for fifos between ST and GW

BlockDvbNcc::Downward::Downward(const string &name, tal_id_t mac_id):
	DvbDownward(name),
	DvbSpotList(mac_id),
	pep_interface(),
	svno_interface(),
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
	int pep_tcp_port, svno_tcp_port;
	map<spot_id_t, DvbChannel *>::iterator spot_iter;

	// get the common parameters
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		return false;
	}

	if(!this->initCommon(FORWARD_DOWN_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initSpotList())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the spot "
		    "initialisation\n");
		return false;
	}

	for(spot_iter = this->spots.begin(); 
	    spot_iter != this->spots.end(); ++spot_iter)
	{
		SpotDownward *spot;
		spot_id_t spot_id = (*spot_iter).first;
		StFmtSimuList *input_sts = this->getInputStFmt(spot_id);
		StFmtSimuList *output_sts = this->getOutputStFmt(spot_id);

		LOG(this->log_init, LEVEL_DEBUG,
		    "Create downward spot with ID %u\n", spot_id);

		if(!input_sts || !output_sts)
		{
			return false;
		}
		
		spot = new SpotDownwardTransp(spot_id, this->mac_id,
		                              this->fwd_down_frame_duration_ms,
		                              this->ret_up_frame_duration_ms,
		                              this->stats_period_ms,
		                              this->pkt_hdl,
		                              input_sts,
		                              output_sts);

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

	// retrieve the TCP communication port dedicated
	// for NCC/PEP and NCC/SVNO communications
	if(!OpenSandModelConf::Get()->getNccPorts(pep_tcp_port, svno_tcp_port))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section 'ncc': missing parameter 'pep port' or 'svno port'\n");
		return false;
	}

	// listen for connections from external PEP components
	if(!this->pep_interface.initPepSocket(pep_tcp_port))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to listen for PEP connections\n");
		return false;
	}
	this->addTcpListenEvent("pep_listen",
	                        this->pep_interface.getPepListenSocket(), 200);

	// listen for connections from external SVNO components
	if(!this->svno_interface.initSvnoSocket(svno_tcp_port))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to listen for SVNO connections\n");
		return false;
	}
	this->addTcpListenEvent("svno_listen",
	                        this->svno_interface.getSvnoListenSocket(), 200);

	// Output probes and stats
	this->probe_frame_interval = Output::Get()->registerProbe<float>("Perf.Frames_interval",
	                                                                 "ms", true,
	                                                                 SAMPLE_LAST);

	return result;
}

bool BlockDvbNcc::Downward::initTimers(void)
{
	map<spot_id_t, DvbChannel *>::iterator spot_iter;
	time_ms_t acm_period_ms;

	auto Conf = OpenSandModelConf::Get();

	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	this->frame_timer = this->addTimerEvent("frame",
	                                        this->ret_up_frame_duration_ms);
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                      this->fwd_down_frame_duration_ms);


	// read the pep allocation delay
	if(!Conf->getPepAllocationDelay(this->pep_alloc_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'schedulers': missing parameter 'pep allocation delay'\n");
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "pep_alloc_delay set to %d ms\n", this->pep_alloc_delay);

	if(!Conf->getAcmRefreshPeriod(acm_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		   "section 'timers': missing parameter 'acm refresh period'\n");
		return false;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "ACM period set to %d ms\n",
	    acm_period_ms);

	// create timer
	for(spot_iter = this->spots.begin(); 
		spot_iter != this->spots.end(); ++spot_iter)
	{
		SpotDownward *spot;
		spot = dynamic_cast<SpotDownward *>((*spot_iter).second);
		if(!spot)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Error when getting spot %d\n", (*spot_iter).first);
			return false;
		}
		spot->setPepCmdApplyTimer(this->addTimerEvent("pep_request",
		                                              pep_alloc_delay,
		                                              false, // no rearm
		                                              false // do not start
		                                              ));
	}

	return true;
}


bool BlockDvbNcc::Downward::onEvent(const RtEvent *const event)
{
	map<spot_id_t, DvbChannel *>::iterator spot_iter;

	switch(event->getType())
	{
    case EventType::Message:
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
					SpotDownward *spot;
					list<SpotDownward*> spot_list;
					list<SpotDownward*>::iterator spot_list_iter;

					// FIXME at the moment broadcast is sent on all spots
					if(tal_id == BROADCAST_TAL_ID)
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
						spot = dynamic_cast<SpotDownward *>(this->getSpot(this->mac_id));
						if(!spot)
						{
							LOG(this->log_receive, LEVEL_WARNING,
							    "Error when getting spot\n");
							return false;
						}
						spot_list.push_back(spot);
					}

					for(spot_list_iter = spot_list.begin(); 
					    spot_list_iter != spot_list.end();
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
    case EventType::Timer:
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

					// Update Fmt here for TTP
					spot->updateFmt();

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
				else if(*event == spot->getPepCmdApplyTimer())
				{
					// it is time to apply the command sent by the external
					// PEP component

					PepRequest *pep_request;

					LOG(this->log_receive, LEVEL_NOTICE,
					    "apply PEP requests now\n");
					while((pep_request = this->pep_interface.getNextPepRequest()) != NULL)
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
		// TODO factorize, some elements are exactly the same between NccInterface classes
    case EventType::NetSocket:
		{
			if(*event == this->pep_interface.getPepClientSocket())
			{
				tal_id_t tal_id;

				// event received on PEP client socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP client socket\n");

				// read the message sent by PEP or delete socket
				// if connection is dead
				if(!this->pep_interface.readPepMessage((NetSocketEvent *)event, tal_id))
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "network problem encountered with PEP, "
					    "connection was therefore closed\n");
					// Free the socket
					if(shutdown(this->pep_interface.getPepClientSocket(), SHUT_RDWR) != 0)
					{
						LOG(this->log_init, LEVEL_ERROR,
						    "failed to clase socket: "
						    "%s (%d)\n", strerror(errno), errno);
					}
					this->removeEvent(this->pep_interface.getPepClientSocket());
					return false;
				}
				// we have received a set of commands from the
				// PEP component, let's apply the resources
				// allocations/releases they contain

				// first get the spot associated with this terminal
				spot_iter = this->spots.find(this->mac_id);
				if(spot_iter == this->spots.end())
				{
					LOG(this->log_receive, LEVEL_ERROR, 
					    "couldn't find spot of gw %d", 
					    this->mac_id);
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

				// set delay for applying the commands
				if(this->pep_interface.getPepRequestType() == PEP_REQUEST_ALLOCATION)
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
				else if(this->pep_interface.getPepRequestType() == PEP_REQUEST_RELEASE)
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
				if(shutdown(this->pep_interface.getPepClientSocket(), SHUT_RDWR) != 0)
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "failed to clase socket: "
					    "%s (%d)\n", strerror(errno), errno);
				}
				this->removeEvent(this->pep_interface.getPepClientSocket());
			}
			else if(*event == this->svno_interface.getSvnoClientSocket())
			{
				SvnoRequest *request;
				// event received on SVNO client socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on SVNO client socket\n");

				// read the message sent by SVNO or delete socket
				// if connection is dead
				if(!this->svno_interface.readSvnoMessage((NetSocketEvent *)event))
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "network problem encountered with SVNO, "
					    "connection was therefore closed\n");
					// Free the socket
					if(shutdown(this->svno_interface.getSvnoClientSocket(), SHUT_RDWR) != 0)
					{
						LOG(this->log_init, LEVEL_ERROR,
						    "failed to clase socket: "
						    "%s (%d)\n", strerror(errno), errno);
					}
					this->removeEvent(this->svno_interface.getSvnoClientSocket());
					return false;
				}
				// we have received a set of commands from the
				// SVNO component, let's apply the resources
				// allocations/releases they contain

				request = this->svno_interface.getNextSvnoRequest();
				while(request != NULL)
				{
					SpotDownward *spot;

					// first get the spot concerned by the request
					spot_iter = spots.find(request->getSpotId());
					if(spot_iter == spots.end())
					{
						LOG(this->log_receive, LEVEL_ERROR, 
						    "couldn't find spot %d", 
						    request->getSpotId());
						return false;
					}
					spot = dynamic_cast<SpotDownward *>((*spot_iter).second);
					if(!spot)
					{
					    LOG(this->log_receive, LEVEL_WARNING,
					        "Error when getting spot\n");
						return false;
					}

					if(!spot->applySvnoCommand(request))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "Cannot apply SVNO interface request\n");
						return false;
					}
					delete request;
					request = this->svno_interface.getNextSvnoRequest();
				}
			}
		}
		break;

    case EventType::TcpListen:
		{
			if(*event == this->pep_interface.getPepListenSocket())
			{
				this->pep_interface.setSocketClient(((TcpListenEvent *)event)->getSocketClient());
				this->pep_interface.setIsConnected(true);

				// event received on PEP listen socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on PEP listen socket\n");

				LOG(this->log_receive, LEVEL_NOTICE,
				    "NCC is now connected to PEP\n");
				// add a fd to handle events on the client socket
				this->addNetSocketEvent("pep_client",
				                        this->pep_interface.getPepClientSocket(),
				                        200);
			}
			else if(*event == this->svno_interface.getSvnoListenSocket())
			{
				this->svno_interface.setSocketClient(((TcpListenEvent *)event)->getSocketClient());
				this->svno_interface.setIsConnected(true);

				// event received on SVNO listen socket
				LOG(this->log_receive, LEVEL_NOTICE,
				    "event received on SVNO listen socket\n");

				// create the client socket to receive messages
				LOG(this->log_receive, LEVEL_NOTICE,
				    "NCC is now connected to SVNO\n");
				// add a fd to handle events on the client socket
				this->addNetSocketEvent("svno_client",
				                        this->svno_interface.getSvnoClientSocket(),
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
	
	// TODO only used here tal_id and logon_id are the same
	// may be we can simplify the constructor
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

// updateStats is pure virtual in BlockDvb not used in this case
void BlockDvbNcc::Downward::updateStats(void)
{
}


/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbNcc::Upward::Upward(const string &name, tal_id_t mac_id):
	DvbUpward(name),
	DvbSpotList(mac_id),
	mac_id(mac_id),
	log_saloha(NULL),
	probe_gw_received_modcod(NULL),
	probe_gw_rejected_modcod(NULL)
{
}


BlockDvbNcc::Upward::~Upward()
{
}


bool BlockDvbNcc::Upward::onInit(void)
{
	bool result = true;
	map<spot_id_t, DvbChannel *>::iterator spot_iter;

	if(!this->initSpotList())
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
		StFmtSimuList *input_sts = this->getInputStFmt(spot_id);
		StFmtSimuList *output_sts = this->getOutputStFmt(spot_id);

		LOG(this->log_init, LEVEL_DEBUG,
		    "Create upward spot with ID %u\n", spot_id);

		spot = new SpotUpwardTransp(spot_id, this->mac_id,
		                            input_sts, output_sts);
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

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output\n");
		return false;
	}

	LOG(this->log_init_channel, LEVEL_DEBUG,
	    "Link is up msg sent to upper layer\n");

	// everything went fine
	return result;
}


bool BlockDvbNcc::Upward::initOutput(void)
{
	auto output = Output::Get();

	this->probe_gw_received_modcod = output->registerProbe<int>("Down_Return_modcod.Received_modcod",
								    "modcod index",
								    true, SAMPLE_LAST);
	this->probe_gw_rejected_modcod = output->registerProbe<int>("Down_Return_modcod.Rejected_modcod",
								    "modcod index",
								    true, SAMPLE_LAST);

	return true;
}

bool BlockDvbNcc::Upward::onEvent(const RtEvent *const event)
{
	DvbFrame *dvb_frame = NULL;

	switch(event->getType())
	{
    case EventType::Message:
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

			fmt_id_t modcod_id = 0;
			uint8_t msg_type = dvb_frame->getMessageType();
			LOG(this->log_receive, LEVEL_INFO,
			    "DVB frame received with type %u\n", msg_type);

			if(msg_type == MSG_TYPE_BBFRAME)
			{
				BBFrame *bbframe = (BBFrame *)dvb_frame;
				modcod_id = bbframe->getModcodId();
			}
			else if(msg_type == MSG_TYPE_DVB_BURST)
			{
				DvbRcsFrame *dvb_rcs_frame = (DvbRcsFrame *)dvb_frame;
				modcod_id = dvb_rcs_frame->getModcodId();
			}
			switch(msg_type)
			{
				// burst
				case MSG_TYPE_BBFRAME:
				case MSG_TYPE_DVB_BURST:
				{
					// Update C/N0
					spot->handleFrameCni(dvb_frame);

					bool corrupted = dvb_frame->isCorrupted();
					if(!corrupted)
					{
						// update MODCOD probes
						this->probe_gw_received_modcod->put(modcod_id);
						this->probe_gw_rejected_modcod->put(0);
					}
					else
					{
						this->probe_gw_rejected_modcod->put(modcod_id);
						this->probe_gw_received_modcod->put(0);
					}

					NetBurst *burst = NULL;
					if(!spot->handleFrame(dvb_frame, &burst))
					{
						return false;
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
				{
					// Update C/N0
					Sac *UNUSED(sac) = (Sac *) dvb_frame;
					spot->handleFrameCni(dvb_frame);
					if(!spot->handleSac(dvb_frame))
					{
						return false;
					}
					if(!this->shareFrame(dvb_frame))
					{
						return false;
					}
				}
				break;

				case MSG_TYPE_SESSION_LOGON_REQ:
				{
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
				}
				break;

				case MSG_TYPE_SESSION_LOGOFF:
				{
					LOG(this->log_receive, LEVEL_INFO,
					    "Logoff Req\n");
					if(!this->shareFrame(dvb_frame))
					{
						return false;
					}
				}
				break;

				case MSG_TYPE_TTP:
				case MSG_TYPE_SESSION_LOGON_RESP:
				{
					// nothing to do in this case
					LOG(this->log_receive, LEVEL_DEBUG,
					    "ignore TTP or logon response "
					    "(type = %d)\n", dvb_frame->getMessageType());
					delete dvb_frame;
				}
				break;

				case MSG_TYPE_SOF:
				{
					// use SoF for SAloha scheduling
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
				{
					if(!spot->handleSlottedAlohaFrame(dvb_frame))
					{
						return false;
					}
				}
				break;

				case MSG_TYPE_SALOHA_CTRL:
				{
					delete dvb_frame;
				}
				break;

				default:
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "unknown type (%d) of DVB frame\n",
					    dvb_frame->getMessageType());
					delete dvb_frame;
					return false;
				}
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



/***** DvbSpotList ****/

bool DvbSpotList::initSpotList(void)
{
	this->spots[this->default_spot] = NULL;
	return true;
}


DvbChannel *DvbSpotList::getSpot(spot_id_t spot_id) const
{
	map<spot_id_t, DvbChannel *>::const_iterator spot_it;

	spot_it = this->spots.find(spot_id);
	if(spot_it == this->spots.end())
	{
		LOG(this->log_spot, LEVEL_ERROR,
		    "spot %d does not exist\n",
		    spot_id);
		return NULL;
	}
	return (*spot_it).second;
}


void DvbSpotList::setStFmt(const map<spot_id_t, StFmtSimuList *> &output_sts_list,
                           const map<spot_id_t, StFmtSimuList *> &input_sts_list)
{
	this->output_sts_list = output_sts_list;
	this->input_sts_list = input_sts_list;
}

StFmtSimuList *DvbSpotList::getOutputStFmt(spot_id_t spot_id)
{
	return this->getStFmt(spot_id, this->output_sts_list);
}


StFmtSimuList *DvbSpotList::getInputStFmt(spot_id_t spot_id)
{
	return this->getStFmt(spot_id, this->input_sts_list);
}

StFmtSimuList *DvbSpotList::getStFmt(spot_id_t spot_id,
                                     const map<spot_id_t, StFmtSimuList *> &sts)
{
	map<spot_id_t, StFmtSimuList *>::const_iterator it;
	it = sts.find(spot_id);
	if(it == sts.end())
	{
		LOG(this->log_spot, LEVEL_ERROR,
		    "cannot find StFmtSimuList for spot %u\n",
		    spot_id);
		return NULL;
	}
	return (*it).second;
}

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
#include "DvbRcsFrame.h"
#include "Sof.h"

#include <errno.h>
#include <opensand_rt/TcpListenEvent.h>
#include <opensand_rt/MessageEvent.h>



/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/

BlockDvbNcc::BlockDvbNcc(const std::string &name, struct dvb_specific specific):
	BlockDvb{name},
	mac_id{specific.mac_id},
	output_sts{nullptr},
	input_sts{nullptr}
{
}


BlockDvbNcc::~BlockDvbNcc()
{
	delete this->input_sts;
	this->input_sts = nullptr;

	delete this->output_sts;
	this->output_sts = nullptr;
}

void BlockDvbNcc::generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane)
{
	SpotDownward::generateConfiguration(disable_ctrl_plane);
	SpotUpward::generateConfiguration(disable_ctrl_plane);
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
	if (!this->input_sts)
	{
		this->input_sts = new StFmtSimuList("in");
	}
	if(!this->input_sts)
	{
		return false;
	}

	if(!this->output_sts)
	{
		this->output_sts = new StFmtSimuList("out");
	}
	if(!this->output_sts)
	{
		return false;
	}

	// input and output sts are shared between up and down
	// and protected by a mutex
	static_cast<Upward *>(this->upward)->setOutputSts(this->output_sts);
	static_cast<Upward *>(this->upward)->setInputSts(this->input_sts);
	static_cast<Downward *>(this->downward)->setOutputSts(this->output_sts);
	static_cast<Downward *>(this->downward)->setInputSts(this->input_sts);

	return true;
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/


// TODO lot of duplicated code for fifos between ST and GW
BlockDvbNcc::Downward::Downward(const std::string &name, struct dvb_specific specific):
    DvbDownward{name, specific},
    DvbFmt{},
    pep_interface{},
    svno_interface{},
    mac_id{specific.mac_id},
    spot_id{specific.spot_id},
    fwd_frame_counter{0},
    fwd_timer{-1},
    probe_frame_interval{nullptr}
{
}


BlockDvbNcc::Downward::~Downward()
{
}


bool BlockDvbNcc::Downward::onInit(void)
{
	// get the common parameters
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		return false;
	}

	if(!this->initCommon(EncapSchemeList::FORWARD_DOWN))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	LOG(this->log_init, LEVEL_DEBUG,
	    "Create downward spot with ID %u\n", spot_id);

	this->spot = new SpotDownward(this->spot_id,
	                              this->mac_id,
	                              this->fwd_down_frame_duration_ms,
	                              this->ret_up_frame_duration_ms,
	                              this->stats_period_ms,
	                              this->pkt_hdl,
	                              this->input_sts,
	                              this->output_sts);

	if (!spot || !spot->onInit()) {
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the downward spot\n");
		return false;
	}

	// initialize the timers
	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the timers part of the "
		    "initialisation\n");
		return false;
	}

	if (!this->disable_control_plane)
	{
		// retrieve the TCP communication port dedicated
		// for NCC/PEP and NCC/SVNO communications
		int pep_tcp_port, svno_tcp_port;
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
	}

	// generate probes prefix
	std::ostringstream ss{};
	ss << "spot_" << int{spot_id} << ".";
	if (OpenSandModelConf::Get()->getComponentType() == Component::satellite)
	{
		ss << "sat.";
	}
	ss << "gw.";
	auto prefix = ss.str();

	// Output probes and stats
	this->probe_frame_interval = Output::Get()->registerProbe<float>(prefix + "Perf.Frames_interval",
	                                                                 "ms", true,
	                                                                 SAMPLE_LAST);

	return true;
}


bool BlockDvbNcc::Downward::initTimers(void)
{
	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                      this->fwd_down_frame_duration_ms);

	if (this->disable_control_plane)
	{
		return true;
	}

	this->frame_timer = this->addTimerEvent("frame",
	                                        this->ret_up_frame_duration_ms);

	auto Conf = OpenSandModelConf::Get();

	// read the pep allocation delay
	if(!Conf->getPepAllocationDelay(this->pep_alloc_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'schedulers': missing parameter 'pep allocation delay'\n");
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "pep_alloc_delay set to %d ms\n", this->pep_alloc_delay);

	time_ms_t acm_period_ms;
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
	if(!spot)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "Error when getting spot %d\n", spot_id);
		return false;
	}
	spot->setPepCmdApplyTimer(this->addTimerEvent("pep_request",
	                                              pep_alloc_delay,
	                                              false, // no rearm
	                                              false // do not start
	                                              ));

	return true;
}


bool BlockDvbNcc::Downward::handleDvbFrame(DvbFrame *dvb_frame)
{
  if (this->disable_control_plane)
  {
    bool result = this->sendDvbFrame(dvb_frame, spot->getCtrlCarrierId());
    return result;
  }

	EmulatedMessageType msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case EmulatedMessageType::Sac: // when physical layer is enabled
			return spot->handleSac(dvb_frame);

		case EmulatedMessageType::SessionLogonReq:
			return this->handleLogonReq(dvb_frame, spot);

		case EmulatedMessageType::SessionLogoff:
			return spot->handleLogoffReq(dvb_frame);

		default:
			LOG(this->log_receive, LEVEL_ERROR,
					"SF#%u: unknown type of DVB frame (%u), ignore\n",
					this->super_frame_counter, msg_type);
			delete dvb_frame;
			return false;
	}

	delete dvb_frame;
	return true;
}


bool BlockDvbNcc::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
    case EventType::Message:
		{
			auto msg_event = static_cast<const MessageEvent*>(event);
      InternalMessageType msg_type = to_enum<InternalMessageType>(msg_event->getMessageType());

			// first handle specific messages
			if(msg_type == InternalMessageType::sig)
			{
				auto dvb_frame = static_cast<DvbFrame *>(msg_event->getData());
				auto spot_id = dvb_frame->getSpot();
				if (spot_id != this->spot_id)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "Spot %d trying to send a DvbFrame destined to spot %d\n",
					    spot_id, spot_id);
					delete dvb_frame;
					return false;
				}

				if(!this->handleDvbFrame(dvb_frame)) {
					goto error;
				}
				break;
			}
			else if(msg_type == InternalMessageType::saloha)
			{
				auto ack_frames = static_cast<std::list<DvbFrame *> *>(msg_event->getData());
				auto spot_id = ack_frames->front()->getSpot();
				if (spot_id != this->spot_id)
				{
					LOG(this->log_receive, LEVEL_WARNING,
					    "Spot %d trying to send ACK frames destined to spot %d\n",
					    spot_id, spot_id);
					delete ack_frames;
					return false;
				}

				spot->handleSalohaAcks(ack_frames);
				delete ack_frames;
			}
			else
			{
				auto burst = static_cast<NetBurst *>(msg_event->getData());

				LOG(this->log_receive_channel, LEVEL_INFO,
						"SF#%u: encapsulation burst received "
						"(%d packet(s))\n", super_frame_counter,
						burst->length());

				// set each packet of the burst in MAC FIFO
				for(auto&& pkt : *burst)
				{
					if(!spot->handleEncapPacket(std::move(pkt)))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "cannot push burst into fifo\n");
						continue;
					}
				}
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
					time_val_t time = event->getAndSetCustomTime();
					this->probe_frame_interval->put(time/1000.f);
				}

				// we reached the end of a superframe
				// beginning of a new one, send SOF and run allocation
				// algorithms (DAMA)
				// increase the superframe number and reset
				// counter of frames per superframe
				this->super_frame_counter++;
			}

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
					break;
				}

				// Update Fmt here for TTP
				spot->updateFmt();

				if(!spot->handleFrameTimer(this->super_frame_counter))
				{
					return false;
				}

				// send TTP computed by DAMA
				this->sendTTP(spot);
			}
			else if(*event == this->fwd_timer)
			{
				this->fwd_frame_counter++;
				if(!spot->handleFwdFrameTimer(this->fwd_frame_counter))
				{
					return false;
				}

				// send the scheduled frames
				if(!this->sendBursts(&spot->getCompleteDvbFrames(),
				                     spot->getDataCarrierId()))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to build and send DVB/BB "
					    "frames\n");
					return false;
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
	if(!this->sendDvbFrame(reinterpret_cast<DvbFrame *>(sof), carrier_id))
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

	if(!this->sendDvbFrame(reinterpret_cast<DvbFrame *>(ttp), spot->getCtrlCarrierId()))
	{
		delete ttp;
		LOG(this->log_send, LEVEL_ERROR,
				"Failed to send TTP\n");
		return;
	}

	LOG(this->log_send, LEVEL_DEBUG,
			"SF#%u: TTP sent\n", this->super_frame_counter);
}


bool BlockDvbNcc::Downward::handleLogonReq(DvbFrame *dvb_frame, SpotDownward *spot)
{
	LogonRequest *logon_req = reinterpret_cast<LogonRequest *>(dvb_frame);
	uint16_t mac = logon_req->getMac();

	// Inform the Dama controller (for its own context)
	if(!spot->handleLogonReq(logon_req))
	{
		delete dvb_frame;
		return false;
	}

	// TODO only used here tal_id and logon_id are the same
	// may be we can simplify the constructor
	LogonResponse *logon_resp = new LogonResponse(mac, this->mac_id, mac);

	LOG(this->log_send, LEVEL_DEBUG,
			"SF#%u: logon response sent to lower layer\n",
			this->super_frame_counter);


	if(!this->sendDvbFrame(reinterpret_cast<DvbFrame *>(logon_resp),
                         spot->getCtrlCarrierId()))
	{
		LOG(this->log_send, LEVEL_ERROR,
				"Failed send logon response\n");
		return false;
	}

	return true;
}


// updateStats is pure virtual in BlockDvb not used in this case
void BlockDvbNcc::Downward::updateStats(void)
{
}


/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbNcc::Upward::Upward(const std::string &name, struct dvb_specific specific):
    DvbUpward{name, specific},
    DvbFmt{},
    mac_id{specific.mac_id},
    spot_id{specific.spot_id},
    log_saloha{nullptr},
    probe_gw_received_modcod{nullptr},
    probe_gw_rejected_modcod{nullptr}
{
}


BlockDvbNcc::Upward::~Upward()
{
}


bool BlockDvbNcc::Upward::onInit(void)
{
	LOG(this->log_init, LEVEL_DEBUG,
	    "Create upward spot with ID %u\n", spot_id);

	// TODO: check if disable_control_plane is needed here
	this->spot = new SpotUpward(this->spot_id,
	                            this->mac_id,
	                            this->input_sts,
	                            this->output_sts);

	if(!spot || !spot->onInit())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the upward spot\n");
		return false;
	}

	// create and send a "link is up" message to upper layer
	T_LINK_UP *link_is_up = new T_LINK_UP;
	if(!link_is_up)
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
	                         to_underlying(InternalMessageType::link_up)))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to send link up message to upper layer\n");
		delete link_is_up;
		return false;
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
	return true;
}


bool BlockDvbNcc::Upward::initOutput(void)
{
	auto output = Output::Get();

	// generate probes prefix
	std::ostringstream ss{};
	ss << "spot_" << int{spot_id} << ".";
	if (OpenSandModelConf::Get()->getComponentType() == Component::satellite)
	{
		ss << "sat.";
	}
	ss << "gw.";
	auto prefix = ss.str();

	this->probe_gw_received_modcod =
	    output->registerProbe<int>(prefix + "Down_Return_modcod.Received_modcod",
	                               "modcod index",
	                               true, SAMPLE_LAST);
	this->probe_gw_rejected_modcod =
	    output->registerProbe<int>(prefix + "Down_Return_modcod.Rejected_modcod",
	                               "modcod index",
	                               true, SAMPLE_LAST);

	return true;
}


bool BlockDvbNcc::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case EventType::Message:
		{
			DvbFrame *dvb_frame = static_cast<DvbFrame*>(static_cast<const MessageEvent*>(event)->getData());
			if (!this->onRcvDvbFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Failed handling DVB Frame\n");
				return false;
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


bool BlockDvbNcc::Upward::onRcvDvbFrame(DvbFrame* dvb_frame)
{
	spot_id_t dest_spot = dvb_frame->getSpot();
	if (dest_spot != this->spot_id)
	{
		LOG(this->log_receive, LEVEL_WARNING,
		    "Spot %d received a DvbFrame destined to spot %d\n",
		    spot_id, dest_spot);
		delete dvb_frame;
		return false;
	}

	fmt_id_t modcod_id = 0;
	EmulatedMessageType msg_type = dvb_frame->getMessageType();
	LOG(this->log_receive, LEVEL_INFO,
	    "DVB frame received with type %u\n", msg_type);

	if(msg_type == EmulatedMessageType::BbFrame)
	{
		BBFrame *bbframe = *dvb_frame;
		modcod_id = bbframe->getModcodId();
	}
	else if(msg_type == EmulatedMessageType::DvbBurst)
	{
		DvbRcsFrame *dvb_rcs_frame = *dvb_frame;
		modcod_id = dvb_rcs_frame->getModcodId();
	}
	switch(msg_type)
	{
		// burst
		case EmulatedMessageType::BbFrame:
		case EmulatedMessageType::DvbBurst:
		{
			if (!this->disable_control_plane)
			{
				// Update C/N0
				spot->handleFrameCni(dvb_frame);
			}

			if(!dvb_frame->isCorrupted())
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

			NetBurst *burst{nullptr};
			if(!spot->handleFrame(dvb_frame, &burst) || burst == nullptr)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle the frame\n");
				return false;
			}

			// send the message to the upper layer
			if (!this->enqueueMessage((void **)&burst, 0, to_underlying(InternalMessageType::decap_data)))
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

		case EmulatedMessageType::Sac:
		{
			// Update C/N0
			if (!this->disable_control_plane)
			{
				// Update C/N0
				spot->handleFrameCni(dvb_frame);
				if(!spot->handleSac(dvb_frame))
				{
					return false;
				}
			}

			if(!this->shareFrame(dvb_frame))
			{
				return false;
			}
		}
		break;

		case EmulatedMessageType::SessionLogonReq:
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "Logon Req\n");

			if(!this->disable_control_plane && !spot->onRcvLogonReq(dvb_frame))
			{
				return false;
			}

			if(!this->shareFrame(dvb_frame))
			{
				return false;
			}
		}
		break;

		case EmulatedMessageType::SessionLogoff:
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "Logoff Req\n");
			if(!this->shareFrame(dvb_frame))
			{
				return false;
			}
		}
		break;

		case EmulatedMessageType::Ttp:
		case EmulatedMessageType::SessionLogonResp:
		{
			if (this->disable_control_plane)
			{
				return this->enqueueMessage((void **)&dvb_frame, 0, to_underlying(InternalMessageType::sig));
			}
			// nothing to do in this case
			LOG(this->log_receive, LEVEL_DEBUG,
			    "ignore TTP or logon response "
			    "(type = %d)\n",
			    dvb_frame->getMessageType());
			delete dvb_frame;
			return true;
		}
		break;

		case EmulatedMessageType::Sof:
		{
			// use SoF for SAloha scheduling
			spot->updateStats();

			if (this->disable_control_plane)
			{
				return this->enqueueMessage((void **)&dvb_frame, 0, to_underlying(InternalMessageType::sig));
			}

			std::list<DvbFrame *> *ack_frames{nullptr};
			NetBurst *sa_burst{nullptr};

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
			if (sa_burst && !this->enqueueMessage((void **)&sa_burst, 0, to_underlying(InternalMessageType::unknown)))
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
			                       to_underlying(InternalMessageType::saloha)))
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
		case EmulatedMessageType::SalohaData:
		{
			if(!spot->handleSlottedAlohaFrame(dvb_frame))
			{
				return false;
			}

			std::list<DvbFrame *> *ack_frames{nullptr};
			NetBurst *sa_burst{nullptr};

			if(!spot->scheduleSaloha(nullptr, ack_frames, &sa_burst))
			{
				return false;
			}
		}
		break;

		case EmulatedMessageType::SalohaCtrl:
		{
			if (this->disable_control_plane)
			{
				if (!this->enqueueMessage((void **)&dvb_frame, 0, to_underlying(InternalMessageType::unknown)))
				{
					return false;
				}
				break;
			}
			else
			{
				delete dvb_frame;
			}
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

	return true;
}

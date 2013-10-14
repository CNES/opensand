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
 * @file BlockDvbTal.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal, compatible
 *        with Legacy and RrmQosDama agent
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_DVB_RCS_TAL
#include <opensand_conf/uti_debug.h>
#define DVB_DBG_PREFIX "[Tal]"

#include "BlockDvbTal.h"

#include "DamaAgentRcsLegacy.h"
#include "DamaAgentRcsRrmQos.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Ttp.h"
#include "Sof.h"

#include <opensand_rt/Rt.h>


#include <sstream>
#include <assert.h>

int BlockDvbTal::qos_server_sock = -1;

/**
 * constructor use mgl_bloc default constructor
 * @see mgl_bloc::mgl_bloc()
 */
BlockDvbTal::BlockDvbTal(const string &name, tal_id_t mac_id):
	BlockDvb(name),
	_state(state_initializing),
	mac_id(mac_id),
	dama_agent(NULL),
	m_carrierIdDvbCtrl(-1),
	m_carrierIdLogon(-1),
	m_carrierIdData(-1),
	complete_dvb_frames(),
	capacity_request(mac_id),
	ttp(),
	out_encap_packet_length(-1),
	out_encap_packet_type(MSG_TYPE_ERROR),
	in_encap_packet_length(-1),
	fixed_bandwidth(0),
	max_rbdc_kbps(0),
	max_vbdc_kb(0),
	logon_timer(-1),
	frame_timer(-1),
	first(true),
	default_fifo_id(0),
	nbr_pvc(0),
	qos_server_host(),
	m_obrPeriod(-1),
	m_obrSlotFrame(-1),
	event_login_sent(NULL),
	event_login_complete(NULL),
	probe_st_queue_size(NULL),
	probe_st_queue_size_kb(NULL),
	probe_st_l2_to_sat_before_sched(NULL),
	probe_st_l2_to_sat_after_sched(NULL),
	probe_st_l2_to_sat_total(NULL),
	probe_st_phy_to_sat(NULL),
	probe_st_l2_from_sat(NULL),
	probe_st_phy_from_sat(NULL),
	probe_st_real_modcod(NULL),
	probe_st_used_modcod(NULL),
	probe_sof_interval(NULL)
{
	this->l2_to_sat_cells_before_sched = NULL;
	this->l2_to_sat_cells_after_sched = NULL;
}


/**
 * Destructor
 */
BlockDvbTal::~BlockDvbTal()
{
	if(this->dama_agent != NULL)
	{
		delete this->dama_agent;
	}

	deletePackets();
	// delete fifos
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();

	if(this->l2_to_sat_cells_before_sched != NULL)
	{
		delete[] this->l2_to_sat_cells_before_sched;
	}
	if(this->l2_to_sat_cells_after_sched != NULL)
	{
		delete[] this->l2_to_sat_cells_after_sched;
	}

	// close QoS Server socket if it was opened
	if(BlockDvbTal::qos_server_sock != -1)
	{
		close(BlockDvbTal::qos_server_sock);
	}

	// release the reception DVB standard
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}

	// release the output arrays (no need to delete the probes)
	delete[] this->probe_st_queue_size;
	delete[] this->probe_st_queue_size_kb;
	delete[] this->probe_st_l2_to_sat_before_sched;
	delete[] this->probe_st_l2_to_sat_after_sched;

	this->complete_dvb_frames.clear();
}


bool BlockDvbTal::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// messages from upper layer: burst of encapsulation packets
			NetBurst *burst;
			NetBurst::iterator pkt_it;
			unsigned int fifo_priority;
			std::string message;
			std::ostringstream oss;
			int ret;

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			UTI_DEBUG("SF#%u: encapsulation burst received (%d packets)\n",
			          this->super_frame_counter, burst->length());

			// set each packet of the burst in MAC FIFO

			for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
			{
				UTI_DEBUG_L3("SF#%u: encapsulation packet has QoS value %d\n",
				             this->super_frame_counter, (*pkt_it)->getQos());

				fifo_priority = (*pkt_it)->getQos();
				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				// else use the default id

				if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
				{
					fifo_priority = this->default_fifo_id;
				}

				UTI_DEBUG("SF#%u: store one encapsulation packet (QoS = %d)\n",
				          this->super_frame_counter, fifo_priority);


				// store the encapsulation packet in the FIFO
				if(!this->onRcvEncapPacket(*pkt_it,
					                       this->dvb_fifos[fifo_priority],
					                       0))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					UTI_ERROR("SF#%u: frame %u: unable to "
					          "store received encapsulation "
					          "packet (see previous errors)\n",
					          this->super_frame_counter,
					          this->frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

				this->l2_to_sat_cells_before_sched[
					this->dvb_fifos[fifo_priority]->getPriority()]++;
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;

			// Cross layer information: if connected to QoS Server, build XML
			// message and send it
			// TODO move in a dedicated class
			if(BlockDvbTal::qos_server_sock == -1)
			{
				break;
			}

 			message = "";
			message.append("<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n");
			message.append("<XMLQoSMessage>\n");
			message.append(" <Sender>");
			message.append("CrossLayer");
			message.append("</Sender>\n");
			message.append(" <Type type=\"CrossLayer\" >\n");
			message.append(" <Infos ");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				int nbFreeFrames = (*it).second->getMaxSize() -
				                   (*it).second->getCurrentSize();
				int nbFreeBits = nbFreeFrames * this->out_encap_packet_length * 8; // bits
				float macRate = nbFreeBits / this->frame_duration_ms ; // bits/ms or kbits/s
				oss << "File=\"" << (int) macRate << "\" ";
				message.append(oss.str());
				oss.str("");
			}
			message.append("/>");
			message.append(" </Type>\n");
			message.append("</XMLQoSMessage>\n");

			ret = write(BlockDvbTal::qos_server_sock, message.c_str(), message.length());
			if(ret == -1)
			{
				UTI_NOTICE("failed to send message to QoS Server: %s (%d)\n",
				           strerror(errno), errno);
			}
		}
		break;

		case evt_timer:
			if(*event == this->logon_timer)
			{
				if(this->_state == state_wait_logon_resp)
				{
					// send another logon_req and raise timer
					// only if we are in the good state
					UTI_INFO("still no answer from NCC to the "
					         "logon request we sent for MAC ID %d, "
					         "send a new logon request\n",
					         this->mac_id);
					this->sendLogonReq();
				}
			}
			else if(*event == this->qos_server_timer)
			{
				// try to re-connect to QoS Server if not already connected
				if(BlockDvbTal::qos_server_sock == -1)
				{
					if(!this->connectToQoSServer())
					{
						UTI_DEBUG("failed to connect with QoS Server, cannot "
						          "send cross layer information");
					}
				}
			}
			else
			{
				UTI_ERROR("SF#%u: unknown timer event received %s",
				          this->super_frame_counter, event->getName().c_str());
				return false;
			}
			break;

		default:
			UTI_ERROR("SF#%u: unknown event received %s",
			          this->super_frame_counter, event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockDvbTal::onUpwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			unsigned char *dvb_frame;
			long len;
			T_DVB_META *dvb_meta;

			dvb_meta = (T_DVB_META *)((MessageEvent *)event)->getData();
			dvb_frame = (unsigned char *) dvb_meta->hdr;
			len = ((MessageEvent *)event)->getLength();

			if(this->probe_sof_interval->isEnabled() &&
			   dvb_meta->hdr->msg_type == MSG_TYPE_SOF)
			{
				struct timeval time = event->getTimeFromCustom();
				float val = time.tv_sec * 1000000L + time.tv_usec;
				event->setCustomTime();
				this->probe_sof_interval->put(val/1000);
			}

			// message from lower layer: DL dvb frame
			UTI_DEBUG_L3("SF#%u DVB frame received (len %ld)\n",
				         this->super_frame_counter, len);

			if(!this->onRcvDvbFrame(dvb_frame, len))
			{
				UTI_DEBUG_L3("SF#%u: failed to handle received DVB frame\n",
				             this->super_frame_counter);
				// a problem occured, trace is made in onRcvDVBFrame()
				// carry on simulation
				delete dvb_meta;
				return false;
			}
			delete dvb_meta;
		}
		break;

		case evt_timer:
			if(*event == this->frame_timer)
			{
				// beginning of a new frame
				if(this->_state == state_running)
				{
					UTI_DEBUG("SF#%u: send encap bursts on timer basis\n",
					          this->super_frame_counter);

					if(this->processOnFrameTick() < 0)
					{
						// exit because the bloc is unable to continue
						UTI_ERROR("SF#%u: treatments failed at frame %u",
						          this->super_frame_counter, this->frame_counter);
						// Fatal error
						this->upward->reportError(true, "superframe treatment failed");
						return false;
					}
				}
			}
			break;

		default:
			UTI_ERROR("SF#%u: unknown event received %s",
			          this->super_frame_counter, event->getName().c_str());
			return false;
	}

	return true;
}


// TODO remove receptionStd as functions are merged but contains part
//      dedicated to each host ?
bool BlockDvbTal::initMode()
{
	this->receptionStd = new DvbS2Std(this->down_forward_pkt_hdl);
	if(this->receptionStd == NULL)
	{
		UTI_ERROR("failed to create the reception standard\n");
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvbTal::initParameters()
{
	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	if(!globalConfig.getValue(DVB_TAL_SECTION, DVB_RT_BANDWIDTH,
	                          this->fixed_bandwidth))
	{
		UTI_ERROR("Missing %s", DVB_RT_BANDWIDTH);
		goto error;
	}
	UTI_INFO("fixed_bandwidth = %d kbits/s\n", this->fixed_bandwidth);

	return true;

error:
	return false;
}


bool BlockDvbTal::initCarrierId()
{
	int val;

#define FMT_KEY_MISSING "SF#%u %s missing from section %s\n", this->super_frame_counter

	 // Get the carrier Id m_carrierIdDvbCtrl
	if(!globalConfig.getValue(DVB_TAL_SECTION, DVB_CAR_ID_CTRL, val))
	{
		UTI_ERROR(FMT_KEY_MISSING, DVB_CAR_ID_CTRL, DVB_TAL_SECTION);
		goto error;
	}
	this->m_carrierIdDvbCtrl = val;

	// Get the carrier Id m_carrierIdLogon
	if(!globalConfig.getValue(DVB_TAL_SECTION, DVB_CAR_ID_LOGON, val))
	{
		UTI_ERROR(FMT_KEY_MISSING, DVB_CAR_ID_LOGON, DVB_TAL_SECTION);
		goto error;
	}
	this->m_carrierIdLogon = val;

	// Get the carrier Id m_carrierIdData
	if(!globalConfig.getValue(DVB_TAL_SECTION, DVB_CAR_ID_DATA, val))
	{
		UTI_ERROR(FMT_KEY_MISSING, DVB_CAR_ID_DATA, DVB_TAL_SECTION);
		goto error;
	}
	this->m_carrierIdData = val;

	UTI_INFO("SF#%u: carrier IDs for Ctrl = %ld, Logon = %ld, Data = %ld\n",
	         this->super_frame_counter, this->m_carrierIdDvbCtrl,
	         this->m_carrierIdLogon, this->m_carrierIdData);

	return true;
error:
	return false;
}

bool BlockDvbTal::initMacFifo(std::vector<std::string>& fifo_types)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	if(!globalConfig.getListItems(DVB_TAL_SECTION, FIFO_LIST, fifo_list))
	{
		UTI_ERROR("section '%s, %s': missing fifo list", DVB_TAL_SECTION,
		          FIFO_LIST);
		goto err_fifo_release;
	}

	for(iter = fifo_list.begin(); iter != fifo_list.end(); iter++)
	{
		unsigned int fifo_priority;
		unsigned int pvc;
		vol_pkt_t fifo_size = 0;
		string fifo_mac_prio;
		string fifo_cr_type;
		DvbFifo *fifo;

		// get fifo_id --> fifo_priority
		if(!globalConfig.getAttributeValue(iter, FIFO_PRIO, fifo_priority))
		{
			UTI_ERROR("%s: cannot get %s from section '%s, %s'\n",
			          FUNCNAME, FIFO_PRIO, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_mac_prio
		if(!globalConfig.getAttributeValue(iter, FIFO_TYPE, fifo_mac_prio))
		{
			UTI_ERROR("%s: cannot get %s from section '%s, %s'\n",
			          FUNCNAME, FIFO_TYPE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_size
		if(!globalConfig.getAttributeValue(iter, FIFO_SIZE, fifo_size))
		{
			UTI_ERROR("%s: cannot get %s from section '%s, %s'\n",
			          FUNCNAME, FIFO_SIZE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get pvc
		if(!globalConfig.getAttributeValue(iter, FIFO_PVC, pvc))
		{
			UTI_ERROR("%s: cannot get %s from section '%s, %s'\n",
			          FUNCNAME, FIFO_PVC, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get the fifo CR type
		if(!globalConfig.getAttributeValue(iter, FIFO_CR_TYPE, fifo_cr_type))
		{
			UTI_ERROR("%s: cannot get %s from section '%s, %s'\n",
			          FUNCNAME, FIFO_CR_TYPE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}

		fifo = new DvbFifo(fifo_priority, fifo_mac_prio,
		                   fifo_cr_type, pvc, fifo_size);

		UTI_INFO("%s: Fifo priority = %u, FIFO name %s, size %u, pvc %u, "
		         "CR type %d\n", FUNCNAME,
		         fifo->getPriority(),
		         fifo->getName().c_str(),
		         fifo->getMaxSize(),
		         fifo->getPvc(),
		         fifo->getCrType());

		// update the number of PVC = the maximum PVC
		this->nbr_pvc = std::max(this->nbr_pvc, pvc);

		// the default FIFO is the last one = the one with the smallest priority
		// actually, the IP plugin should add packets in the default FIFO if
		// the DSCP field is not recognize, default_fifo_id should not be used
		// this is only used if traffic categories configuration and fifo configuration
		// are not coherent.
		this->default_fifo_id = std::max(this->default_fifo_id, fifo->getPriority());

		this->dvb_fifos.insert(pair<unsigned int, DvbFifo *>(fifo->getPriority(), fifo));
		fifo_types.push_back(fifo_mac_prio);
	} // end for(queues are now instanciated and initialized)

	// init stats context per QoS
	this->l2_to_sat_cells_before_sched = new int[this->dvb_fifos.size()];
	if(this->l2_to_sat_cells_before_sched == NULL)
	{
		goto err_before_release;
	}

	this->l2_to_sat_cells_after_sched = new int[this->dvb_fifos.size()];
	if(this->l2_to_sat_cells_after_sched == NULL)
	{
		goto err_after_release;
	}

	this->resetStatsCxt();

	return true;

err_before_release:
	delete[] this->l2_to_sat_cells_after_sched;
err_after_release:
	delete[] this->l2_to_sat_cells_before_sched;
err_fifo_release:
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();
	return false;
}


bool BlockDvbTal::initObr()
{
	// get the OBR period - in number of frames
	if(!globalConfig.getValue(DVB_TAL_SECTION, DVB_OBR_PERIOD_DATA, m_obrPeriod))
	{
		UTI_ERROR("Missing %s", DVB_OBR_PERIOD_DATA);
		goto error;
	}

	// deduce the Obr slot position within the multi-frame, from the mac
	// address and the OBR period
	// ObrSlotFrame= MacAddress 'modulo' Obr Period
	// NB : ObrSlotFrame is within [0, Obr Period -1]
	m_obrSlotFrame = this->mac_id % m_obrPeriod;
	UTI_INFO("SF#%u: MAC adress = %d, OBR period = %d, "
	         "OBR slot frame = %d\n", this->super_frame_counter,
	         this->mac_id, m_obrPeriod, m_obrSlotFrame);

	return true;
error:
	return false;
}


bool BlockDvbTal::initDama()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	time_sf_t rbdc_timeout_sf = 0;
	time_sf_t msl_sf = 0;
	string dama_algo;
	bool cr_output_only;

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	if(!globalConfig.getValue(DA_TAL_SECTION, DA_MAX_RBDC_DATA,
	                                          this->max_rbdc_kbps))
	{
		UTI_ERROR("%s Missing %s\n",
		          FUNCNAME, DA_MAX_RBDC_DATA);
		goto error;
	}

	if(!globalConfig.getValue(DA_TAL_SECTION,
	                          DA_RBDC_TIMEOUT_DATA, rbdc_timeout_sf))
	{
		UTI_ERROR("%s Missing %s\n",
		          FUNCNAME, DA_RBDC_TIMEOUT_DATA);
		goto error;
	}

	// Max VBDC
	if(!globalConfig.getValue(DA_TAL_SECTION, DA_MAX_VBDC_DATA,
	                          this->max_vbdc_kb))
	{
		UTI_ERROR("%s Missing %s\n",
		          FUNCNAME, DA_MAX_VBDC_DATA);
		goto error;
	}

	// MSL duration -- in frames number
	if(!globalConfig.getValue(DA_TAL_SECTION, DA_MSL_DURATION, msl_sf))
	{
		UTI_ERROR("%s Missing %s\n",
		          FUNCNAME, DA_MSL_DURATION);
		goto error;
	}

	// CR computation rule
	if(!globalConfig.getValue(DA_TAL_SECTION, DA_CR_RULE, cr_output_only))
	{
		UTI_ERROR("%s Missing %s\n",
		          FUNCNAME, DA_CR_RULE);
		goto error;
	}

	UTI_INFO("%s ULCarrierBw %d kbits/s, "
	         "RBDC max %d kbits/s, RBDC Timeout %d frame, "
	         "VBDC max %d kbits, mslDuration %d frames, "
	         "getIpOutputFifoSizeOnly %d\n",
	         FUNCNAME,
	         this->fixed_bandwidth, this->max_rbdc_kbps,
	         rbdc_timeout_sf, this->max_vbdc_kb, msl_sf,
	         cr_output_only);

	// dama algorithm
	if(!globalConfig.getValue(DVB_TAL_SECTION, DAMA_ALGO,
	                          dama_algo))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_TAL_SECTION, DAMA_ALGO);
		goto error;
	}

	if(dama_algo == "Legacy")
	{
		UTI_INFO("%s SF#%u: create Legacy DAMA agent\n", FUNCNAME,
		         this->super_frame_counter);

		this->dama_agent = new DamaAgentRcsLegacy();
	}
	else if(dama_algo == "RrmQos")
	{
		UTI_INFO("%s SF#%u: create RrmQos DAMA agent\n", FUNCNAME,
		         this->super_frame_counter);

		this->dama_agent = new DamaAgentRcsRrmQos();
	}
	else
	{
		UTI_ERROR("cannot create DAMA agent: algo named '%s' is not "
		          "managed by current MAC layer\n", dama_algo.c_str());
		goto error;
	}

	if(this->dama_agent == NULL)
	{
		UTI_ERROR("failed to create DAMA agent\n");
		goto error;
	}

	// Initialize the DamaAgent parent class
	if(!this->dama_agent->initParent(this->frame_duration_ms,
	                                 this->fixed_bandwidth,
	                                 this->max_rbdc_kbps,
	                                 rbdc_timeout_sf,
	                                 this->max_vbdc_kb,
	                                 msl_sf,
	                                 this->m_obrPeriod,
	                                 cr_output_only,
	                                 this->up_return_pkt_hdl,
	                                 this->dvb_fifos))
	{

		UTI_ERROR("%s SF#%u Dama Agent Initialization failed.\n", FUNCNAME,
		          this->super_frame_counter);
		goto err_agent_release;
	}

	// Initialize the DamaAgentRcsXXX class
	if(!this->dama_agent->init())
	{
		UTI_ERROR("%s Dama Agent initialization failed.\n", FUNCNAME);
		goto err_agent_release;
	}

	return true;

err_agent_release:
	delete this->dama_agent;
error:
	return false;
}


bool BlockDvbTal::initQoSServer()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[initQoSServer]";

	// QoS Server: read hostname and port from configuration
	if(!globalConfig.getValue(SECTION_QOS_AGENT, QOS_SERVER_HOST,
	                          this->qos_server_host))
	{
		UTI_ERROR("%s section %s, %s missing",
		          FUNCNAME, SECTION_QOS_AGENT, QOS_SERVER_HOST);
		goto error;
	}

	if(!globalConfig.getValue(SECTION_QOS_AGENT, QOS_SERVER_PORT,
	                          this->qos_server_port))
	{
		UTI_ERROR("%s section %s, %s missing\n",
		          FUNCNAME, SECTION_QOS_AGENT, QOS_SERVER_PORT);
		goto error;
	}
	else if(this->qos_server_port <= 1024 || this->qos_server_port > 0xffff)
	{
		UTI_ERROR("%s QoS Server port (%d) not valid\n",
		          FUNCNAME, this->qos_server_port);
		goto error;
	}

	// QoS Server: catch the SIGFIFO signal that is sent to the process
	// when QoS Server kills the TCP connection
	if(signal(SIGPIPE, BlockDvbTal::closeQosSocket) == SIG_ERR)
	{
		UTI_ERROR("cannot catch signal SIGPIPE\n");
		goto error;
	}

	// QoS Server: try to connect to remote host
	this->connectToQoSServer();

	// QoS Server: check connection status in 5 seconds
	this->qos_server_timer = this->downward->addTimerEvent("qos_server", 5000);

	return true;
error:
	return false;
}

bool BlockDvbTal::initOutput(const std::vector<std::string>& fifo_types)
{
	this->event_login_sent = Output::registerEvent("bloc_dvb:login_sent",
	                                               LEVEL_INFO);
	this->event_login_complete = Output::registerEvent("bloc_dvb:login_complete",
	                                                   LEVEL_INFO);
	this->probe_st_real_modcod = Output::registerProbe<int>("Real_modcod",
	                                                        "modcod index",
	                                                        true, SAMPLE_LAST);
	this->probe_st_used_modcod = Output::registerProbe<int>("Received_modcod",
	                                                        "modcod index",
	                                                        true, SAMPLE_LAST);
	this->probe_sof_interval = Output::registerProbe<float>("perf.SOF_interval",
	                                                        "ms", true,
	                                                        SAMPLE_LAST);

	this->probe_st_queue_size = new Probe<int>*[this->dvb_fifos.size()];
	this->probe_st_queue_size_kb = new Probe<int>*[this->dvb_fifos.size()];
	this->probe_st_l2_to_sat_before_sched = new Probe<int>*[this->dvb_fifos.size()];
	this->probe_st_l2_to_sat_after_sched = new Probe<int>*[this->dvb_fifos.size()];

	if(this->probe_st_queue_size == NULL ||
	   this->probe_st_queue_size_kb == NULL ||
	   this->probe_st_l2_to_sat_before_sched == NULL ||
	   this->probe_st_l2_to_sat_after_sched == NULL)
	{
		UTI_ERROR("Failed to allocate memory for probe arrays");
		return false;
	}

	for(unsigned int i = 0 ; i < this->dvb_fifos.size() ; i++)
	{
		const char* fifo_type = fifo_types[i].c_str();

		this->probe_st_queue_size[i] =
			Output::registerProbe<int>("Packets", true, SAMPLE_LAST,
			                           "Queue size.packets.%s", fifo_type);
		this->probe_st_queue_size_kb[i] =
			Output::registerProbe<int>("kbits", true, SAMPLE_LAST,
			                           "Queue size.%s", fifo_type);

		this->probe_st_l2_to_sat_before_sched[i] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
			                           "Throughputs.L2_to_SAT_before_sched.%s",
			                           fifo_type);
		this->probe_st_l2_to_sat_after_sched[i] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
			                           "Throughputs.L2_to_SAT_after_sched.%s",
			                           fifo_type);
	}
	this->probe_st_l2_to_sat_total =
		Output::registerProbe<int>("Throughputs.L2_to_SAT_after_sched.total",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->probe_st_l2_from_sat =
		Output::registerProbe<int>("Throughputs.L2_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->probe_st_phy_from_sat =
		Output::registerProbe<int>("Throughputs.PHY_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->probe_st_phy_to_sat =
		Output::registerProbe<int>("Throughputs.PHY_to_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);

	return true;
}


bool BlockDvbTal::initTimers()
{
	this->logon_timer = this->downward->addTimerEvent("logon", 5000,
	                                                  false, // do not rearm
	                                                  false // do not start
	                                                  );
	this->frame_timer = this->upward->addTimerEvent("frame",
	                                                 DVB_TIMER_ADJUST(
	                                                   this->frame_duration_ms),
	                                                 false,
	                                                 false);
	return true;
}


bool BlockDvbTal::onInit()
{
	std::vector<std::string> fifo_types;

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

	if(!this->initParameters())
	{
		UTI_ERROR("failed to complete the 'parameters' part of the "
		          "initialisation");
		goto error;
	}

	if(!this->initCarrierId())
	{
		UTI_ERROR("failed to complete the carrier IDs part of the "
		          "initialisation");
		goto error;
	}

	if(!this->initMacFifo(fifo_types))
	{
		UTI_ERROR("failed to complete the MAC FIFO part of the "
		          "initialisation");
		goto error;
	}

	if(!this->initObr())
	{
		UTI_ERROR("failed to complete the OBR part of the "
		          "initialisation");
		goto error;
	}

	if(!this->initDama())
	{
		UTI_ERROR("failed to complete the DAMA part of the "
		          "initialisation");
		goto error;
	}

	if(!this->initQoSServer())
	{
		UTI_ERROR("failed to complete the QoS Server part of the "
		          "initialisation");
		goto error;
	}

	// Init the output here since we now know the FIFOs
	if(!this->initOutput(fifo_types))
	{
		UTI_ERROR("failed to complete the initialisation of output");
		goto error;
	}

	if(!this->initTimers())
	{
		UTI_ERROR("failed to complete the initialization of timers");
		goto error;
	}

	return true;

error:
	return false;
}


// TODO: move to a dedicated class
/**
 * Signal callback called upon SIGFIFO reception.
 *
 * This function is declared as static.
 *
 * @param sig  The signal that called the function
 */
void BlockDvbTal::closeQosSocket(int sig)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[BlockDvbTal::closeQosSocket]";
	UTI_NOTICE("%s TCP connection broken, close socket\n", FUNCNAME);
	close(BlockDvbTal::qos_server_sock);
	BlockDvbTal::qos_server_sock = -1;
}


// TODO: move to a dedicated class
/**
 * Try to connect to the QoS Server
 *
 * The qos_server_host and qos_server_port class variables must be correctly
 * initialized. The qos_server_sock variable should be -1 when calling this
 * function.
 *
 * @return   true if connection is successful, false otherwise
 */
bool BlockDvbTal::connectToQoSServer()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[BlockDvbTal::connectToQoSServer]";
	struct addrinfo hints;
	struct protoent *tcp_proto;
	struct servent *serv;
	struct addrinfo *addresses;
	struct addrinfo *address;
	char straddr[INET6_ADDRSTRLEN];
	int ret;

	if(BlockDvbTal::qos_server_sock != -1)
	{
		UTI_NOTICE("%s already connected to QoS Server, do not call this "
		           "function when already connected\n", FUNCNAME);
		goto skip;
	}

	// set criterias to resolve hostname
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// get TCP protocol number
	tcp_proto = getprotobyname("TCP");
	if(tcp_proto == NULL)
	{
		UTI_NOTICE("%s TCP is not available on the system\n", FUNCNAME);
		goto error;
	}
	hints.ai_protocol = tcp_proto->p_proto;

	// get service name
	serv = getservbyport(htons(this->qos_server_port), "tcp");
	if(serv == NULL)
	{
		UTI_DEBUG("%s service on TCP/%d is not available\n", FUNCNAME,
		          this->qos_server_port);
		goto error;
	}

	// resolve hostname
	ret = getaddrinfo(this->qos_server_host.c_str(), serv->s_name, &hints, &addresses);
	if(ret != 0)
	{
		UTI_NOTICE("%s cannot resolve hostname '%s': %s (%d)\n",
		           FUNCNAME, this->qos_server_host.c_str(), gai_strerror(ret), ret);
		goto error;
	}

	// try to create socket with available addresses
	address = addresses;
	while(address != NULL && BlockDvbTal::qos_server_sock == -1)
	{
		bool is_ipv4;
		void *sin_addr;
		const char *retptr;

		is_ipv4 = (address->ai_family == AF_INET);
		if(is_ipv4)
			sin_addr = &((struct sockaddr_in *) address->ai_addr)->sin_addr;
		else // ipv6
			sin_addr = &((struct sockaddr_in6 *) address->ai_addr)->sin6_addr;

		retptr = inet_ntop(address->ai_family,
		                   sin_addr,
		                   straddr,
		                   sizeof(straddr));
		if(retptr != NULL)
		{
			UTI_DEBUG("%s try IPv%d address %s\n", FUNCNAME, is_ipv4 ? 4 : 6, straddr);
		}
		else
		{
			UTI_DEBUG("%s try an IPv%d address\n", FUNCNAME, is_ipv4 ? 4 : 6);
		}

		BlockDvbTal::qos_server_sock = socket(address->ai_family,
		                                      address->ai_socktype,
		                                      address->ai_protocol);
		if(BlockDvbTal::qos_server_sock == -1)
		{
			UTI_DEBUG("%s cannot create socket (%s) with address %s\n",
			           FUNCNAME, strerror(errno), straddr);
			address = address->ai_next;
			continue;
		}

		UTI_DEBUG("%s socket created for address %s\n", FUNCNAME, straddr);
	}

	if(BlockDvbTal::qos_server_sock == -1)
	{
		UTI_NOTICE("%s no valid address found for hostname %s\n", FUNCNAME,
		           this->qos_server_host.c_str());
		goto free_dns;
	}

	UTI_DEBUG("%s try to connect with QoS Server at %s[%s]:%d\n", FUNCNAME,
	this->qos_server_host.c_str(), straddr, this->qos_server_port);

	// try to connect with the socket
	ret = connect(BlockDvbTal::qos_server_sock,
	              address->ai_addr, address->ai_addrlen);
	if(ret == -1)
	{
		UTI_DEBUG("%s connect() failed: %s (%d)\n", FUNCNAME, strerror(errno), errno);
		UTI_DEBUG("%s will retry to connect later\n", FUNCNAME);
		goto close_socket;
	}

	UTI_INFO("%s connected with QoS Server at %s[%s]:%d\n", FUNCNAME,
	         this->qos_server_host.c_str(), straddr, this->qos_server_port);

	// clean allocated addresses
	freeaddrinfo(addresses);

skip:
	return true;

close_socket:
	close(BlockDvbTal::qos_server_sock);
	BlockDvbTal::qos_server_sock = -1;
free_dns:
	freeaddrinfo(addresses);
error:
	return false;
}


bool BlockDvbTal::sendLogonReq()
{
	LogonRequest logon_req(this->mac_id, this->fixed_bandwidth,
	                       this->max_rbdc_kbps, this->max_vbdc_kb);

	// send the message to the lower layer
	if(!this->sendDvbFrame(logon_req.getFrame(),
		                   m_carrierIdLogon,
		                   logon_req.getLength()))
	{
		UTI_ERROR("Failed to send Logon Request\n");
		goto error;
	}
	UTI_DEBUG_L3("SF#%u Logon Req. sent to lower layer\n",
	             this->super_frame_counter);

	if(!this->downward->startTimer(this->logon_timer))
	{
		UTI_ERROR("cannot start logon timer");
		goto error;
	}

	// send the corresponding event
	Output::sendEvent(event_login_sent, "Login sent to %d",
	                  this->mac_id);

	return true;

error:
	return false;
}


bool BlockDvbTal::onRcvDvbFrame(unsigned char *ip_buf, long i_len)
{
	T_DVB_HDR *hdr;

	// Get msg header
	hdr = (T_DVB_HDR *) ip_buf;

	switch(hdr->msg_type)
	{
		case MSG_TYPE_BBFRAME:
		{
			// Update stats
			this->l2_from_sat_bytes += hdr->msg_length;
			this->l2_from_sat_bytes -= sizeof(T_DVB_HDR);
			this->phy_from_sat_bytes += hdr->msg_length;

			NetBurst *burst;

			if(this->receptionStd->onRcvFrame(ip_buf, i_len, hdr->msg_type,
			                                  this->m_talId, &burst) < 0)
			{
				UTI_ERROR("failed to handle the reception of "
				          "BB frame (length = %ld)\n", i_len);
				goto error;
			}
			if(burst && this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}

			break;
		}

		// Start of frame (SOF):
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_SOF:
			const char *state_descr;

			if(this->_state == state_running)
				state_descr = "state_running";
			else if(this->_state == state_initializing)
				state_descr = "state_initializing";
			else
				state_descr = "other";

			UTI_DEBUG("SF#%u: received SOF in state %s\n",
			          this->super_frame_counter, state_descr);

			if(this->_state == state_running)
			{
				if(!this->onStartOfFrame(ip_buf, i_len))
				{
					UTI_ERROR("Cannot handle SoF");
					goto error;
				}
			}
			else
			{
				free(ip_buf);
			}
			break;

		// TTP:
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_TTP:
			if(this->_state == state_running)
			{
				this->ttp.parse(ip_buf, i_len);
				if(!this->dama_agent->hereIsTTP(this->ttp))
				{
					free(ip_buf);
					goto error_on_TTP;
				}
			}
			free(ip_buf);
			break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->onRcvLogonResp(ip_buf, i_len))
			{
				goto error;
			}
			break;

		// messages sent by current or another ST for the NCC --> ignore
		case MSG_TYPE_CR:
		case MSG_TYPE_SESSION_LOGON_REQ:
			free(ip_buf);
			break;

		case MSG_TYPE_CORRUPTED:
			UTI_INFO("SF#%u: the message was corrupted by physical layer, "
			         "drop it", this->super_frame_counter);
			free(ip_buf);
			break;

		default:
			UTI_ERROR("SF#%u: unknown type of DVB frame (%u), ignore\n",
			          this->super_frame_counter, hdr->msg_type);
			free(ip_buf);
			goto error;
	}

	return true;

error_on_TTP:
	UTI_ERROR("TTP Treatments failed at SF#%u, frame %u",
	          this->super_frame_counter, this->frame_counter);
	return false;

error:
	UTI_ERROR("Treatments failed at SF#%u, frame %u",
	          this->super_frame_counter, this->frame_counter);
	return false;
}

/**
 * Send a capacity request for NRT data
 * @return true on success, false otherwise
 */
bool BlockDvbTal::sendCR()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[sendCR]";
	unsigned char *dvb_frame;
	size_t length;
	bool empty;

	// Set CR body
	// NB: cr_type parameter is not used here as CR is built for both
	// RBDC and VBDC
	if(!this->dama_agent->buildCR(cr_none,
	                              this->capacity_request,
	                              empty))
	{
		UTI_ERROR("%s SF#%u frame %u: DAMA cannot build CR\n", FUNCNAME,
		          this->super_frame_counter, this->frame_counter);
		goto error;
	}

	if(empty)
	{
		UTI_DEBUG_L3("SF#%u frame %u: Empty CR\n",
		             this->super_frame_counter, this->frame_counter);
		return true;
	}

	// Get a dvb frame
	dvb_frame = (unsigned char *)calloc(MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX,
	                                    sizeof(unsigned char));
	if(dvb_frame == 0)
	{
		UTI_ERROR("SF#%u frame %u: cannot get memory for CR\n",
		          this->super_frame_counter, this->frame_counter);
		goto error;
	}

	this->capacity_request.build(dvb_frame, length);

	// Send message
	if(!this->sendDvbFrame((T_DVB_HDR *) dvb_frame, m_carrierIdDvbCtrl, length))
	{
		UTI_ERROR("%s SF#%u frame %u: failed to allocate mgl msg\n",
		          FUNCNAME, this->super_frame_counter, this->frame_counter);
		free(dvb_frame);
		goto error;
	}

	UTI_DEBUG("%s SF#%u frame %u: CR sent\n", FUNCNAME,
	          this->super_frame_counter, this->frame_counter);

	return true;

error:
	return false;
}


/**
 * Upon reception of a SoF:
 * - update allocation with TBTP received last superframe (in DAMA agent)
 * - reset timers
 * @param ip_buf points to the dvb_rcs buffer containing SoF
 * @param i_len is the length of *ip_buf
 * @return true eon success, false otherwise
 */
bool BlockDvbTal::onStartOfFrame(unsigned char *ip_buf, long i_len)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	Sof sof(ip_buf, i_len);

	sfn = sof.getSuperFrameNumber();

	UTI_DEBUG_L3("SOF reception SFN #%u super frame nb %u frame "
	             "counter %u\n", sfn, this->super_frame_counter,
	             this->frame_counter);
	UTI_DEBUG("superframe number: %u", sfn);

	// if the NCC crashed, we must reinitiate a logon
	// TODO handle modulo on maximum
	if(sfn < this->super_frame_counter)
	{
		UTI_ERROR("SF#%u: it seems NCC rebooted => flush buffer & "
		          "resend a logon request\n",
		          this->super_frame_counter);

		this->deletePackets();
		if(!this->sendLogonReq())
		{
			goto error;
		}

		this->_state = state_wait_logon_resp;
		this->super_frame_counter = sfn;
		this->first = true;
		this->frame_counter = 0;
		goto error;
	}

	// as long as the frame is changing, send all probes and event
	Output::sendProbes();

	// update the frame numerotation
	this->super_frame_counter = sfn;

	// Inform dama agent
	if(!this->dama_agent->hereIsSOF(sfn))
	{
		goto error;
	}

	// There is a risk of unprecise timing so the following hack

	// ---- if we have consumed all frames of previous sf ----
	// ---- (or if it is the first frame)                 ----
	if(this->frame_counter == this->frames_per_superframe ||
	   this->first)
	{
		UTI_DEBUG("SF#%u frame %u: all frames from previous SF are "
		          "consumed or it is the first frame\n",
		          this->super_frame_counter, this->frame_counter);

		// reset frame counter: it will be init to 1 (1st frame number)
		// at the beginning of processOnFrameTick()
		this->frame_counter = 0;

		// we have consumed all of our frames, we start a new one immediately
		// this is the first frame of the new superframe
		if(this->processOnFrameTick() < 0)
		{
			// exit because the bloc is unable to continue
			UTI_ERROR("SF#%u frame %u: treatments failed\n",
			          this->super_frame_counter, this->frame_counter);
			goto error;
		}
	}
	else
	{
		// else : frame_counter < frames_per_superframe
		// if we have not consumed all our frames (it is the risk)
		// Then there is, by design, a timer active, we have to leave it
		// as we cannot remove it
		// hence we do only a reassignation of frame_counter (the frame active
		// count now as one frame in our superframe)
		this->frame_counter = 0;
	}

	free(ip_buf);
	return true;

error:
	free(ip_buf);
	return false;
}


/**
 * When receive a frame tick, send a constant DVB burst size for RT traffic,
 * and a DVB burst for NRT allocated by the DAMA agent
 * @return 0 on success, -1 if failed
 */
int BlockDvbTal::processOnFrameTick()
{
	int ret = 0;
	int globalFrameNumber;

	// update frame counter for current SF - 1st frame within SF is 1 -
	this->frame_counter++;
	UTI_DEBUG("SF#%u: frame %u: start processOnFrameTick\n",
	          this->super_frame_counter, this->frame_counter);

	// ------------ arm timer for next frame -----------
	// this is done at the beginning in order not to increase next frame
	// by current frame treatments delay
	if(this->frame_counter < this->frames_per_superframe)
	{
		if(!this->upward->startTimer(this->frame_timer))
		{
			UTI_ERROR("cannot start frame timer");
			goto error;
		}
	}

	// ---------- tell the DAMA agent that a new frame begins ----------
	// Inform dama agent, and update total Available Allocation
	// for current frame
	if(!this->dama_agent->processOnFrameTick())
	{
		UTI_ERROR("SF#%u: frame %u: failed to process frame tick\n",
	          this->super_frame_counter, this->frame_counter);
	    goto error;
	}

	// ---------- schedule and send data frames ---------
	// schedule packets extracted from DVB FIFOs according to
	// the algorithm defined in DAMA agent
	if(!this->dama_agent->returnSchedule(&this->complete_dvb_frames))
	{
		UTI_ERROR("SF#%u: frame %u: failed to schedule packets from DVB FIFOs\n",
		          this->super_frame_counter, this->frame_counter);
		goto error;
	}

	// send on the emulated DVB network the DVB frames that contain
	// the encapsulation packets scheduled by the DAMA agent algorithm
	if(!this->sendBursts(&this->complete_dvb_frames, this->m_carrierIdData))
	{
		UTI_ERROR("failed to send bursts in DVB frames\n");
		goto error;
	}

	// ---------- Capacity Request ----------
	// compute and send Capacity Request ... only if
	// the OBR period has been reached
	globalFrameNumber =
		(this->super_frame_counter - 1) * this->frames_per_superframe
		+ this->frame_counter;
	if((globalFrameNumber % m_obrPeriod) == m_obrSlotFrame)
	{
		if(!this->sendCR())
		{
			UTI_ERROR("failed to send Capacity Request\n");
			goto error;
		}
	}

	// ---------- Statistics ---------
	// trace statistics for current frame
	this->updateStatsOnFrame();

	return ret;

error:
	return -1;
}


bool BlockDvbTal::onRcvLogonResp(unsigned char *ip_buf, long l_len)
{
	T_LINK_UP *link_is_up;
	LogonResponse logon_resp(ip_buf, l_len);

	// Retrieve the Logon Response frame
	if(logon_resp.getMac() != this->mac_id)
	{
		UTI_DEBUG("SF#%u Loggon_resp for mac=%d, not %d\n",
		          this->super_frame_counter, logon_resp.getMac(), this->mac_id);
		goto ok;
	}

	// Remember the id
	this->m_groupId = logon_resp.getGroupId();
	this->m_talId = logon_resp.getLogonId();

	// Inform Dama agent
	this->dama_agent->hereIsLogonResp(logon_resp);

	// Send a link is up message to upper layer
	// link_is_up
	link_is_up = new T_LINK_UP;
	if(link_is_up == 0)
	{
		UTI_ERROR("SF#%u Memory allocation error on link_is_up\n",
		          this->super_frame_counter);
		goto error;
	}
	link_is_up->group_id = this->m_groupId;
	link_is_up->tal_id = this->m_talId;

	if(!this->sendUp((void **)(&link_is_up), sizeof(T_LINK_UP), msg_link_up))
	{
		UTI_ERROR("SF#%u: failed to send link up message to upper layer",
		          this->super_frame_counter);
		delete link_is_up;
		goto error;
	}
	UTI_DEBUG_L3("SF#%u Link is up msg sent to upper layer\n",
	             this->super_frame_counter);

	// Set the state to "running"
	this->_state = state_running;
	UTI_INFO("SF#%u: logon succeeded, running as group %ld and logon %ld\n",
	         this->super_frame_counter, this->m_groupId, this->m_talId);

	// send the corresponding event

	Output::sendEvent(event_login_complete, "Login complete with MAC %d",
	                  this->mac_id);

 ok:
	free(ip_buf);
	return true;
 error:
	free(ip_buf);
	return false;
}


/**
 * Update statistics :
 * These statistics will be updated when the DVB bloc receive a frame tick
 * because they depend on frame duration
 */
void BlockDvbTal::updateStatsOnFrame()
{

	this->dama_agent->updateStatistics();

	mac_fifo_stat_context_t fifo_stat;
	// MAC fifos stats
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->getStatsCxt(fifo_stat);

		// NB: mac queueing delay = fifo_stat.lastPkQueuingDelay
		// is writing at each UL cell emission by MAC layer and DA

		// NB: outgoingCells = cells directly sent from IP packets + cells
		//     stored before extraction next frame
		this->l2_to_sat_cells_after_sched[(*it).first] += fifo_stat.out_pkt_nbr;
		this->l2_to_sat_total_cells += fifo_stat.out_pkt_nbr;

		// write in statitics file
		this->probe_st_l2_to_sat_before_sched[(*it).first]->put(
			this->l2_to_sat_cells_before_sched[(*it).first] *
			this->up_return_pkt_hdl->getFixedLength() * 8 /
			this->frame_duration_ms);
		this->probe_st_l2_to_sat_after_sched[(*it).first]->put(
			this->l2_to_sat_cells_after_sched[(*it).first] *
			this->up_return_pkt_hdl->getFixedLength() * 8 /
			this->frame_duration_ms);

		this->probe_st_queue_size[(*it).first]->put(fifo_stat.current_pkt_nbr);
		this->probe_st_queue_size_kb[(*it).first]->put(
			((int) fifo_stat.current_length_bytes * 8 / 1000));
	}
	this->probe_st_l2_to_sat_total->put(
		this->l2_to_sat_total_cells *
		this->up_return_pkt_hdl->getFixedLength() * 8 /
		this->frame_duration_ms);
	this->probe_st_l2_from_sat->put(
		this->l2_from_sat_bytes * 8 / this->frame_duration_ms);
	this->probe_st_phy_from_sat->put(
		this->phy_from_sat_bytes * 8 / this->frame_duration_ms);
	this->probe_st_phy_to_sat->put(
		this->phy_to_sat_bytes * 8 / this->frame_duration_ms);

	// send all probes
	Output::sendProbes();

	// reset stat context for next frame
	this->resetStatsCxt();

}

/**
 * Reset statistics context
 * @return: none
 */
void BlockDvbTal::resetStatsCxt()
{
	for(unsigned int i = 0; i < this->dvb_fifos.size(); i++)
	{
		this->l2_to_sat_cells_before_sched[i] = 0;
		this->l2_to_sat_cells_after_sched[i] = 0;
	}
	this->l2_to_sat_total_cells = 0;
	this->l2_from_sat_bytes = 0;
	this->phy_from_sat_bytes = 0;
	this->phy_to_sat_bytes = 0;
}


/**
 * Delete packets in dvb_fifo
 */
void BlockDvbTal::deletePackets()
{
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->flush();
	}
}

// TODO move all upward initialization here
bool BlockDvbTal::DvbTalUpward::onInit()
{
	return true;
}

// TODO move all downward initialization here, move attributes and methods also
bool BlockDvbTal::DvbTalDownward::onInit()
{
	// here everyhing is initialized so we can do some processing

	// after all of things have been initialized successfully,
	// send a logon request
	UTI_DEBUG("send a logon request with MAC ID %d to NCC\n",
	          ((BlockDvbTal *)&this->block)->mac_id);
	((BlockDvbTal *)&this->block)->_state = state_wait_logon_resp;
	if(!((BlockDvbTal *)&this->block)->sendLogonReq())
	{
		UTI_ERROR("failed to send the logon request to the NCC");
		return false;
	}

	return true;
}

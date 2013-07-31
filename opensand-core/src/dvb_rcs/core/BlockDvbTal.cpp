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
 *        with Legacy Dama agent
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_DVB_RCS_TAL
#include <opensand_conf/uti_debug.h>
#define DVB_DBG_PREFIX "[Tal]"

#include "BlockDvbTal.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"

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
	m_pDamaAgent(NULL),
	m_carrierIdDvbCtrl(-1),
	m_carrierIdLogon(-1),
	m_carrierIdData(-1),
	complete_dvb_frames(),
	m_bbframe_dropped_rate(0),
	m_bbframe_dropped(0),
	m_bbframe_received(0),
	out_encap_packet_length(-1),
	out_encap_packet_type(MSG_TYPE_ERROR),
	in_encap_packet_length(-1),
	m_fixedBandwidth(-1),
	logon_timer(-1),
	frame_timer(-1),
	super_frame_counter(-1),
	frame_counter(-1),
	default_fifo_id(0),
	nbr_pvc(0),
	qos_server_host(),
	m_obrPeriod(-1),
	m_obrSlotFrame(-1),
	event_login_sent(NULL),
	event_login_complete(NULL),
	probe_st_terminal_queue_size(NULL),
	probe_st_real_in_thr(NULL),
	probe_st_real_out_thr(NULL),
	probe_st_phys_out_thr(NULL),
	probe_st_rbdc_req_size(NULL),
	probe_st_cra(NULL),
	probe_st_alloc_size(NULL),
	probe_st_unused_capacity(NULL),
	probe_st_bbframe_drop_rate(NULL),
	probe_st_real_modcod(NULL),
	probe_st_used_modcod(NULL)
{
	this->m_statCounters.ulOutgoingCells = NULL;
	this->m_statCounters.ulIncomingCells = NULL;
	this->m_statContext.ulOutgoingThroughput = NULL;
	this->m_statContext.ulIncomingThroughput = NULL;
}


/**
 * Destructor
 */
BlockDvbTal::~BlockDvbTal()
{
	if(this->m_pDamaAgent != NULL)
	{
		delete this->m_pDamaAgent;
	}

	deletePackets();
	// delete fifos
	for(map<unsigned int, DvbFifo *>::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();

	if(this->m_statCounters.ulOutgoingCells != NULL)
		delete[] this->m_statCounters.ulOutgoingCells;
	if(this->m_statCounters.ulIncomingCells != NULL)
		delete[] this->m_statCounters.ulIncomingCells;
	if(this->m_statContext.ulOutgoingThroughput != NULL)
		delete[] this->m_statContext.ulOutgoingThroughput;
	if(this->m_statContext.ulIncomingThroughput != NULL)
		delete[] this->m_statContext.ulIncomingThroughput;

	// close QoS Server socket if it was opened
	if(BlockDvbTal::qos_server_sock != -1)
	{
		close(BlockDvbTal::qos_server_sock);
	}

	// release the reception and emission DVB standards
	if(this->emissionStd != NULL)
	{
		delete this->emissionStd;
	}
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}
	
	// release the output arrays (no need to delete the probes)
	delete[] this->probe_st_terminal_queue_size;
	delete[] this->probe_st_real_in_thr;
	delete[] this->probe_st_real_out_thr;

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

			UTI_DEBUG("SF#%ld: encapsulation burst received (%d packets)\n",
			          this->super_frame_counter, burst->length());

			// set each packet of the burst in MAC FIFO
			
			for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
			{
				UTI_DEBUG_L3("SF#%ld: encapsulation packet has QoS value %d\n",
				             this->super_frame_counter, (*pkt_it)->getQos());

				fifo_priority = (*pkt_it)->getQos();
				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				// else use the default id

				if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
				{
					fifo_priority = this->default_fifo_id;
				}
				
				UTI_DEBUG("SF#%ld: store one encapsulation packet (QoS = %d)\n",
				          this->super_frame_counter, fifo_priority);


				// store the encapsulation packet in the FIFO
				if(this->emissionStd->onRcvEncapPacket(
				   *pkt_it,
				   this->dvb_fifos[fifo_priority],
				   this->getCurrentTime(),
				   0) < 0)
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					UTI_ERROR("SF#%ld: frame %ld: unable to "
					          "store received encapsulation "
					          "packet (see previous errors)\n",
					          this->super_frame_counter,
					          this->frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

				// update incoming counter (if packet is stored or sent)
				m_statCounters.ulIncomingCells[this->dvb_fifos[fifo_priority]->getPriority()]++;
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;

			// Cross layer information: if connected to QoS Server, build XML
			// message and send it
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
			for(map<unsigned int, DvbFifo *>::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				int nbFreeFrames = (*it).second->getMaxSize() -
				                   (*it).second->getCurrentSize();
				int nbFreeBits = nbFreeFrames * this->out_encap_packet_length * 8; // bits
				float macRate = nbFreeBits / this->frame_duration ; // bits/ms or kbits/s
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
			if(*event == this->frame_timer)
			{
				// beginning of a new frame
				if(this->_state == state_running)
				{
					UTI_DEBUG("SF#%ld: send encap bursts on timer basis\n",
					          this->super_frame_counter);

					if(this->processOnFrameTick() < 0)
					{
						// exit because the bloc is unable to continue
						UTI_ERROR("SF#%ld: treatments failed at frame %ld",
						          this->super_frame_counter, this->frame_counter);
						// Fatal error
						this->upward->reportError(true, "superframe treatment failed");
						return false;
					}
				}
			}
			else if(*event == this->logon_timer)
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
				UTI_ERROR("SF#%ld: unknown timer event received %s",
				          this->super_frame_counter, event->getName().c_str());
				return false;
			}
			break;

		default:
			UTI_ERROR("SF#%ld: unknown event received %s",
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

			// message from lower layer: DL dvb frame
			UTI_DEBUG_L3("SF#%ld DVB frame received (len %ld)\n",
				     this->super_frame_counter, len);

			if(!onRcvDvbFrame(dvb_frame, len))
			{
				UTI_DEBUG_L3("SF#%ld: failed to handle received DVB frame\n",
				             this->super_frame_counter);
				// a problem occured, trace is made in onRcvDVBFrame()
				// carry on simulation
				delete dvb_meta;
				return false;
			}
			delete dvb_meta;
		}
		break;

		default:
			UTI_ERROR("SF#%ld: unknown event received %s",
			          this->super_frame_counter, event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockDvbTal::initMode()
{
	this->emissionStd = new DvbRcsStd(this->up_return_pkt_hdl);
	if(this->emissionStd == NULL)
	{
		UTI_ERROR("failed to create the emission standard\n");
		goto error;
	}

	this->receptionStd = new DvbS2Std(this->down_forward_pkt_hdl);
	if(this->receptionStd == NULL)
	{
		UTI_ERROR("failed to create the reception standard\n");
		goto release_emission;
	}

	return true;

release_emission:
	delete(this->emissionStd);
error:
	return false;
}


bool BlockDvbTal::initParameters()
{
	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	if(!globalConfig.getValue(DVB_TAL_SECTION, DVB_RT_BANDWIDTH, this->m_fixedBandwidth))
	{
		UTI_ERROR("Missing %s", DVB_RT_BANDWIDTH);
		goto error;
	}
	UTI_INFO("fixed_bandwidth = %d kbits/s\n", this->m_fixedBandwidth);

	// Get the number of the row in modcod and dra files
	if(!globalConfig.getValueInList(DVB_SIMU_COL, COLUMN_LIST, TAL_ID,
	                                toString(this->mac_id), COLUMN_NBR,
	                                this->m_nbRow))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_SIMU_COL, COLUMN_LIST);
		goto error;
	}
	UTI_INFO("nb row = %d\n", this->m_nbRow);

	return true;

error:
	return false;
}


bool BlockDvbTal::initCarrierId()
{
	int val;

#define FMT_KEY_MISSING "SF#%ld %s missing from section %s\n", this->super_frame_counter

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

	UTI_INFO("SF#%ld: carrier IDs for Ctrl = %ld, Logon = %ld, Data = %ld\n",
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
		         "CR type %d/n", FUNCNAME,
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
	m_statContext.ulIncomingThroughput = new int[this->dvb_fifos.size()];
	if(this->m_statContext.ulIncomingThroughput == NULL)
		goto err_fifo_release;

	m_statContext.ulOutgoingThroughput = new int[this->dvb_fifos.size()];
	if(this->m_statContext.ulOutgoingThroughput == NULL)
		goto err_incoming_throughput_release;

	m_statCounters.ulIncomingCells = new int[this->dvb_fifos.size()];
	if(this->m_statCounters.ulIncomingCells == NULL)
		goto err_outgoing_throughput_release;

	m_statCounters.ulOutgoingCells = new int[this->dvb_fifos.size()];
	if(this->m_statCounters.ulOutgoingCells == NULL)
		goto err_incoming_cells_release;

	resetStatsCxt();

	return true;

err_incoming_cells_release:
	delete[] this->m_statCounters.ulIncomingCells;
err_outgoing_throughput_release:
	delete[] this->m_statContext.ulOutgoingThroughput;
err_incoming_throughput_release:
	delete[] this->m_statContext.ulIncomingThroughput;
err_fifo_release:
	for(map<unsigned int, DvbFifo *>::iterator it = this->dvb_fifos.begin();
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
	UTI_INFO("SF#%ld: MAC adress = %d, OBR period = %d, "
	         "OBR slot frame = %d\n", this->super_frame_counter,
	         this->mac_id, m_obrPeriod, m_obrSlotFrame);

	return true;
error:
	return false;
}


bool BlockDvbTal::initDama()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	rate_kbps_t max_rbdc_kbps;
	time_sf_t rbdc_timeout_sf = 0;
	vol_pkt_t max_vbdc_pkt;
	time_sf_t msl_sf = 0;
	bool cr_output_only;

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	if(!globalConfig.getValue(DA_TAL_SECTION, DA_MAX_RBDC_DATA, max_rbdc_kbps))
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
	if(!globalConfig.getValue(DA_TAL_SECTION, DA_MAX_VBDC_DATA, max_vbdc_pkt))
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
	         this->m_fixedBandwidth, max_rbdc_kbps,
	         rbdc_timeout_sf, max_vbdc_pkt, msl_sf,
	         cr_output_only);

	if(this->dama_algo == "Legacy")
	{
		UTI_INFO("%s SF#%ld: create Legacy DAMA agent\n", FUNCNAME,
		         this->super_frame_counter);
		m_pDamaAgent = new DamaAgentRcsLegacy(this->up_return_pkt_hdl,
		                                      this->dvb_fifos);
	}
	else if(this->dama_algo == "UoR")
	{
		UTI_INFO("%s SF#%ld: create UoR DAMA agent\n", FUNCNAME,
		         this->super_frame_counter);
		m_pDamaAgent = new DamaAgentRcsUor(this->up_return_pkt_hdl,
		                                   this->dvb_fifos);
	}
	// TODO remove here because DamaAgent and DAMACtrl choice will be separated
	else if(this->dama_algo == "Yes")
	{
		UTI_INFO("%s SF#%ld: no %s DAMA agent thus Legacy dama is used by default\n",
		         FUNCNAME, this->super_frame_counter, this->dama_algo.c_str());
		m_pDamaAgent = new DamaAgentRcsLegacy(this->up_return_pkt_hdl,
		                                      this->dvb_fifos);
		goto error;
	}
	else
	{
		UTI_ERROR("cannot create DAMA agent: algo named '%s' is not "
		          "managed by current MAC layer\n", this->dama_algo.c_str());
		goto error;
	}

	if(this->m_pDamaAgent == NULL)
	{
		UTI_ERROR("failed to create DAMA agent\n");
		goto error;
	}

	// Initialize the DamaAgent parent class
	if(!this->m_pDamaAgent->initParent(this->frame_duration,
	                                   this->m_fixedBandwidth,
	                                   max_rbdc_kbps,
	                                   rbdc_timeout_sf,
	                                   max_vbdc_pkt,
	                                   msl_sf,
	                                   this->m_obrPeriod,
	                                   cr_output_only))
	{

		UTI_ERROR("%s SF#%ld Dama Agent Initialization failed.\n", FUNCNAME,
		          this->super_frame_counter);
		goto err_agent_release;
	}

	// Initialize the DamaAgentRcsXXX class
	if(!m_pDamaAgent->init())
	{
		UTI_ERROR("%s Dama Agent initialization failed.\n", FUNCNAME);
		goto err_agent_release;
	}

	return true;

err_agent_release:
	delete m_pDamaAgent;
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
	this->probe_st_phys_out_thr =
		Output::registerProbe<int>("Physical_outgoing_throughput",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->probe_st_rbdc_req_size =
		Output::registerProbe<int>("RBDC_request_size", "Kbits/s", true, SAMPLE_LAST);
	this->probe_st_vbdc_req_size =
		Output::registerProbe<int>("VBDC_request_size", "Kbits/s", true, SAMPLE_LAST);
	this->probe_st_cra = Output::registerProbe<int>("CRA", "Kbits/s",
	                                                true, SAMPLE_LAST);
	this->probe_st_alloc_size = Output::registerProbe<int>("Allocation",
	                                                       "Kbits/s", true,
	                                                       SAMPLE_LAST);
	this->probe_st_unused_capacity =
		Output::registerProbe<int>("Unused_capacity", "time slots", true, SAMPLE_LAST);
	// FIXME: Unit?
	this->probe_st_bbframe_drop_rate =
		Output::registerProbe<float>("BBFrames_dropped_rate", true, SAMPLE_LAST);
	this->probe_st_real_modcod = Output::registerProbe<int>("Real_modcod",
	                                                        "modcod index",
	                                                        true, SAMPLE_LAST);
	this->probe_st_used_modcod = Output::registerProbe<int>("Received_modcod",
	                                                        "modcod index",
	                                                        true, SAMPLE_LAST);
	
	this->probe_st_terminal_queue_size = new Probe<int>*[this->dvb_fifos.size()];
	this->probe_st_real_in_thr = new Probe<int>*[this->dvb_fifos.size()];
	this->probe_st_real_out_thr = new Probe<int>*[this->dvb_fifos.size()];
	
	if(this->probe_st_terminal_queue_size == NULL ||
	   this->probe_st_real_in_thr == NULL ||
	   this->probe_st_real_out_thr == NULL)
	{
		UTI_ERROR("Failed to allocate memory for probe arrays");
		return false;
	}
	
	for(unsigned int i = 0 ; i < this->dvb_fifos.size() ; i++)
	{
		const char *fifo_type = fifo_types[i].c_str();
		char probe_name[32];
		
		snprintf(probe_name, sizeof(probe_name), "Terminal_queue_size.%s",
		         fifo_type);
		this->probe_st_terminal_queue_size[i] =
			Output::registerProbe<int>(probe_name, "cells", true, SAMPLE_AVG);
		
		snprintf(probe_name, sizeof(probe_name), "Real_incoming_throughput.%s",
		         fifo_type);
		this->probe_st_real_in_thr[i] = Output::registerProbe<int>(probe_name,
		                                                           "Kbits/s",
		                                                           true,
		                                                           SAMPLE_AVG);
		
		snprintf(probe_name, sizeof(probe_name), "Real_outgoing_throughput.%s",
		         fifo_type);
		this->probe_st_real_out_thr[i] = Output::registerProbe<int>(probe_name,
		                                                            "Kbits/s",
		                                                            true,
		                                                            SAMPLE_AVG);
	}
	
	return true;
}


bool BlockDvbTal::initDownwardTimers()
{
	this->logon_timer = this->downward->addTimerEvent("logon", 5000,
	                                                  false, // do not rearm
	                                                  false // do not start
	                                                  );
	this->frame_timer = this->downward->addTimerEvent("frame",
	                                                  DVB_TIMER_ADJUST(this->frame_duration),
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

	if(!this->initDownwardTimers())
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
		UTI_NOTICE("%s service on TCP/%d is not available\n", FUNCNAME,
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

		retptr = inet_ntop(address->ai_family, sin_addr, straddr, sizeof(straddr));
		if(retptr != NULL)
		{
			UTI_DEBUG("%s try IPv%d address %s\n", FUNCNAME, is_ipv4 ? 4 : 6, straddr);
		}
		else
		{
			UTI_DEBUG("%s try an IPv%d address\n", FUNCNAME, is_ipv4 ? 4 : 6);
		}

		BlockDvbTal::qos_server_sock = socket(address->ai_family, address->ai_socktype,
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
	ret = connect(BlockDvbTal::qos_server_sock, address->ai_addr, address->ai_addrlen);
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
	const char *FUNCNAME = DVB_DBG_PREFIX "[sendLogonReq]";
	T_DVB_LOGON_REQ *lp_logon_req;
	long l_size;

	// create a new DVB frame
	lp_logon_req = (T_DVB_LOGON_REQ *)malloc(sizeof(T_DVB_LOGON_REQ));
	if(!lp_logon_req)
	{
		UTI_ERROR("SF#%ld: failed to allocate memory for LOGON "
		          "request\n", this->super_frame_counter);
		goto error;
	}

	// build the DVB header
	l_size = sizeof(T_DVB_LOGON_REQ);
	lp_logon_req->hdr.msg_length = l_size;
	lp_logon_req->hdr.msg_type = MSG_TYPE_SESSION_LOGON_REQ;
	lp_logon_req->capa = 0; // TODO
	lp_logon_req->mac = this->mac_id;
	lp_logon_req->nb_row = m_nbRow;
	lp_logon_req->rt_bandwidth = m_fixedBandwidth;	/* in kbits/s */

	// send the message to the lower layer
	if(!this->sendDvbFrame((T_DVB_HDR *) lp_logon_req, m_carrierIdLogon, l_size))
	{
		UTI_ERROR("%s Failed to send Logon Request\n", FUNCNAME);
		goto free_logon_req;
	}
	UTI_DEBUG_L3("%s SF#%ld Logon Req. sent to lower layer\n", FUNCNAME,
	             this->super_frame_counter);

	if(!this->downward->startTimer(this->logon_timer))
	{
		UTI_ERROR("cannot start logon timer");
		goto free_logon_req;
	}

	// send the corresponding event
	Output::sendEvent(event_login_sent, "%s Login sent to %d", FUNCNAME,
	                  this->mac_id);

	return true;

free_logon_req:
	delete lp_logon_req;
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
			// keep statistics because data will be released before storing them
			unsigned int data_len = ((T_DVB_BBFRAME *) ip_buf)->dataLength;
			NetBurst *burst;

			if(this->receptionStd->onRcvFrame(ip_buf, i_len, hdr->msg_type,
			                                  this->mac_id, &burst) < 0)
			{
				UTI_ERROR("failed to handle the reception of "
				          "BB frame (length = %ld)\n", i_len);
				goto error;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}

			// update statistics
			this->m_statCounters.dlOutgoingCells += data_len;

			break;
		}

		case MSG_TYPE_DVB_BURST:
		{
			// keep statistics because data will be released before storing them
			unsigned int nb_packets = 0;
			NetBurst *burst;

			if(this->down_forward_pkt_hdl->getFixedLength() > 0)
			{
				nb_packets = (hdr->msg_length - sizeof(T_DVB_HDR)) /
				             this->down_forward_pkt_hdl->getFixedLength();
			}
			else
			{
				UTI_ERROR("packet size is not fixed\n");
				goto error;
			}

			if(this->receptionStd->onRcvFrame(ip_buf, i_len, hdr->msg_type,
			                                  this->mac_id, &burst) < 0)
			{
				UTI_ERROR("failed to handle the reception of "
				          "DVB frame (length = %ld)\n", i_len);
				goto error;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}

			// update statistics
			this->m_statCounters.dlOutgoingCells += nb_packets;
		}
		break;

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

			UTI_DEBUG("SF#%ld: received SOF in state %s\n",
			          this->super_frame_counter, state_descr);

			if(this->_state == state_running)
			{
				if(this->onStartOfFrame(ip_buf, i_len) < 0)
				{
					goto error;
				}
			}
			else
			{
				free(ip_buf);
			}
			break;

		// TBTP:
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_TBTP:
			if(this->_state == state_running)
			{
				if(!this->m_pDamaAgent->hereIsTTP(ip_buf, i_len))
				{
					free(ip_buf);
					goto error_on_TBTP;
				}
			}
			free(ip_buf);
			break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(this->onRcvLogonResp(ip_buf, i_len) < 0)
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
			UTI_INFO("SF#%ld: the message was corrupted by physical layer, "
			         "drop it", this->super_frame_counter);
			free(ip_buf);
			break;

		default:
			UTI_DEBUG_L3("SF#%ld: unknown type of DVB frame (%ld), ignore\n",
			             this->super_frame_counter, hdr->msg_type);
			free(ip_buf);
			goto error;
	}

	return true;

error_on_TBTP:
	UTI_ERROR("TBTP Treatments failed at SF# %ld, frame %ld",
	          this->super_frame_counter, this->frame_counter);
	return false;

error:
	UTI_ERROR("Treatments failed at SF# %ld, frame %ld",
	          this->super_frame_counter, this->frame_counter);
	return false;
}

/**
 * Send a capacity request for NRT data
 * @return -1 if failled, 0 if succeed
 */
int BlockDvbTal::sendCR()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[sendCR]";
	unsigned char *dvb_frame;
	size_t length;
	bool empty;
	CapacityRequest *capacity_request;

	// Get a dvb frame
	dvb_frame = (unsigned char *)malloc(MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX);
	if(dvb_frame == 0)
	{
		UTI_ERROR("%s SF#%ld frame %ld: cannot get memory from dvb_rcs "
		          "memory pool\n", FUNCNAME, this->super_frame_counter,
		          this->frame_counter);
		goto error;
	}

	// Set CR body
	// NB: cr_type parameter is not used here as CR is built for both
	// RBDC and VBDC
	if(!this->m_pDamaAgent->buildCR(cr_none,
	                                &capacity_request,
	                                empty))
	{
		free(dvb_frame);
		UTI_ERROR("%s SF#%ld frame %ld: DAMA cannot build CR\n", FUNCNAME,
		          this->super_frame_counter, this->frame_counter);
		goto error;
	}

	if(empty)
	{
		free(dvb_frame);
		UTI_DEBUG_L3("SF#%ld frame %ld: Empty CR\n",
		             this->super_frame_counter, this->frame_counter);
		return 0;
	}

	capacity_request->build(dvb_frame, length);
	delete capacity_request;

	// Send message
	if(!this->sendDvbFrame((T_DVB_HDR *) dvb_frame, m_carrierIdDvbCtrl, length))
	{
		UTI_ERROR("%s SF#%ld frame %ld: failed to allocate mgl msg\n",
				  FUNCNAME, this->super_frame_counter, this->frame_counter);
		free(dvb_frame);
		goto error;
	}

	UTI_DEBUG("%s SF#%ld frame %ld: CR sent\n", FUNCNAME,
			  this->super_frame_counter, this->frame_counter);

	return 0;

error:
	return -1;
}


/**
 * Upon reception of a SoF:
 * - update allocation with TBTP received last superframe (in DAMA agent)
 * - reset timers
 * @param ip_buf points to the dvb_rcs buffer containing SoF
 * @param i_len is the length of *ip_buf
 * @return 0 on success, -1 on failure
 */
int BlockDvbTal::onStartOfFrame(unsigned char *ip_buf, long i_len)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onStartOfFrame]";
	long sfn; // the superframe number piggybacked by SOF packet

	// store the superframe number;
	sfn = ((T_DVB_SOF *) ip_buf)->frame_nr;

	UTI_DEBUG_L3("%s sof reception SFN #%ld super frame nb %ld frame "
	             "counter %ld\n", FUNCNAME, sfn, this->super_frame_counter,
	             this->frame_counter);
	UTI_DEBUG("%s superframe number: %ld", FUNCNAME, sfn);

	// if the NCC crashed, we must reinitiate a logon
	if(sfn < this->super_frame_counter)
	{
		UTI_ERROR("SF#%ld: it seems NCC rebooted => flush buffer & "
		          "resend a logon request\n",
		          this->super_frame_counter);

		deletePackets();
		if(!sendLogonReq())
		{
			goto error;
		}

		this->_state = state_wait_logon_resp;
		this->super_frame_counter = sfn;
		this->frame_counter = -1;
		goto error;
	}

	// as long as the frame is changing, send all probes and event
	// FIXME: Still useful ? Events are sent automatically now
	Output::sendProbes();

	// update the frame numerotation
	this->super_frame_counter = sfn;

	// Inform dama agent
	if(!m_pDamaAgent->hereIsSOF(ip_buf, i_len))
		goto error;

	// There is a risk of unprecise timing so the following hack

	// ---- if we have consumed all frames of previous sf ----
	// ---- (or if it is the first frame)                 ----
	if(this->frame_counter == this->frames_per_superframe ||
	   this->frame_counter == -1)
	{
		UTI_DEBUG("%s SF#%ld frame %ld: all frames from previous SF are "
		          "consumed or it is the first frame\n", FUNCNAME,
		          this->super_frame_counter, this->frame_counter);

		// reset frame counter: it will be init to 1 (1st frame number)
		// at the beginning of processOnFrameTick()
		this->frame_counter = 0;

		// we have consumed all of our frames, we start a new one immediately
 		// this is the first frame of the new superframe
		if(this->processOnFrameTick() < 0)
		{
			// exit because the bloc is unable to continue
			UTI_ERROR("%s treatments at sf %ld, frame %ld failed\n",
			          FUNCNAME, this->super_frame_counter,
			          this->frame_counter);
			goto error;
		}
	}
	else
	{
		// ----  if we have not consumed all our frames (it is the risk) ----
		// else : frame_counter < frames_per_superframe
		// if we have not consumed all our frames (it is the risk)
		// Then there is, by design, a timer active, we have to leave it
		// as we cannot remove it
		// hence we do only a reassignation of frame_counter (the frame active
		// count now as one frame in our superframe)
		this->frame_counter = 0;
	}

	free(ip_buf);
	return 0;

error:
	free(ip_buf);
	return -1;
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
	UTI_DEBUG("SF#%ld: frame %ld: start processOnFrameTick\n",
	          this->super_frame_counter, this->frame_counter);

	// ------------ arm timer for next frame -----------
	// this is done at the beginning in order not to increase next frame
	// by current frame treatments delay
	if(this->frame_counter < this->frames_per_superframe)
	{
		if(!this->downward->startTimer(this->frame_timer))
		{
			UTI_ERROR("cannot start frame timer");
			goto error;
		}
	}

	// ---------- tell the DAMA agent that a new frame begins ----------
	// Inform dama agent, and update total Available Allocation
	// for current frame
	if(!this->m_pDamaAgent->processOnFrameTick())
	{
		UTI_ERROR("SF#%ld: frame %ld: failed to process frame tick\n",
	          this->super_frame_counter, this->frame_counter);
	    goto error;
	}

	// ---------- schedule and send data frames ---------
	// schedule packets extracted from DVB FIFOs according to
	// the algorithm defined in DAMA agent
	if(!this->m_pDamaAgent->uplinkSchedule(&this->complete_dvb_frames))
	{
		UTI_ERROR("SF#%ld: frame %ld: failed to schedule packets from DVB FIFOs\n",
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
		if(this->sendCR() < 0)
		{
			UTI_ERROR("failed to send Capacity Request\n");
			goto error;
		}
	}

	// ---------- Statistics ---------
	// trace statistics for current frame
	this->updateStatsOnFrame();
	this->updateStatsOnFrameAndEncap();

	return ret;

error:
	return -1;
}

/**
 * Manage logon response: inform dama and upper layer that the link is now up and running
 * @param ip_buf the pointer to the longon response message
 * @param l_len the lenght of the message
 * @return 0 on success, -1 on failure
 */
int BlockDvbTal::onRcvLogonResp(unsigned char *ip_buf, long l_len)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onRcvLogonResp]";
	T_LINK_UP *link_is_up;
	T_DVB_LOGON_RESP *lp_logon_resp;
	std::string name = __FUNCTION__;

	// Retrieve the Logon Response frame
	lp_logon_resp = (T_DVB_LOGON_RESP *) ip_buf;
	if(lp_logon_resp->mac != this->mac_id)
	{
		UTI_DEBUG("%s SF#%ld Loggon_resp for mac=%d, not %d\n", FUNCNAME,
		          this->super_frame_counter, lp_logon_resp->mac, this->mac_id);
		goto ok;
	}

	// Remember the id
	m_groupId = lp_logon_resp->group_id;
	m_talId = lp_logon_resp->logon_id;

	// Inform Dama agent
	m_pDamaAgent->hereIsLogonResp(ip_buf, l_len);

	// Send a link is up message to upper layer
	// link_is_up
	link_is_up = new T_LINK_UP;
	if(link_is_up == 0)
	{
		UTI_ERROR("%s SF#%ld Memory allocation error on link_is_up.\n",
		          FUNCNAME, this->super_frame_counter);
		goto error;
	}
	link_is_up->group_id = m_groupId;
	link_is_up->tal_id = m_talId;

	if(!this->sendUp((void **)(&link_is_up), sizeof(T_LINK_UP), msg_link_up))
	{
		UTI_ERROR("SF#%ld: failed to send link up message to upper layer",
		          this->super_frame_counter);
		delete link_is_up;
		goto error;
	}
	UTI_DEBUG_L3("%s SF#%ld Link is up msg sent to upper layer\n", FUNCNAME,
	             this->super_frame_counter);

	// Set the state to "running"
	this->_state = state_running;
	UTI_INFO("SF#%ld: logon succeeded, running as group %ld and logon %ld\n",
	         this->super_frame_counter, this->m_groupId, this->m_talId);

	// send the corresponding event

	Output::sendEvent(event_login_complete, "%s Login complete with MAC %d",
	                     FUNCNAME, this->mac_id);

 ok:
	free(ip_buf);
	return 0;
 error:
	free(ip_buf);
	return -1;
}


/**
 * Update statistics :
 *  - UL Incoming Throughput
 *  - UL Outgoing Throughput
 *  - DL Outgoing Throughput
 * These statistics will be updated when the DVB bloc receive a frame tick
 * because they depend on frame duration
 */
void BlockDvbTal::updateStatsOnFrame()
{
	mac_fifo_stat_context_t fifo_stat;
	//da_stat_context_t dama_stat;
	int ulOutgoingCells = 0;

	// DAMA agent stat
	//dama_stat = m_pDamaAgent->getStatsCxt();

	// MAC fifos stats
	for(map<unsigned int, DvbFifo *>::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->getStatsCxt(fifo_stat);

		// NB: mac queueing delay = fifo_stat.lastPkQueuingDelay
		// is writing at each UL cell emission by MAC layer and DA

		// compute UL incoming Throughput - in kbits/s
		m_statContext.ulIncomingThroughput[(*it).first] =
			(m_statCounters.ulIncomingCells[(*it).first]
			* this->up_return_pkt_hdl->getFixedLength() * 8) / this->frame_duration;

		// compute UL outgoing Throughput
		// NB: outgoingCells = cells directly sent from IP packets + cells
		//     stored before extraction next frame
		ulOutgoingCells = m_statCounters.ulOutgoingCells[(*it).first] +
		                  fifo_stat.out_pkt_nbr;
		m_statContext.ulOutgoingThroughput[(*it).first] = (ulOutgoingCells
			* this->up_return_pkt_hdl->getFixedLength() * 8) / this->frame_duration;

		// write in statitics file
		probe_st_real_in_thr[(*it).first]->put(m_statContext.ulIncomingThroughput[(*it).first]);
		probe_st_real_out_thr[(*it).first]->put(m_statContext.ulOutgoingThroughput[(*it).first]);
	}

	// outgoing DL throughput
	m_statContext.dlOutgoingThroughput =
		(m_statCounters.dlOutgoingCells *
		 this->down_forward_pkt_hdl->getFixedLength() * 8) /
		this->frame_duration;

	// write in statitics file
	probe_st_phys_out_thr->put(m_statContext.dlOutgoingThroughput);

	// reset stat context for next frame
	resetStatsCxt();
}

/**
 * Update statistics :
 *  - RBDC Request
 *  - VBDC Request
 *  - CRA Allocation
 *  - Allocation Size
 *  - Unused Capacity
 *  - BBFrame Drop Rate
 *  - Real Modcod
 *  - Used Modcod
 *  - Terminal queue size
 * These statistics will be updated when the DVB bloc receive a frame tick
 * and an UL encapsulation packet
 *
 */
void BlockDvbTal::updateStatsOnFrameAndEncap()
{
	mac_fifo_stat_context_t fifo_stat;
	if(m_bbframe_dropped != 0 ||  m_bbframe_received !=0)
	{
		m_bbframe_dropped_rate = ((float) m_bbframe_dropped) /
		                         ((float) (m_bbframe_dropped + m_bbframe_received));
		UTI_DEBUG("m_bbframe_dropped_rate : %f \n", m_bbframe_dropped_rate);
	}

	// DAMA agent stat
	const da_stat_context_t &dama_stat = m_pDamaAgent->getStatsCxt();

	// write in statitics file
	// COMMENTED by fab for merge from trunk (to opensand_trunk_dama)
	/*probe_st_rbdc_req_size->put(damaStat->rbdcRequest);
	probe_st_vbdc_req_size->put(damaStat->vbdcRequest);
	probe_st_cra->put(damaStat->craAlloc);
	probe_st_alloc_size->put(damaStat->globalAlloc);
	probe_st_unused_capacity->put(damaStat->unusedAlloc);
	probe_st_bbframe_drop_rate->put(m_bbframe_dropped_rate);
	probe_st_real_modcod->put(this->receptionStd->getRealModcod());
	probe_st_used_modcod->put(this->receptionStd->getReceivedModcod());*/
	
	probe_st_rbdc_req_size->put(dama_stat.rbdc_request_kbps);
	probe_st_vbdc_req_size->put(dama_stat.vbdc_request_pkt);
	probe_st_cra->put(dama_stat.cra_alloc_kbps);
	probe_st_alloc_size->put(dama_stat.global_alloc_kbps);
	probe_st_unused_capacity->put(dama_stat.unused_alloc_kbps);
	probe_st_bbframe_drop_rate->put(m_bbframe_dropped_rate);
	probe_st_real_modcod->put(this->receptionStd->getRealModcod());
	probe_st_used_modcod->put(this->receptionStd->getReceivedModcod());
	
	// MAC fifos stats
	for(map<unsigned int, DvbFifo *>::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->getStatsCxt(fifo_stat);

		// write in statitics file : mac queue size

		//probe_st_terminal_queue_size[fifoIndex]->put(macQStat.currentPkNb);
		probe_st_terminal_queue_size[(*it).first]->put(fifo_stat.current_pkt_nbr);
	}

	// Reset stats for next frame
	this->m_pDamaAgent->resetStatsCxt();
}


/**
 * Reset statistics context
 * @return: none
 */
void BlockDvbTal::resetStatsCxt()
{
	unsigned int i;
	m_statCounters.dlOutgoingCells = 0;
	m_statContext.dlOutgoingThroughput = 0;
	for(i = 0; i < this->dvb_fifos.size(); i++)
	{
		m_statContext.ulIncomingThroughput[i] = 0;
		m_statContext.ulOutgoingThroughput[i] = 0;
		m_statCounters.ulIncomingCells[i] = 0;
		m_statCounters.ulOutgoingCells[i] = 0;
	}
}


/**
 * Delete packets in dvb_fifo
 */
void BlockDvbTal::deletePackets()
{
	for(map<unsigned int, DvbFifo *>::iterator it = this->dvb_fifos.begin();
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

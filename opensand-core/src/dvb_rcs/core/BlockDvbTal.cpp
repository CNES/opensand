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
#include <opensand_output/Output.h>

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

int BlockDvbTal::Downward::Downward::qos_server_sock = -1;


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/


BlockDvbTal::BlockDvbTal(const string &name, tal_id_t UNUSED(mac_id)):
	BlockDvb(name)
{
}

BlockDvbTal::~BlockDvbTal()
{
}

bool BlockDvbTal::onInit(void)
{
	return true;
}


bool BlockDvbTal::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}


bool BlockDvbTal::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/

BlockDvbTal::Downward::Downward(Block *const bl, tal_id_t mac_id):
	DvbDownward(bl),
	mac_id(mac_id),
	state(state_initializing),
	group_id(),
	tal_id(),
	fixed_bandwidth(0),
	max_rbdc_kbps(0),
	max_vbdc_kb(0),
	dama_agent(NULL),
	frame_counter(),
	carrier_id_ctrl(),
	carrier_id_logon(),
	carrier_id_data(),
	dvb_fifos(),
	default_fifo_id(0),
	nbr_pvc(0),
	obr_period_frame(-1),
	obr_slot_frame(-1),
	frame_timer(-1),
	is_first_frame(true),
	complete_dvb_frames(),
	logon_timer(-1),
	cni(100),
	qos_server_host(),
	event_login_sent(NULL),
	event_login_complete(NULL),
	probe_st_queue_size(),
	probe_st_queue_size_kb(),
	probe_st_l2_to_sat_before_sched(),
	probe_st_l2_to_sat_after_sched(),
	probe_st_l2_to_sat_total(NULL),
	probe_st_l2_from_sat(NULL)
{
}

BlockDvbTal::Downward::~Downward()
{
	if(this->dama_agent != NULL)
	{
		delete this->dama_agent;
	}

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
	if(BlockDvbTal::Downward::Downward::qos_server_sock != -1)
	{
		close(BlockDvbTal::Downward::Downward::qos_server_sock);
	}

	this->complete_dvb_frames.clear();
}

bool BlockDvbTal::Downward::onInit(void)
{
	this->log_qos_server = Output::registerLog(LEVEL_WARNING, 
	                                           "Dvb.QoSServer");	
	this->log_frame_tick = Output::registerLog(LEVEL_WARNING, 
	                                           "Dvb.DamaAgent.FrameTick");	

	// get the common parameters
	if(!this->initCommon(UP_RETURN_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation");
		goto error;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation");
		goto error;
	}

	if(!this->initCarrierId())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the "
		    "initialisation");
		goto error;
	}

	if(!this->initMacFifo())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the MAC FIFO part of the "
		    "initialisation");
		goto error;
	}

	if(!this->initObr())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the OBR part of the "
		    "initialisation");
		goto error;
	}

	if(!this->initDama())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation");
		goto error;
	}

	if(!this->initQoSServer())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the QoS Server part of the "
		    "initialisation");
		goto error;
	}

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output");
		goto error;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialization of timers");
		goto error;
	}

	// now everyhing is initialized so we can do some processing

	// after all of things have been initialized successfully,
	// send a logon request
	LOG(this->log_init, LEVEL_DEBUG,
	    "send a logon request with MAC ID %d to NCC\n",
	    this->mac_id);
	this->state = state_wait_logon_resp;
	if(!this->sendLogonReq())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to send the logon request to the NCC\n");
		goto error;
	}

	return true;
error:
// TODO something to release ?
	return false;
}


bool BlockDvbTal::Downward::initCarrierId(void)
{
	// Get the ID for control carrier
	if(!Conf::getValue(DVB_TAL_SECTION,
	                   DVB_CAR_ID_CTRL,
	                   this->carrier_id_ctrl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_CTRL, DVB_TAL_SECTION);
		goto error;
	}

	// Get the ID for logon carrier
	if(!Conf::getValue(DVB_TAL_SECTION,
	                   DVB_CAR_ID_LOGON,
	                   this->carrier_id_logon))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_LOGON, DVB_TAL_SECTION);
		goto error;
	}

	// Get the ID for data carrier
	if(!Conf::getValue(DVB_TAL_SECTION,
	                   DVB_CAR_ID_DATA,
	                          this->carrier_id_data))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_DATA, DVB_TAL_SECTION);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: carrier IDs for Ctrl = %u, Logon = %u, "
	    "Data = %u\n", this->super_frame_counter,
	    this->carrier_id_ctrl,
	    this->carrier_id_logon, this->carrier_id_data);

	return true;
error:
	return false;
}

bool BlockDvbTal::Downward::initMacFifo(void)
{
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	if(!Conf::getListItems(DVB_TAL_SECTION, FIFO_LIST, fifo_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing fifo list", DVB_TAL_SECTION,
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
		if(!Conf::getAttributeValue(iter, FIFO_PRIO, fifo_priority))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_PRIO, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_mac_prio
		if(!Conf::getAttributeValue(iter, FIFO_TYPE, fifo_mac_prio))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_TYPE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_size
		if(!Conf::getAttributeValue(iter, FIFO_SIZE, fifo_size))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_SIZE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get pvc
		if(!Conf::getAttributeValue(iter, FIFO_PVC, pvc))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_PVC, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get the fifo CR type
		if(!Conf::getAttributeValue(iter, FIFO_CR_TYPE, fifo_cr_type))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_CR_TYPE, DVB_TAL_SECTION,
			    FIFO_LIST);
			goto err_fifo_release;
		}

		fifo = new DvbFifo(fifo_priority, fifo_mac_prio,
		                   fifo_cr_type, pvc, fifo_size);

		LOG(this->log_init, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "pvc %u, CR type %d\n",
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


bool BlockDvbTal::Downward::initObr(void)
{
	// get the OBR period - in number of frames
	if(!Conf::getValue(DVB_TAL_SECTION, DVB_OBR_PERIOD_DATA,
	                   this->obr_period_frame))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s", DVB_OBR_PERIOD_DATA);
		goto error;
	}

	// deduce the Obr slot position within the multi-frame, from the mac
	// address and the OBR period
	// ObrSlotFrame= MacAddress 'modulo' Obr Period
	// NB : ObrSlotFrame is within [0, Obr Period -1]
	this->obr_slot_frame = this->mac_id % this->obr_period_frame;
	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: MAC adress = %d, OBR period = %d, "
	    "OBR slot frame = %d\n", this->super_frame_counter,
	    this->mac_id, this->obr_period_frame, this->obr_slot_frame);

	return true;
error:
	return false;
}


bool BlockDvbTal::Downward::initDama(void)
{
	time_sf_t rbdc_timeout_sf = 0;
	time_sf_t msl_sf = 0;
	string dama_algo;
	bool cr_output_only;

	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	if(!Conf::getValue(DVB_TAL_SECTION, DVB_RT_BANDWIDTH,
	                   this->fixed_bandwidth))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DVB_RT_BANDWIDTH);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "fixed_bandwidth = %d kbits/s\n", this->fixed_bandwidth);

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	if(!Conf::getValue(DA_TAL_SECTION, DA_MAX_RBDC_DATA,
	                   this->max_rbdc_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n",
		    DA_MAX_RBDC_DATA);
		goto error;
	}

	if(!Conf::getValue(DA_TAL_SECTION,
	                   DA_RBDC_TIMEOUT_DATA, rbdc_timeout_sf))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n",
		    DA_RBDC_TIMEOUT_DATA);
		goto error;
	}

	// Max VBDC
	if(!Conf::getValue(DA_TAL_SECTION, DA_MAX_VBDC_DATA,
	                   this->max_vbdc_kb))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_MAX_VBDC_DATA);
		goto error;
	}

	// MSL duration -- in frames number
	if(!Conf::getValue(DA_TAL_SECTION, DA_MSL_DURATION, msl_sf))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_MSL_DURATION);
		goto error;
	}

	// CR computation rule
	if(!Conf::getValue(DA_TAL_SECTION, DA_CR_RULE, cr_output_only))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_CR_RULE);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "ULCarrierBw %d kbits/s, "
	    "RBDC max %d kbits/s, RBDC Timeout %d frame, "
	    "VBDC max %d kbits, mslDuration %d frames, "
	    "getIpOutputFifoSizeOnly %d\n",
	    this->fixed_bandwidth, this->max_rbdc_kbps,
	    rbdc_timeout_sf, this->max_vbdc_kb, msl_sf,
	    cr_output_only);

	// dama algorithm
	if(!Conf::getValue(DVB_TAL_SECTION, DAMA_ALGO,
	                   dama_algo))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_TAL_SECTION, DAMA_ALGO);
		goto error;
	}

	if(dama_algo == "Legacy")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create Legacy DAMA agent\n",
		    this->super_frame_counter);

		this->dama_agent = new DamaAgentRcsLegacy();
	}
	else if(dama_algo == "RrmQos")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create RrmQos DAMA agent\n",
		    this->super_frame_counter);

		this->dama_agent = new DamaAgentRcsRrmQos();
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot create DAMA agent: algo named '%s' is not "
		    "managed by current MAC layer\n", dama_algo.c_str());
		goto error;
	}

	if(this->dama_agent == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create DAMA agent\n");
		goto error;
	}

	// Initialize the DamaAgent parent class
	if(!this->dama_agent->initParent(this->frame_duration_ms,
	                                 this->fixed_bandwidth,
	                                 this->max_rbdc_kbps,
	                                 rbdc_timeout_sf,
	                                 this->max_vbdc_kb,
	                                 msl_sf,
	                                 this->obr_period_frame,
	                                 cr_output_only,
	                                 this->pkt_hdl,
	                                 this->dvb_fifos))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u Dama Agent Initialization failed.\n",
		    this->super_frame_counter);
		goto err_agent_release;
	}

	// Initialize the DamaAgentRcsXXX class
	if(!this->dama_agent->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Agent initialization failed.\n");
		goto err_agent_release;
	}

	return true;

err_agent_release:
	delete this->dama_agent;
error:
	return false;
}


bool BlockDvbTal::Downward::initQoSServer(void)
{
	// QoS Server: read hostname and port from configuration
	if(!Conf::getValue(SECTION_QOS_AGENT, QOS_SERVER_HOST,
	                   this->qos_server_host))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section %s, %s missing",
		    SECTION_QOS_AGENT, QOS_SERVER_HOST);
		goto error;
	}

	if(!Conf::getValue(SECTION_QOS_AGENT, QOS_SERVER_PORT,
	                   this->qos_server_port))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section %s, %s missing\n",
		    SECTION_QOS_AGENT, QOS_SERVER_PORT);
		goto error;
	}
	else if(this->qos_server_port <= 1024 || this->qos_server_port > 0xffff)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "QoS Server port (%d) not valid\n",
		    this->qos_server_port);
		goto error;
	}

	// QoS Server: catch the SIGFIFO signal that is sent to the process
	// when QoS Server kills the TCP connection
	if(signal(SIGPIPE, BlockDvbTal::Downward::closeQosSocket) == SIG_ERR)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "cannot catch signal SIGPIPE\n");
		goto error;
	}

	// QoS Server: try to connect to remote host
	this->connectToQoSServer();

	return true;
error:
	return false;
}

bool BlockDvbTal::Downward::initOutput(void)
{
	this->event_login_sent = Output::registerEvent("bloc_dvb:login_sent");
	this->event_login_complete = Output::registerEvent("bloc_dvb:login_complete");

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		const char *fifo_name = ((*it).second)->getName().data();
		unsigned int id = (*it).first;

		this->probe_st_queue_size[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_LAST,
			                           "Queue size.packets.%s", fifo_name);
		this->probe_st_queue_size_kb[id] =
			Output::registerProbe<int>("kbits", true, SAMPLE_LAST,
			                           "Queue size.%s", fifo_name);

		this->probe_st_l2_to_sat_before_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
			                           "Throughputs.L2_to_SAT_before_sched.%s",
			                           fifo_name);
		this->probe_st_l2_to_sat_after_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
			                           "Throughputs.L2_to_SAT_after_sched.%s",
			                           fifo_name);
	}
	this->probe_st_l2_to_sat_total =
		Output::registerProbe<int>("Throughputs.L2_to_SAT_after_sched.total",
		                           "Kbits/s", true, SAMPLE_AVG);
	return true;
}


bool BlockDvbTal::Downward::initTimers(void)
{
	this->logon_timer = this->addTimerEvent("logon", 5000,
	                                        false, // do not rearm
	                                        false // do not start
	                                        );
	this->frame_timer = this->addTimerEvent("frame",
	                                        DVB_TIMER_ADJUST(
	                                            this->frame_duration_ms),
	                                        false,
	                                        false);
	this->stats_timer = this->addTimerEvent("dvb_stats",
	                                        this->stats_period_ms);

	// QoS Server: check connection status in 5 seconds
	this->qos_server_timer = this->addTimerEvent("qos_server", 5000);

	return true;
}


bool BlockDvbTal::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// first handle specific messages
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				DvbFrame *frame = (DvbFrame *)((MessageEvent *)event)->getData();
				if(!this->handleDvbFrame(frame))
				{
					return false;
				}
				break;
			}
			// messages from upper layer: burst of encapsulation packets
			NetBurst *burst;
			NetBurst::iterator pkt_it;
			unsigned int fifo_priority;
			std::string message;
			std::ostringstream oss;
			int ret;

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: encapsulation burst received (%d "
			    "packets)\n", this->super_frame_counter,
			    burst->length());

			// set each packet of the burst in MAC FIFO

			for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "SF#%u: encapsulation packet has QoS value "
				    "%d\n", this->super_frame_counter,
				    (*pkt_it)->getQos());

				fifo_priority = (*pkt_it)->getQos();
				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				// else use the default id

				if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
				{
					fifo_priority = this->default_fifo_id;
				}

				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: store one encapsulation packet "
				    "(QoS = %d)\n", this->super_frame_counter,
				    fifo_priority);


				// store the encapsulation packet in the FIFO
				if(!this->onRcvEncapPacket(*pkt_it,
				                           this->dvb_fifos[fifo_priority],
				                           0))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					LOG(this->log_receive, LEVEL_ERROR,
					    "SF#%u: frame %u: unable to "
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
			if(BlockDvbTal::Downward::Downward::qos_server_sock == -1)
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
				int nbFreeBits = nbFreeFrames * this->pkt_hdl->getFixedLength() * 8; // bits
				float macRate = nbFreeBits / this->frame_duration_ms ; // bits/ms or kbits/s
				oss << "File=\"" << (int) macRate << "\" ";
				message.append(oss.str());
				oss.str("");
			}
			message.append("/>");
			message.append(" </Type>\n");
			message.append("</XMLQoSMessage>\n");

			ret = write(BlockDvbTal::Downward::Downward::qos_server_sock,
			            message.c_str(),
			            message.length());
			if(ret == -1)
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "failed to send message to QoS Server: %s "
				    "(%d)\n", strerror(errno), errno);
			}
		}
		break;

		case evt_timer:
			if(*event == this->frame_timer)
			{
				// beginning of a new frame
				if(this->state == state_running)
				{
					LOG(this->log_receive, LEVEL_INFO,
					    "SF#%u: send encap bursts on timer "
					    "basis\n", this->super_frame_counter);

					if(this->processOnFrameTick() < 0)
					{
						// exit because the bloc is unable to continue
						LOG(this->log_receive, LEVEL_ERROR,
						    "SF#%u: treatments failed at frame %u",
						    this->super_frame_counter,
						    this->frame_counter);
						// Fatal error
						this->reportError(true,
						                  "superframe treatment failed");
						return false;
					}
				}
			}
			else if(*event == this->logon_timer)
			{
				if(this->state == state_wait_logon_resp)
				{
					// send another logon_req and raise timer
					// only if we are in the good state
					LOG(this->log_receive, LEVEL_NOTICE,
					    "still no answer from NCC to the "
					    "logon request we sent for MAC ID %d, "
					    "send a new logon request\n",
					    this->mac_id);
					this->sendLogonReq();
				}
			}
			else if(*event == this->stats_timer)
			{
				this->updateStats();
			}
			else if(*event == this->qos_server_timer)
			{
				// try to re-connect to QoS Server if not already connected
				if(BlockDvbTal::Downward::Downward::qos_server_sock == -1)
				{
					if(!this->connectToQoSServer())
					{
						LOG(this->log_receive, LEVEL_INFO,
						    "failed to connect with QoS Server, "
						    "cannot send cross layer information");
					}
				}
			}
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "SF#%u: unknown timer event received %s",
				    this->super_frame_counter, event->getName().c_str());
				return false;
			}
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown event received %s",
			    this->super_frame_counter,
			    event->getName().c_str());
			return false;
	}

	return true;
}



bool BlockDvbTal::Downward::sendLogonReq(void)
{
	LogonRequest *logon_req = new LogonRequest(this->mac_id,
	                                           this->fixed_bandwidth,
	                                           this->max_rbdc_kbps,
	                                           this->max_vbdc_kb);

	// send the message to the lower layer
	if(!this->sendDvbFrame((DvbFrame *)logon_req,
		                   this->carrier_id_logon))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send Logon Request\n");
		goto error;
	}
	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u Logon Req. sent to lower layer\n",
	    this->super_frame_counter);

	if(!this->startTimer(this->logon_timer))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "cannot start logon timer");
		goto error;
	}

	// send the corresponding event
	Output::sendEvent(this->event_login_sent, "Login sent to GW");

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_CORRUPTED:
			if(!this->with_phy_layer)
			{
				delete dvb_frame;
				goto error;
			}
			// get ACM parameters that will be transmited to GW in SAC
			this->cni = dvb_frame->getCn();
			delete dvb_frame;
			break;

		case MSG_TYPE_SOF:
			if(!this->handleStartOfFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle SoF\n");
				delete dvb_frame;
				goto error;
			}
			delete dvb_frame;
			break;

		case MSG_TYPE_TTP:
		{
			// TODO Ttp *ttp = dynamic_cast<Ttp *>(dvb_frame);
			Ttp *ttp = (Ttp *)dvb_frame;
			if(!this->dama_agent->hereIsTTP(ttp))
			{
				delete dvb_frame;
				goto error_on_TTP;
			}
			delete dvb_frame;
		}
		break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->handleLogonResp(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle logon response\n");
				delete dvb_frame;
				goto error;
			}
			delete dvb_frame;
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}

	return true;

error_on_TTP:
	LOG(this->log_receive, LEVEL_ERROR,
	    "TTP Treatments failed at SF#%u, frame %u",
	    this->super_frame_counter, this->frame_counter);
	return false;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u, frame %u",
	    this->super_frame_counter, this->frame_counter);
	return false;
}


bool BlockDvbTal::Downward::sendSAC(void)
{
	bool empty;
	Sac *sac = new Sac(this->tal_id, this->group_id);

	// Set CR body
	// NB: cr_type parameter is not used here as CR is built for both
	// RBDC and VBDC
	if(!this->dama_agent->buildSAC(cr_none,
	                               sac,
	                               empty))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u frame %u: DAMA cannot build CR\n",
		    this->super_frame_counter, this->frame_counter);
		goto error;
	}
	// Set the ACM parameters
	if(this->with_phy_layer)
	{
		sac->setAcm(this->cni);
	}

	if(empty)
	{
		LOG(this->log_send, LEVEL_DEBUG,
		    "SF#%u frame %u: Empty CR\n",
		    this->super_frame_counter, this->frame_counter);
		// keep going as we can send ACM parameters
	}

	// Send message
	if(!this->sendDvbFrame((DvbFrame *)sac,
	                       this->carrier_id_ctrl))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u frame %u: failed to send SAC\n",
		    this->super_frame_counter, this->frame_counter);
		delete sac;
		goto error;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "SF#%u frame %u: SAC sent\n", this->super_frame_counter,
	    this->frame_counter);

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleStartOfFrame(DvbFrame *dvb_frame)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	LOG(this->log_frame_tick, LEVEL_DEBUG,
	    "SOF reception SFN #%u super frame nb %u frame "
	    "counter %u\n", sfn, this->super_frame_counter,
	    this->frame_counter);
	LOG(this->log_frame_tick, LEVEL_DEBUG,
	    "superframe number: %u", sfn);

	// if the NCC crashed, we must reinitiate a logon
	if(sfn < this->super_frame_counter &&
	   (sfn != 0 || (this->super_frame_counter + 1) % 65536 != 0))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: it seems NCC rebooted => flush buffer & "
		    "resend a logon request\n",
		    this->super_frame_counter);

		this->deletePackets();
		if(!this->sendLogonReq())
		{
			goto error;
		}

		this->state = state_wait_logon_resp;
		this->super_frame_counter = sfn;
		this->is_first_frame = true;
		this->frame_counter = 0;
		goto error;
	}

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
	   this->is_first_frame)
	{
		LOG(this->log_frame_tick, LEVEL_INFO,
		    "SF#%u frame %u: all frames from previous SF are "
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
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "SF#%u frame %u: treatments failed\n",
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

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::processOnFrameTick(void)
{
	int globalFrameNumber;

	// update frame counter for current SF - 1st frame within SF is 1 -
	this->frame_counter++;
	LOG(this->log_frame_tick, LEVEL_INFO,
	    "SF#%u: frame %u: start processOnFrameTick\n",
	    this->super_frame_counter, this->frame_counter);

	// ------------ arm timer for next frame -----------
	// this is done at the beginning in order not to increase next frame
	// by current frame treatments delay
	if(this->frame_counter < this->frames_per_superframe)
	{
		if(!this->startTimer(this->frame_timer))
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "cannot start frame timer");
			goto error;
		}
	}

	// ---------- tell the DAMA agent that a new frame begins ----------
	// Inform dama agent, and update total Available Allocation
	// for current frame
	if(!this->dama_agent->processOnFrameTick())
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: frame %u: failed to process frame tick\n",
		    this->super_frame_counter, this->frame_counter);
	    goto error;
	}

	// ---------- schedule and send data frames ---------
	// schedule packets extracted from DVB FIFOs according to
	// the algorithm defined in DAMA agent
	if(!this->dama_agent->returnSchedule(&this->complete_dvb_frames))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: frame %u: failed to schedule packets from DVB "
		    "FIFOs\n", this->super_frame_counter, this->frame_counter);
		goto error;
	}

	// send on the emulated DVB network the DVB frames that contain
	// the encapsulation packets scheduled by the DAMA agent algorithm
	if(!this->sendBursts(&this->complete_dvb_frames,
	                     this->carrier_id_data))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "failed to send bursts in DVB frames\n");
		goto error;
	}

	// ---------- SAC ----------
	// compute Capacity Request and send SAC...
	// only if the OBR period has been reached
	globalFrameNumber =
		(this->super_frame_counter - 1) * this->frames_per_superframe
		+ this->frame_counter;
	if((globalFrameNumber % this->obr_period_frame) == this->obr_slot_frame)
	{
		if(!this->sendSAC())
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "failed to send SAC\n");
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleLogonResp(DvbFrame *frame)
{
	// TODO static or dynamic_cast
	LogonResponse *logon_resp = (LogonResponse *)frame;
	// Remember the id
	this->group_id = logon_resp->getGroupId();
	this->tal_id = logon_resp->getLogonId();

	// Inform Dama agent
	if(!this->dama_agent->hereIsLogonResp(logon_resp))
	{
		return false;
	}

	// Set the state to "running"
	this->state = state_running;

	// send the corresponding event
	Output::sendEvent(event_login_complete, "Login complete with MAC %d",
	                  this->mac_id);

	return true;
}


void BlockDvbTal::Downward::updateStats(void)
{
	this->dama_agent->updateStatistics(this->stats_period_ms);

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
			this->pkt_hdl->getFixedLength() * 8 /
			this->stats_period_ms);
		this->probe_st_l2_to_sat_after_sched[(*it).first]->put(
			this->l2_to_sat_cells_after_sched[(*it).first] *
			this->pkt_hdl->getFixedLength() * 8 /
			this->stats_period_ms);

		this->probe_st_queue_size[(*it).first]->put(fifo_stat.current_pkt_nbr);
		this->probe_st_queue_size_kb[(*it).first]->put(
			((int) fifo_stat.current_length_bytes * 8 / 1000));
	}
	this->probe_st_l2_to_sat_total->put(
		this->l2_to_sat_total_cells *
		this->pkt_hdl->getFixedLength() * 8 /
		this->stats_period_ms);

	// reset stat context for next frame
	this->resetStatsCxt();

}

void BlockDvbTal::Downward::resetStatsCxt(void)
{
	for(unsigned int i = 0; i < this->dvb_fifos.size(); i++)
	{
		this->l2_to_sat_cells_before_sched[i] = 0;
		this->l2_to_sat_cells_after_sched[i] = 0;
	}
	this->l2_to_sat_total_cells = 0;
}


// TODO: move to a dedicated class
/**
 * Signal callback called upon SIGFIFO reception.
 *
 * This function is declared as static.
 *
 * @param sig  The signal that called the function
 */
void BlockDvbTal::Downward::closeQosSocket(int UNUSED(sig))
{
	// TODO static function, no this->
	DFLTLOG(LEVEL_NOTICE,
	        "TCP connection broken, close socket\n");
	close(BlockDvbTal::Downward::Downward::qos_server_sock);
	BlockDvbTal::Downward::qos_server_sock = -1;
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
bool BlockDvbTal::Downward::connectToQoSServer()
{
	struct addrinfo hints;
	struct protoent *tcp_proto;
	struct servent *serv;
	struct addrinfo *addresses;
	struct addrinfo *address;
	char straddr[INET6_ADDRSTRLEN];
	int ret;

	if(BlockDvbTal::Downward::qos_server_sock != -1)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "already connected to QoS Server, do not call this "
		    "function when already connected\n");
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
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "TCP is not available on the system\n");
		goto error;
	}
	hints.ai_protocol = tcp_proto->p_proto;

	// get service name
	serv = getservbyport(htons(this->qos_server_port), "tcp");
	if(serv == NULL)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "service on TCP/%d is not available\n",
		    this->qos_server_port);
		goto error;
	}

	// resolve hostname
	ret = getaddrinfo(this->qos_server_host.c_str(), serv->s_name, &hints, &addresses);
	if(ret != 0)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "cannot resolve hostname '%s': %s (%d)\n",
		    this->qos_server_host.c_str(),
		    gai_strerror(ret), ret);
		goto error;
	}

	// try to create socket with available addresses
	address = addresses;
	while(address != NULL && BlockDvbTal::Downward::qos_server_sock == -1)
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
			LOG(this->log_qos_server, LEVEL_INFO,
			    "try IPv%d address %s\n",
			    is_ipv4 ? 4 : 6, straddr);
		}
		else
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "try an IPv%d address\n",
			    is_ipv4 ? 4 : 6);
		}

		BlockDvbTal::Downward::qos_server_sock = socket(address->ai_family,
		                                      address->ai_socktype,
		                                      address->ai_protocol);
		if(BlockDvbTal::Downward::qos_server_sock == -1)
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "cannot create socket (%s) with address %s\n",
			    strerror(errno), straddr);
			address = address->ai_next;
			continue;
		}

		LOG(this->log_qos_server, LEVEL_INFO,
		    "socket created for address %s\n",
		    straddr);
	}

	if(BlockDvbTal::Downward::qos_server_sock == -1)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "no valid address found for hostname %s\n",
		    this->qos_server_host.c_str());
		goto free_dns;
	}

	LOG(this->log_qos_server, LEVEL_INFO,
	    "try to connect with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// try to connect with the socket
	ret = connect(BlockDvbTal::Downward::qos_server_sock,
	              address->ai_addr, address->ai_addrlen);
	if(ret == -1)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "connect() failed: %s (%d)\n",
		    strerror(errno), errno);
		LOG(this->log_qos_server, LEVEL_INFO,
		    "will retry to connect later\n");
		goto close_socket;
	}

	LOG(this->log_qos_server, LEVEL_NOTICE,
	    "connected with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// clean allocated addresses
	freeaddrinfo(addresses);

skip:
	return true;

close_socket:
	close(BlockDvbTal::Downward::qos_server_sock);
	BlockDvbTal::Downward::qos_server_sock = -1;
free_dns:
	freeaddrinfo(addresses);
error:
	return false;
}



/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbTal::Upward::Upward(Block *const bl, tal_id_t mac_id):
	DvbUpward(bl),
	mac_id(mac_id),
	group_id(),
	tal_id(),
	state(state_initializing),
	probe_st_l2_from_sat(NULL),
	probe_st_real_modcod(NULL),
	probe_st_received_modcod(NULL),
	probe_st_rejected_modcod(NULL),
	probe_sof_interval(NULL)
{
}


bool BlockDvbTal::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			if(this->probe_sof_interval->isEnabled() &&
			   dvb_frame->getMessageType() == MSG_TYPE_SOF)
			{
				struct timeval time = event->getTimeFromCustom();
				float val = time.tv_sec * 1000000L + time.tv_usec;
				event->setCustomTime();
				this->probe_sof_interval->put(val/1000);
			}

			// message from lower layer: DL dvb frame
			LOG(this->log_receive, LEVEL_DEBUG,
			    "SF#%u DVB frame received (len %u)\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageLength());

			if(!this->onRcvDvbFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "SF#%u: failed to handle received DVB frame\n",
				    this->super_frame_counter);
				// a problem occured, trace is made in onRcvDVBFrame()
				// carry on simulation
				return false;
			}
		}
		break;

		case evt_timer:
			if(*event == this->stats_timer)
			{
				this->updateStats();
			}
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown event received %s",
			    this->super_frame_counter,
			    event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockDvbTal::Upward::onInit(void)
{
	// get the common parameters
	if(!this->initCommon(DOWN_FORWARD_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation");
		return false;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output");
		return false;
	}

	this->stats_timer = this->addTimerEvent("dvb_stats",
	                                        this->stats_period_ms);

	return true;
}

// TODO remove receptionStd as functions are merged but contains part
//      dedicated to each host ?
bool BlockDvbTal::Upward::initMode(void)
{
	this->receptionStd = new DvbS2Std(this->pkt_hdl);
	if(this->receptionStd == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize reception standard\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvbTal::Upward::initOutput(void)
{
	if(!this->with_phy_layer)
	{
		// maximum modcod if physical layer is enabled => not useful
		this->probe_st_real_modcod = Output::registerProbe<int>("ACM.Required_modcod",
		                                                        "modcod index",
		                                                        true, SAMPLE_LAST);
	}
	this->probe_st_received_modcod = Output::registerProbe<int>("ACM.Received_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_st_rejected_modcod = Output::registerProbe<int>("ACM.Rejected_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_sof_interval = Output::registerProbe<float>("Perf.SOF_interval",
	                                                        "ms", true,
	                                                        SAMPLE_LAST);

	this->probe_st_l2_from_sat =
		Output::registerProbe<int>("Throughputs.L2_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	return true;
}


bool BlockDvbTal::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_CORRUPTED:
		{
			NetBurst *burst = NULL;
			DvbS2Std *std = (DvbS2Std *)this->receptionStd;

			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getMessageLength();
			this->l2_from_sat_bytes -= sizeof(T_DVB_HDR);

			if(this->with_phy_layer)
			{
				DvbFrame *frame_copy = new DvbFrame(dvb_frame);
				if(!this->shareFrame(frame_copy))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit Frame to opposite channel\n");
				}
			}

			if(!this->receptionStd->onRcvFrame(dvb_frame,
			                                   this->tal_id,
			                                   &burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle the reception of "
				    "BB frame (len = %u)\n",
				    dvb_frame->getMessageLength());
				goto error;
			}
			if(msg_type != MSG_TYPE_CORRUPTED)
			{
				// update MODCOD probes
				if(!this->with_phy_layer)
				{
					this->probe_st_real_modcod->put(std->getRealModcod());
				}
				this->probe_st_received_modcod->put(std->getReceivedModcod());
			}
			else
			{
				this->probe_st_rejected_modcod->put(std->getReceivedModcod());
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
			break;
		}

		// Start of frame (SOF):
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_SOF:
			// get superframe number
			if(!this->onStartOfFrame(dvb_frame))
			{
				delete dvb_frame;
				goto error;
			}
			// continue here
		case MSG_TYPE_TTP:
			const char *state_descr;

			if(this->state == state_running)
				state_descr = "state_running";
			else if(this->state == state_initializing)
				state_descr = "state_initializing";
			else
				state_descr = "other";

			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: received SOF or TTP in state %s\n",
			    this->super_frame_counter, state_descr);

			if(this->state == state_running)
			{
				if(!this->shareFrame(dvb_frame))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit TTP to opposite channel\n");
					goto error;
				}
			}
			else
			{
				delete dvb_frame;
			}
			break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->onRcvLogonResp(dvb_frame))
			{
				goto error;
			}
			break;

		// messages sent by current or another ST for the NCC --> ignore
		case MSG_TYPE_SAC:
		case MSG_TYPE_SESSION_LOGON_REQ:
			delete dvb_frame;
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}
	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u",
	    this->super_frame_counter);
	return false;
}


bool BlockDvbTal::Upward::shareFrame(DvbFrame *frame)
{
	if(!this->shareMessage((void **)&frame, sizeof(frame), msg_sig))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit frame to opposite channel\n");
		delete frame;
		return false;
	}
	return true;
}


bool BlockDvbTal::Upward::onStartOfFrame(DvbFrame *dvb_frame)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	// update the frame numerotation
	this->super_frame_counter = sfn;
	return true;
}



bool BlockDvbTal::Upward::onRcvLogonResp(DvbFrame *dvb_frame)
{
	T_LINK_UP *link_is_up;
	// TODO LogonResponse *logon_resp = dynamic_cast<LogonResponse *>(dvb_frame);
	LogonResponse *logon_resp = (LogonResponse *)(dvb_frame);

	// Retrieve the Logon Response frame
	if(logon_resp->getMac() != this->mac_id)
	{
		LOG(this->log_receive, LEVEL_INFO,
		    "SF#%u Loggon_resp for mac=%d, not %d\n",
		    this->super_frame_counter, logon_resp->getMac(),
		    this->mac_id);
		delete dvb_frame;
		goto ok;
	}

	// Remember the id
	this->group_id = logon_resp->getGroupId();
	this->tal_id = logon_resp->getLogonId();

	if(!this->shareFrame(dvb_frame))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit LogonResponse to opposite channel\n");
	}

	// Send a link is up message to upper layer
	// link_is_up
	link_is_up = new T_LINK_UP;
	if(link_is_up == 0)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u Memory allocation error on link_is_up\n",
		    this->super_frame_counter);
		goto error;
	}
	link_is_up->group_id = this->group_id;
	link_is_up->tal_id = this->tal_id;

	if(!this->enqueueMessage((void **)(&link_is_up),
	                         sizeof(T_LINK_UP),
	                         msg_link_up))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u: failed to send link up message to upper layer",
		    this->super_frame_counter);
		delete link_is_up;
		goto error;
	}
	LOG(this->log_receive, LEVEL_DEBUG,
	    "SF#%u Link is up msg sent to upper layer\n",
	    this->super_frame_counter);

	// Set the state to "running"
	this->state = state_running;
	LOG(this->log_receive, LEVEL_NOTICE,
	    "SF#%u: logon succeeded, running as group %u and logon"
	    " %u\n", this->super_frame_counter,
	    this->group_id, this->tal_id);

 ok:
	return true;
 error:
	 // do not delete here, this will be done by opposite channel
	return false;
}



void BlockDvbTal::Upward::updateStats(void)
{
	this->probe_st_l2_from_sat->put(
		this->l2_from_sat_bytes * 8 / this->stats_period_ms);

	// send all probes
	// in upward because this block has less events to handle => more time
	Output::sendProbes();

	// reset stat context for next frame
	this->resetStatsCxt();
}

void BlockDvbTal::Upward::resetStatsCxt(void)
{
	this->l2_from_sat_bytes = 0;
}


void BlockDvbTal::Downward::deletePackets()
{
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->flush();
	}
}


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
 * @file BlockDvbSat.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "BlockDvbSat.h"

#include "SatGw.h"
#include "Plugin.h"
#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "SlottedAlohaFrame.h"
#include "OpenSandModelConf.h"

#include "Logon.h"
#include "Logoff.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/Output.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>
#include <set>



/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/

BlockDvbSat::BlockDvbSat(const string &name):
	BlockDvb(name),
	gws()
{
}


// BlockDvbSat dtor
BlockDvbSat::~BlockDvbSat()
{
	sat_gws_t::iterator i_gw;

	// delete the satellite spots
	for(i_gw = this->gws.begin(); i_gw != this->gws.end(); ++i_gw)
	{
		delete i_gw->second;
	}
	
}


bool BlockDvbSat::onInit()
{
	// initialize the satellite spots
	if(!this->initSpots())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the spots part of the "
		    "initialisation\n");
		return false;
	}
	return true;
}

bool BlockDvbSat::initSpots(void)
{
	auto Conf = OpenSandModelConf::Get();

	std::vector<tal_id_t> gw_ids;
	if (!Conf->getGwIds(gw_ids))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "couldn't get the list of gateway IDs\n");
		return false;
	}

	for (auto& gw_id : gw_ids)
	{
		// Retrive FIFO sizes
		OpenSandModelConf::spot_infrastructure carriers;
		if (!Conf->getSpotInfrastructure(gw_id, carriers))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "couldn't create spot infrastructure for gw %d",
			    gw_id);
			return false;
		}

		uint8_t ctrl_id = carriers.ctrl_out.id;
		uint8_t data_in_gw_id = carriers.data_in_gw.id;
		uint8_t data_in_st_id = carriers.data_in_st.id;
		uint8_t data_out_gw_id = carriers.data_out_gw.id;
		uint8_t data_out_st_id = carriers.data_out_st.id;
		uint8_t log_id = carriers.logon_out.id;
	
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#: carrier IDs for Ctrl = %u, "
		    "data in gw = %u, data in st = %u, "
		    "data out gw = %u, data out st = %u, "
		    "log id = %u\n", 
		    ctrl_id, data_in_gw_id, data_in_st_id,
		    data_out_gw_id, data_out_st_id, log_id);
		//***************************
		// create a new gw
		//***************************
		SatGw *new_gw = new SatGw(gw_id, gw_id, carriers);
		new_gw->init();

		this->gws[gw_id] = new_gw;
		
		LOG(this->log_init, LEVEL_NOTICE,
		    "satellite spot %u: logon = %u, control = %u, "
		    "data out ST = %u, data out GW = %u\n",
		    gw_id, log_id, ctrl_id, data_out_st_id,
		    data_out_gw_id);
	}

	((Upward *)this->upward)->setGws(this->gws);
	((Downward *)this->downward)->setGws(this->gws);

	return true;
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/
BlockDvbSat::Downward::Downward(const string &name):
	DvbDownward(name),
	down_frame_counter(),
	fwd_timer(-1),
	terminal_affectation(),
	default_category(),
	fmt_groups(),
	gws(),
	probe_frame_interval(NULL)
{
};


BlockDvbSat::Downward::~Downward()
{

	// delete FMT groups here because they may be present in many carriers
	for(fmt_groups_t::iterator it = this->fmt_groups.begin();
		it != this->fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	this->terminal_affectation.clear();
}

bool BlockDvbSat::Downward::onInit()
{
	// get the common parameters
	if(!this->initCommon(TRANSPARENT_SATELLITE_NO_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common "
		    "initialisation\n");
		return false;
	}

	this->down_frame_counter = 0;

	if(!this->initSatLink())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of "
		    "link parameters\n");
		return false;
	}

	this->initStatsTimer(this->fwd_down_frame_duration_ms);

	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize Output probes ans stats\n");
		return false;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize timers\n");
		return false;
	}

	return true;
}

void BlockDvbSat::Downward::setGws(const sat_gws_t &gws)
{
	this->gws = gws;
}

bool BlockDvbSat::Downward::initOutput(void)
{
	this->probe_frame_interval = Output::Get()->registerProbe<float>(
				"Perf.Frames_interval", "ms", true, SAMPLE_LAST);

	return true;
}


bool BlockDvbSat::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				bool status = true;
				DvbFrame *dvb_frame;
				dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
				sat_gws_t::iterator gw;
				unsigned int carrier_id = dvb_frame->getCarrierId();
				tal_id_t gw_id;
				
				if(!OpenSandModelConf::Get()->getGwWithCarrierId(carrier_id, gw_id))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot find gateway with carrier ID %u in spot "
					    "list\n", carrier_id);
					delete dvb_frame;
					break;
				}

				if(gw_id != dvb_frame->getSpot())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Frame: wrong carrier id (%u) or gateway id (%u)\n",
					    carrier_id, dvb_frame->getSpot());
					delete dvb_frame;
					break;
				}

				SatGw *current_gw = this->gws[gw_id];
				if(current_gw == NULL)
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "Spot %u does'nt have gw %u\n", gw_id);
					delete dvb_frame;
					break;
				}

				if(dvb_frame->getMessageType() != MSG_TYPE_SOF)
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "Forwarded frame is not a SoF\n");
					status = false;
					delete dvb_frame;
					break;
				}

				// create a message for the DVB frame
				if(!this->sendDvbFrame(dvb_frame,
				                       current_gw->getControlCarrierId()))
				{
					LOG(this->log_send, LEVEL_ERROR,
					    "failed to send sig frame to lower layer, "
					    "drop it\n");
					status = false;
					delete dvb_frame;
				}
				return status;
			}

			if(!this->handleMessageBurst(event))
			{
				return false;
			}
		}
		break;

		case evt_timer:
		{
			if(*event == this->fwd_timer)
			{
				this->updateStats();
				// Update stats and probes
				if(this->probe_frame_interval->isEnabled())
				{
					timeval interval = event->getAndSetCustomTime();
					float val = interval.tv_sec * 1000000L + interval.tv_usec;
					this->probe_frame_interval->put(val / 1000);
				}

				// increment counter of superframes
				this->down_frame_counter++;
				LOG(this->log_receive, LEVEL_DEBUG,
				    "frame timer expired, send DVB frames\n");

				// send frame for every satellite spot
				for(sat_gws_t::iterator gw_iter = this->gws.begin();
					gw_iter != this->gws.end(); ++gw_iter)
				{

					SatGw *current_gw = gw_iter->second;
					LOG(this->log_send, LEVEL_DEBUG,
					    "send logon frames on satellite spot %u\n",
					    current_gw->getSpotId());
					if(!this->sendFrames(current_gw->getLogonFifo()))
					{
						LOG(this->log_send, LEVEL_ERROR,
						    "Failed to send logon frames on spot %u\n",
						    current_gw->getSpotId());
					}

					LOG(this->log_send, LEVEL_DEBUG,
					    "send control frames on satellite spot %u\n",
					    current_gw->getSpotId());
					if(!this->sendFrames(current_gw->getControlFifo()))
					{
						LOG(this->log_send, LEVEL_ERROR,
						    "Failed to send contol frames on spot %u\n",
						    current_gw->getSpotId());
					}

					if(!this->handleTimerEvent(current_gw))
					{
						return false;
					}
				}
			}
			else 
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unknown timer event received %s\n",
				    event->getName().c_str());
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event: %s\n", event->getName().c_str());
	}

	return true;
}



bool BlockDvbSat::Downward::sendFrames(DvbFifo *fifo)
{
	MacFifoElement *elem;
	//Be careful: When using other than time_ms_t in 32-bit machine, the RTT is no correct!
	time_ms_t current_time = getCurrentTime();

	while(((unsigned long)fifo->getTickOut()) <= current_time &&
	          fifo->getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;
		size_t length;

		elem = fifo->pop();
		assert(elem != NULL);

		dvb_frame = elem->getElem<DvbFrame>();
		length = dvb_frame->getTotalLength();

		// create a message for the DVB frame
		if(!this->sendDvbFrame(dvb_frame, fifo->getCarrierId()))
		{
			LOG(this->log_send, LEVEL_ERROR,
			    "failed to send message, drop the DVB or BB "
			    "frame\n");
			delete dvb_frame;
			goto error;
		}

		LOG(this->log_send, LEVEL_INFO,
		    "Frame sent with a size of %zu\n", length);

		delete elem;
	}

	return true;

error:
	delete elem;
	return false;
}

void BlockDvbSat::Downward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}
	// Update stats and probes
	sat_gws_t::iterator gw_iter;
	for(gw_iter = this->gws.begin();
	    gw_iter != this->gws.end(); ++gw_iter)
	{
		SatGw *current_gw = gw_iter->second;
		current_gw->updateProbes(this->stats_period_ms);
	}

	// Send probes
	Output::Get()->sendProbes();
}





/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/


BlockDvbSat::Upward::Upward(const string &name):
	DvbUpward(name),
	reception_std(NULL),
	gws()
{
};


BlockDvbSat::Upward::~Upward()
{
	// release the reception DVB standards
	if(this->reception_std != NULL)
	{
	   delete this->reception_std;
	}
}


void BlockDvbSat::Upward::setGws(const sat_gws_t &gws)
{
	this->gws = gws;
}

bool BlockDvbSat::Upward::onInit()
{
	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init, LEVEL_ERROR,
			"failed to initialize MOCODS definitions types\n");
		return false;
	}

	// get the common parameters
	if(!this->initCommon(TRANSPARENT_SATELLITE_NO_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	return true;
}


bool BlockDvbSat::Upward::initMode(void)
{
	// create the reception standard
	this->reception_std = new DvbRcs2Std(); 
	if(this->reception_std == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create the reception standard\n");
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvbSat::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message from lower layer: dvb frame
			DvbFrame *dvb_frame;
			dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
			
			if(!this->onRcvDvbFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle received DVB frame\n");
				delete dvb_frame;
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event: %s", event->getName().c_str());
			return false;
	}
	return true;
}

// About multithreaded channels implementation:
// We choose to let the transparent treatment and push in FIFO in Upward
// while we could have only transmitted the frame to the Downward channel
// that would have analysed it but with this solution brings better task
// sharing between channels
// The fifo is protected with a mutex
// The spots also protected for some shared elements
bool BlockDvbSat::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	tal_id_t gw_id;
	unsigned int carrier_id = dvb_frame->getCarrierId();
	bool corrupted = dvb_frame->isCorrupted();

	// get the satellite spot from which the DVB frame comes from
	if(!OpenSandModelConf::Get()->getGwWithCarrierId(carrier_id, gw_id))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot find gw id for carrier %u\n", carrier_id);
		return false;
	}

	SatGw *current_gw = this->gws[gw_id];
	if(current_gw == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot find gw id %d", 
		    gw_id);
		return false;
	}

	LOG(this->log_receive, LEVEL_DEBUG,
	    "DVB frame received from lower layer (type = %d, len %zu)\n",
	    dvb_frame->getMessageType(),
	    dvb_frame->getTotalLength());
	
	if(corrupted)
	{
		return this->handleCorrupted(dvb_frame); 
	}

	switch(dvb_frame->getMessageType())
	{
		case MSG_TYPE_DVB_BURST:
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "DVB-Frame received\n");

			// satellite spot found, forward DVB frame on the same spot
			DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();

			// Update probes and stats
			if(carrier_id == current_gw->getDataInStId())
			{
				// Update probes and stats
				current_gw->updateL2FromSt(frame->getPayloadLength());
			}
			else if(carrier_id == current_gw->getDataInGwId())
			{
				// Update probes and stats
				current_gw->updateL2FromGw(frame->getPayloadLength());
			}
			else
			{
				LOG(this->log_receive, LEVEL_CRITICAL,
				    "Wrong input carrier ID %u\n", carrier_id);
				return false;
			}

			/* The satellite is a transparent one 
			 * and the DVB frame contains a burst:
			 *  - extract the packets from the DVB frame,
			 *  - find the destination spot ID for each packet
			 *  - create a burst of encapsulation packets (NetBurst object)
			 *    with all the packets extracted from the DVB frame,
			 *  - send the burst to the upper layer.
			 */
			this->handleDvbBurst(dvb_frame, 
			                     current_gw);
		}
		break;
		
		/* forward the BB frame (and the burst that the frame contains) */
		// TODO see if we can factorize
		case MSG_TYPE_BBFRAME:
		{
			if(!this->handleBBFrame(dvb_frame, 
			                        current_gw))
			{
				return false;
			}
		}
		break;

		case MSG_TYPE_SALOHA_DATA:
		case MSG_TYPE_SALOHA_CTRL:
		{
			if(!this->handleSaloha(dvb_frame, 
			                       current_gw))
			{
				return false;
			}
		}
		break;

		// Generic control frames (SAC, TTP, etc)
		case MSG_TYPE_SAC:
		case MSG_TYPE_TTP:
		case MSG_TYPE_SYNC:
		case MSG_TYPE_SESSION_LOGON_RESP:
		{
			// forward the frame copy
			if(!this->forwardDvbFrame(current_gw->getControlFifo(),
			                          dvb_frame))
			{
				return false;
			}


		}
		break;

		// Special case of logon frame with dedicated channel
		case MSG_TYPE_SESSION_LOGON_REQ:
		{
			LogonRequest *logon_req = (LogonRequest*)dvb_frame;
			tal_id_t st_id = logon_req->getMac();

			if(!this->addSt(current_gw, st_id))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to register simulated ST with MAC "
				    "ID %u\n", st_id);
				return false;
			}
			
			// check for column in FMT simulation list
			LOG(this->log_receive, LEVEL_DEBUG,
			    "ST logon request received, "
			    "forward it on all satellite spots\n");

			// forward the frame copy
			if(!this->forwardDvbFrame(current_gw->getLogonFifo(),
			                          dvb_frame))
			{
				return false;
			}
			
		}
		break;

		case MSG_TYPE_SOF:
		{
			LOG(this->log_receive, LEVEL_DEBUG,
			    "control frame (type = %u) received, "
			    "forward it on all satellite spots\n",
			    dvb_frame->getMessageType());
			// the SOF message should not be stored in fifo, because it
			// would be kept a random amount of time between [0, fwd_timer]
			// and we need a perfect synchronization
			if(!this->shareMessage((void **)&dvb_frame, 
			                       sizeof(dvb_frame),
			                       msg_sig))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Unable to transmit sig to downward channel\n");
				return false;
			}
		}
		break;

		
		default:
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown type (%u) of DVB frame\n",
			    dvb_frame->getMessageType());
			delete dvb_frame;
		}
		break;
	}

	return true;
}

bool BlockDvbSat::Upward::forwardDvbFrame(DvbFifo *fifo, DvbFrame *dvb_frame)
{
	// TODO: do we still need the fifos now that the delay is implemented on sat_carrier ? 
	return this->pushInFifo(fifo, (NetContainer *)dvb_frame, 0);
}


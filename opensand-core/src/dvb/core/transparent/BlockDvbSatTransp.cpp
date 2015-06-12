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
 * @file BlockDvbSatTransp.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Bénédicte Motto <benedicte.motto@toulouse.viveris.com>
 */

#include "BlockDvbSatTransp.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "GenericSwitch.h"
#include "SlottedAlohaFrame.h"
#include "OpenSandConf.h"

#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>

#include <opensand_output/Output.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>


typedef struct
{
	tal_id_t tal_id;
	double cni;
} cni_info_t;


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/

BlockDvbSatTransp::BlockDvbSatTransp(const string &name):
	BlockDvbSat(name)
{
}


// BlockDvbSatTransp dtor
BlockDvbSatTransp::~BlockDvbSatTransp()
{
}


bool BlockDvbSatTransp::onUpwardEvent(const RtEvent *const event)
{
	return ((UpwardTransp *)this->upward)->onEvent(event);
}


bool BlockDvbSatTransp::onDownwardEvent(const RtEvent *const event)
{
	return ((DownwardTransp *)this->downward)->onEvent(event);
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/
BlockDvbSatTransp::DownwardTransp::DownwardTransp(Block *const bl):
	Downward(bl)
{
};


BlockDvbSatTransp::DownwardTransp::~DownwardTransp()
{
}


bool BlockDvbSatTransp::DownwardTransp::onInit()
{
	// get the common parameters
	// TODO no need to init pkt hdl in transparent mode,
	//      this will avoid loggers for encap to be instanciated
	if(!this->initCommon(FORWARD_DOWN_ENCAP_SCHEME_LIST))
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


bool BlockDvbSatTransp::DownwardTransp::initSatLink(void)
{
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               SAT_DELAY, this->sat_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, SAT_DELAY);
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "Satellite delay = %d\n", this->sat_delay);

	return true;
}


bool BlockDvbSatTransp::DownwardTransp::initStList(void)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "shouldn't initialise St list in transparent mode");

	return false;
}

bool BlockDvbSatTransp::DownwardTransp::initTimers(void)
{
	// create frame timer (also used to send packets waiting in fifo)
	this->fwd_timer = this->addTimerEvent("fwd_timer",
	                                       this->fwd_down_frame_duration_ms);

	return true;
}


bool BlockDvbSatTransp::DownwardTransp::handleMessageBurst(const RtEvent UNUSED(*const event))
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "message event while satellite is "
	    "transparent");

	return false;
}

bool BlockDvbSatTransp::DownwardTransp::handleTimerEvent(SatGw *current_gw,
                                                         uint8_t spot_id)
{
	// send frame for every satellite spot
	bool status = true;

	LOG(this->log_receive, LEVEL_DEBUG,
	    "send data frames on satellite spot "
	    "%u\n", spot_id);
	if(!this->sendFrames(current_gw->getDataOutGwFifo()))
	{
		status = false;
	}
	if(!this->sendFrames(current_gw->getDataOutStFifo()))
	{
		status = false;
	}
	if(!status)
	{
		return false;
	}

	return true;
}


/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/


BlockDvbSatTransp::UpwardTransp::UpwardTransp(Block *const bl):
	Upward(bl)
{
};


BlockDvbSatTransp::UpwardTransp::~UpwardTransp()
{
}


bool BlockDvbSatTransp::UpwardTransp::onInit()
{
	// get the common parameters
	// TODO no need to init pkt hdl in transparent mode,
	//      this will avoid loggers for encap to be instanciated
	if(!this->initCommon(RETURN_UP_ENCAP_SCHEME_LIST))
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


bool BlockDvbSatTransp::UpwardTransp::initMode(void)
{
	// Delay to apply to the medium
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION], 
		               SAT_DELAY, this->sat_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, SAT_DELAY);
		goto error;
	}
	    
	LOG(this->log_init, LEVEL_NOTICE,
	     "Satellite delay = %d", this->sat_delay);

	// create the reception standard
	this->reception_std = new DvbRcsStd(); 
	
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



bool BlockDvbSatTransp::UpwardTransp::initSwitchTable(void)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "shouldn't init switch table in transparent mode");

	return false;
}

bool BlockDvbSatTransp::UpwardTransp::handleDvbBurst(DvbFrame *dvb_frame, 
                                                     SatGw *current_gw,
                                                     SatSpot *current_spot)
{
	LOG(this->log_receive, LEVEL_INFO,
	    "DVB burst comes from spot %u (carrier "
	    "%u) => forward it to spot %u (carrier "
	    "%u)\n", current_spot->getSpotId(),
	    dvb_frame->getCarrierId(),
	    current_spot->getSpotId(),
	    current_gw->getDataOutGwFifo()->getCarrierId());

	if(!this->forwardDvbFrame(current_gw->getDataOutGwFifo(),
	                          dvb_frame))
	{
		return false;
	}
	return true;
}

bool BlockDvbSatTransp::UpwardTransp::handleSac(DvbFrame UNUSED(*dvb_frame), 
		                                        SatGw UNUSED(*current_gw))
{
	return true;
}

bool BlockDvbSatTransp::UpwardTransp::handleBBFrame(DvbFrame *dvb_frame, 
                                                    SatGw *current_gw,
                                                    SatSpot *current_spot)
{
	DvbFifo *out_fifo = NULL;
	unsigned int carrier_id = dvb_frame->getCarrierId();

	// TODO remove is SCPC supports REGEN
	LOG(this->log_receive, LEVEL_INFO,
	    "BBFrame received\n");

	// satellite spot found, forward BBframe on the same spot
	BBFrame *bbframe = dvb_frame->operator BBFrame*();

	// Check were the frame is coming from
	// GW if S2, ST if SCPC
	if(carrier_id == current_gw->getDataInGwId())
	{
		// Update probes and stats
		current_gw->updateL2FromGw(bbframe->getPayloadLength());
		out_fifo = current_gw->getDataOutStFifo();
	}
	else if(carrier_id == current_gw->getDataInStId())
	{
		// Update probes and stats
		current_gw->updateL2FromSt(bbframe->getPayloadLength());
		out_fifo = current_gw->getDataOutGwFifo();
	}
	else
	{
		LOG(this->log_receive, LEVEL_CRITICAL,
		    "Wrong input carrier ID %u\n", carrier_id);
		return false;
	}
	// TODO: forward according to a table
	LOG(this->log_receive, LEVEL_INFO,
	    "BBFRAME burst comes from spot %u (carrier "
	    "%u) => forward it to spot %u (carrier %u)\n",
	    current_spot->getSpotId(),
	    dvb_frame->getCarrierId(),
	    current_spot->getSpotId(),
	    out_fifo->getCarrierId());
	
	if(!this->forwardDvbFrame(out_fifo,
	                          dvb_frame))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot forward burst\n");
		return false;
	}

	return true;
}


bool BlockDvbSatTransp::UpwardTransp::handleSaloha(DvbFrame *dvb_frame,
                                                   SatGw *current_gw,
                                                   SatSpot *current_spot)
{
	LOG(this->log_receive, LEVEL_INFO,
	    "Slotted Aloha frame received\n");

	DvbFifo *fifo;
		
	// satellite spot found, forward frame on the same spot
	SlottedAlohaFrame *sa_frame = dvb_frame->operator SlottedAlohaFrame*();

	// Update probes and stats
	current_gw->updateL2FromSt(sa_frame->getPayloadLength());

	if(dvb_frame->getMessageType() == MSG_TYPE_SALOHA_DATA)
	{
		fifo = current_gw->getDataOutGwFifo();
	}
	else
	{
		fifo = current_gw->getDataOutStFifo();
	}

	// TODO: forward according to a table
	LOG(this->log_receive, LEVEL_INFO,
	    "Slotted Aloha frame comes from spot %u (carrier "
	    "%u) => forward it to spot %u (carrier %u)\n",
	    current_spot->getSpotId(),
	    dvb_frame->getCarrierId(),
	    current_spot->getSpotId(),
	    fifo->getCarrierId());

	if(!this->forwardDvbFrame(fifo,
	                          dvb_frame))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot forward burst\n");
		return false;
	}

	return true;
}

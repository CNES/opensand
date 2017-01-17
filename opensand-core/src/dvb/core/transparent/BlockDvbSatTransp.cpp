/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
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

/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/
BlockDvbSatTransp::DownwardTransp::DownwardTransp(const string &name):
	Downward(name)
{
};


BlockDvbSatTransp::DownwardTransp::~DownwardTransp()
{
}


bool BlockDvbSatTransp::DownwardTransp::initSatLink(void)
{
	return true;
}


bool BlockDvbSatTransp::DownwardTransp::initTimers(void)
{
	// create satellite delay timer, if there is a refresh period
  if(this->sat_delay->getRefreshPeriod())
  {
    this->sat_delay_timer = this->addTimerEvent("sat_delay",
                                                this->sat_delay->getRefreshPeriod());
  }
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

bool BlockDvbSatTransp::DownwardTransp::handleTimerEvent(SatGw *current_gw)
{
	// send frame for every satellite spot
	bool status = true;

	LOG(this->log_receive, LEVEL_DEBUG,
	    "send data frames on satellite spot "
	    "%u\n", current_gw->getSpotId());
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

bool BlockDvbSatTransp::DownwardTransp::handleScenarioTimer(SatGw *UNUSED(current_gw))
{
	assert(0);
}

/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/


BlockDvbSatTransp::UpwardTransp::UpwardTransp(const string &name):
	Upward(name)
{
};


BlockDvbSatTransp::UpwardTransp::~UpwardTransp()
{
}


bool BlockDvbSatTransp::UpwardTransp::initMode(void)
{
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

bool BlockDvbSatTransp::UpwardTransp::addSt(SatGw *UNUSED(current_gw),
                                            tal_id_t UNUSED(st_id))
{
	return true;
}

bool BlockDvbSatTransp::UpwardTransp::handleCorrupted(DvbFrame *dvb_frame)
{
	// in transparent scenario, satellite physical layer cannot corrupt
	LOG(this->log_receive, LEVEL_INFO,
	    "the message was corrupted by physical layer, "
	    "drop it\n");

	delete dvb_frame;
	dvb_frame = NULL;
		
	return true;
}

bool BlockDvbSatTransp::UpwardTransp::handleDvbBurst(DvbFrame *dvb_frame, 
                                                     SatGw *current_gw)
{
	LOG(this->log_receive, LEVEL_INFO,
	    "DVB burst comes from spot %u (carrier "
	    "%u) => forward it to spot %u (carrier "
	    "%u)\n", current_gw->getSpotId(),
	    dvb_frame->getCarrierId(),
	    current_gw->getSpotId(),
	    current_gw->getDataOutGwFifo()->getCarrierId());

	if(!this->forwardDvbFrame(current_gw->getDataOutGwFifo(),
	                          dvb_frame))
	{
		return false;
	}
	return true;
}

// DO something only on regenerative mode
bool BlockDvbSatTransp::UpwardTransp::handleSac(DvbFrame *UNUSED(dvb_frame),
                                                SatGw *UNUSED(current_gw))
{
	return true;
}

bool BlockDvbSatTransp::UpwardTransp::handleBBFrame(DvbFrame *dvb_frame, 
                                                    SatGw *current_gw)
{
	DvbFifo *out_fifo = NULL;
	unsigned int carrier_id = dvb_frame->getCarrierId();

	// TODO remove if SCPC supports REGEN
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
	    current_gw->getSpotId(),
	    dvb_frame->getCarrierId(),
	    current_gw->getSpotId(),
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
                                                   SatGw *current_gw)
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
	    current_gw->getSpotId(),
	    dvb_frame->getCarrierId(),
	    current_gw->getSpotId(),
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


bool BlockDvbSatTransp::UpwardTransp::updateSeriesGenerator(void)
{
	return true;
}

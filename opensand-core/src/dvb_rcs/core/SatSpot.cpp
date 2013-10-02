/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file SatSpot.cpp
 * @brief This bloc implements a satellite spots
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#define DBG_PACKAGE PKG_DVB_RCS_SAT
#include <opensand_conf/uti_debug.h>

#include "SatSpot.h"
#include "OpenSandFrames.h"
#include "MacFifoElement.h"
#include "ForwardSchedulingS2.h"

#include <stdlib.h>

// Declaring SatSpot necessary ctor and dtor
SatSpot::SatSpot():
	spot_id(-1),
	data_in_carrier_id(-1),
	control_fifo(),
	logon_fifo(),
	data_out_gw_fifo(),
	data_out_st_fifo(),
	complete_dvb_frames(),
	scheduling(NULL)
{
	// for statistics purpose
	this->data_stat.previous_tick = times(NULL);
	this->data_stat.sum_data = 0;
}

SatSpot::~SatSpot()
{
	this->complete_dvb_frames.clear();

	// clear logon fifo
	this->logon_fifo.flush();

	// clear control fifo
	this->control_fifo.flush();

	// clear data OUT ST fifo
	this->data_out_st_fifo.flush();

	// clear data OUT GW fifo
	this->data_out_gw_fifo.flush();

	// remove scheduling (only for regenerative satellite)
	if(scheduling)
		delete this->scheduling;
}


// TODO add spot fifos in configuration
// TODO size per fifo ?
bool SatSpot::initFifos(spot_id_t spot_id,
                        unsigned int data_in_carrier_id,
                        unsigned int log_id,
                        unsigned int ctrl_id,
                        unsigned int data_out_st_id,
                        unsigned int data_out_gw_id,
                        size_t fifo_size)
{
	this->spot_id = spot_id;
	this->data_in_carrier_id = data_in_carrier_id;

	// initialize MAC FIFOs
#define SIG_FIFO_SIZE 50
	this->logon_fifo.init(log_id, SIG_FIFO_SIZE, "logon_fifo");
	this->control_fifo.init(ctrl_id, SIG_FIFO_SIZE, "control_fifo");
	this->data_out_st_fifo.init(data_out_st_id, fifo_size, "data_out_st");
	this->data_out_gw_fifo.init(data_out_gw_id, fifo_size, "data_out_gw");

	return true;
}

bool SatSpot::initScheduling(const EncapPlugin::EncapPacketHandler *pkt_hdl,
                             FmtSimulation *const fmt_simu,
                             const TerminalCategory *const category,
                             unsigned int frames_per_superframe)
{
	// TODO if this is not usefull anymore, inherit from scheduling
	fifos_t fifos;
	fifos[this->data_out_st_fifo.getCarrierId()] = &this->data_out_st_fifo;
	this->scheduling = new ForwardSchedulingS2(pkt_hdl,
	                                           fifos,
	                                           frames_per_superframe,
	                                           fmt_simu,
	                                           category);
	if(!this->scheduling)
	{
		UTI_ERROR("cannot create down scheduling for spot %u\n", this->spot_id);
		return false;
	}
	return true;
}

uint8_t SatSpot::getSpotId()
{
	return this->spot_id;
}


bool SatSpot::schedule(const time_sf_t current_superframe_sf,
                       const time_frame_t current_frame,
                       clock_t current_time)
{
	// not used by scheduling here
	uint32_t remaining_allocation = 0;

	if(!scheduling)
	{
		return false;
	}

	return this->scheduling->schedule(current_superframe_sf,
	                                  current_frame,
	                                  current_time,
	                                  &this->complete_dvb_frames,
	                                  remaining_allocation);
}


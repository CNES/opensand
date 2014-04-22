/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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


#include "SatSpot.h"
#include "OpenSandFrames.h"
#include "MacFifoElement.h"
#include "ForwardSchedulingS2.h"

#include <opensand_output/Output.h>

#include <stdlib.h>

// TODO add spot fifos in configuration
// TODO size per fifo ?
// TODO to not do...: do not create all fifo in the regenerative case
SatSpot::SatSpot(spot_id_t spot_id,
                 uint8_t data_in_carrier_id,
                 uint8_t log_id,
                 uint8_t ctrl_id,
                 uint8_t data_out_st_id,
                 uint8_t data_out_gw_id,
                 size_t fifo_size):
	spot_id(spot_id),
	data_in_carrier_id(data_in_carrier_id),
	logon_carrier_id(log_id),
	ctrl_carrier_id(ctrl_id),
	complete_dvb_frames(),
	scheduling(NULL),
	l2_from_st_bytes(0),
	l2_from_gw_bytes(0),
	spot_mutex("Spot")
{
	// initialize MAC FIFOs
	this->data_out_st_fifo = new DvbFifo(data_out_st_id, fifo_size, "data_out_st");
	this->data_out_gw_fifo = new DvbFifo(data_out_gw_id, fifo_size, "data_out_gw");

	// Output Log
	this->log_init = Output::registerLog(LEVEL_WARNING, "Dvb.init");
}

SatSpot::~SatSpot()
{
	this->complete_dvb_frames.clear();

	// remove scheduling (only for regenerative satellite)
	if(scheduling)
		delete this->scheduling;

	delete this->data_out_st_fifo;
	delete this->data_out_gw_fifo;
}

bool SatSpot::initScheduling(const EncapPlugin::EncapPacketHandler *pkt_hdl,
                             FmtSimulation *const fwd_fmt_simu,
                             const TerminalCategory *const category)
{
	fifos_t fifos;
	fifos[this->data_out_st_fifo->getCarrierId()] = this->data_out_st_fifo;
	this->scheduling = new ForwardSchedulingS2(pkt_hdl,
	                                           fifos,
	                                           fwd_fmt_simu,
	                                           category);
	if(!this->scheduling)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "cannot create down scheduling for spot %u\n",
		    this->spot_id);
		return false;
	}
	return true;
}


bool SatSpot::schedule(const time_sf_t current_superframe_sf,
                       clock_t current_time)
{
	// not used by scheduling here
	uint32_t remaining_allocation = 0;

	if(!scheduling)
	{
		return false;
	}

	return this->scheduling->schedule(current_superframe_sf,
	                                  0,
	                                  current_time,
	                                  &this->complete_dvb_frames,
	                                  remaining_allocation);
}

uint8_t SatSpot::getSpotId(void) const
{
	return this->spot_id;
}

uint8_t SatSpot::getInputCarrierId(void) const
{
	return this->data_in_carrier_id;
}

DvbFifo *SatSpot::getDataOutStFifo(void) const
{
	return this->data_out_st_fifo;
}

DvbFifo *SatSpot::getDataOutGwFifo(void) const
{
	return this->data_out_gw_fifo;
}

uint8_t SatSpot::getControlCarrierId(void) const
{
	return this->ctrl_carrier_id;
}

uint8_t SatSpot::getLogonCarrierId(void) const
{
	return this->logon_carrier_id;
}

list<DvbFrame *> &SatSpot::getCompleteDvbFrames(void)
{
	return this->complete_dvb_frames;
}

void SatSpot::updateL2FromSt(vol_bytes_t bytes)
{
	RtLock lock(this->spot_mutex);
	this->l2_from_st_bytes += bytes;
}

void SatSpot::updateL2FromGw(vol_bytes_t bytes)
{
	RtLock lock(this->spot_mutex);
	this->l2_from_gw_bytes += bytes;
}

vol_bytes_t SatSpot::getL2FromSt(void)
{
	RtLock lock(this->spot_mutex);
	vol_bytes_t val = this->l2_from_st_bytes;
	this->l2_from_st_bytes = 0;
	return val;
}

vol_bytes_t SatSpot::getL2FromGw(void)
{
	RtLock lock(this->spot_mutex);
	vol_bytes_t val = this->l2_from_gw_bytes;
	this->l2_from_gw_bytes = 0;
	return val;
}


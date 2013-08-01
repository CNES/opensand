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


#include "SatSpot.h"
#include "msg_dvb_rcs.h"
#include "MacFifoElement.h"

#define DBG_PACKAGE PKG_DVB_RCS_SAT
#include <opensand_conf/uti_debug.h>

#include <stdlib.h>

// Declaring SatSpot necessary ctor and dtor
SatSpot::SatSpot():
	m_ctrlFifo(),
	m_logonFifo(),
	m_dataOutGwFifo(),
	m_dataOutStFifo(),
	complete_dvb_frames()
{
	this->m_spotId = -1;
	this->m_dataInId = -1;

	// for statistics purpose
	m_dataStat.previous_tick = times(NULL);
	m_dataStat.sum_data = 0;
}

SatSpot::~SatSpot()
{
	this->complete_dvb_frames.clear();

	// clear logon fifo
	this->m_logonFifo.flush();

	// clear control fifo
	this->m_ctrlFifo.flush();

	// clear data OUT ST fifo
	this->m_dataOutStFifo.flush();

	// clear data OUT GW fifo
	this->m_dataOutGwFifo.flush();
}


// TODO add spot fifos in configuration
int SatSpot::init(long spotId, long logId, long ctrlId,
                  long dataInId, long dataOutStId, long dataOutGwId)
{
	m_spotId = spotId;

	// initialize MAC FIFOs
#define FIFO_SIZE 5000
	m_logonFifo.init(logId, FIFO_SIZE, "logon_Fifo");
	m_ctrlFifo.init(ctrlId, FIFO_SIZE, "control_Fifo");
	m_dataInId = dataInId;
	m_dataOutStFifo.init(dataOutStId, FIFO_SIZE, "dataOutSt_Fifo");
	m_dataOutGwFifo.init(dataOutGwId, FIFO_SIZE, "dataOutGw_Fifo");

	return 0;
}

long SatSpot::getSpotId()
{
	return this->m_spotId;
}

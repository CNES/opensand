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
 * @file PhysicStd.cpp
 * @brief Generic Physical Transmission Standard
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "opensand_conf/uti_debug.h"

#include "PhysicStd.h"


PhysicStd::PhysicStd(string type,
                     const EncapPlugin::EncapPacketHandler *const pkt_hdl,
                     const FmtSimulation *const fmt_simu,
                     time_ms_t frame_duration_ms,
                     freq_khz_t bandwidth_khz):
	type(type),
	packet_handler(pkt_hdl),
	fmt_simu(fmt_simu),
	frame_duration_ms(frame_duration_ms),
	bandwidth_khz(0),
	remaining_credit_ms(0)
{
}


PhysicStd::~PhysicStd()
{
}


string PhysicStd::getType()
{
	return this->type;
}


int PhysicStd::onRcvEncapPacket(NetPacket *packet,
                                DvbFifo *fifo,
                                long current_time,
                                int fifo_delay)
{

	MacFifoElement *elem;

	// create a new satellite cell to store the packet
	elem = new MacFifoElement(packet, current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		UTI_ERROR("cannot allocate FIFO element, drop packet\n");
		goto error;
	}

	// append the new satellite cell in the ST FIFO of the appropriate
	// satellite spot
	if(!fifo->push(elem))
	{
		UTI_ERROR("FIFO is full: drop packet\n");
		goto release_elem;
	}

	UTI_DEBUG("encapsulation packet %s stored in FIFO "
	          "(tick_in = %ld, tick_out = %ld)\n",
	          packet->getName().c_str(),
	          elem->getTickIn(), elem->getTickOut());

	return 0;

release_elem:
	delete elem;
error:
	delete packet;
	return -1;
}

bool PhysicStd::onForwardFrame(DvbFifo *data_fifo,
                              unsigned char *frame,
                              unsigned int length,
                              long current_time,
                              int fifo_delay)
{
	MacFifoElement *elem;

	// sanity check
	if(frame == NULL || length <= 0)
	{
		UTI_ERROR("invalid DVB burst to forward to carrier ID %d\n",
		          data_fifo->getCarrierId());
		goto error;
	}

	// get a room with timestamp in fifo
	elem = new MacFifoElement(frame, length,
	                          current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		UTI_ERROR("cannot allocate FIFO element, drop packet\n");
		goto error;
	}

	// fill the delayed queue
	if(!data_fifo->push(elem))
	{
		UTI_ERROR("fifo full, drop the DVB frame\n");
		goto release_elem;
	}

	UTI_DEBUG("DVB/BB frame stored in FIFO for carrier ID %d "
	          "(tick_in = %ld, tick_out = %ld)\n", data_fifo->getCarrierId(),
	          elem->getTickIn(), elem->getTickOut());

	return true;

release_elem:
	delete elem;
error:
	free(frame);
	return false;
}

int PhysicStd::getRealModcod()
{
	return this->realModcod;
}

int PhysicStd::getReceivedModcod()
{
	return this->receivedModcod;
}


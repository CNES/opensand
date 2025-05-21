/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file RtChannelMux.cpp
 * @author Yohan SIMARD / <yohan.simard@viveris.fr>
 * @brief  A channel with N input fifos and 1 output fifo
 */

#include "RtChannelMux.h"
#include "RtFifo.h"


namespace Rt
{


ChannelMux::ChannelMux(const std::string &name, const std::string &type):
	ChannelBase{name, type},
	previous_fifos{},
	next_fifo{nullptr}
{
}


bool ChannelMux::initPreviousFifo()
{
	bool success = true;
	for (auto&& fifo : this->previous_fifos)
		success = success && this->initSingleFifo(fifo);

	return success;
}


bool ChannelMux::enqueueMessage(Ptr<void> data, uint8_t type)
{
	Message m{std::move(data)};
	m.type = type;
	return this->pushMessage(this->next_fifo, std::move(m));
}


void ChannelMux::addPreviousFifo(std::shared_ptr<Fifo> &fifo)
{
	this->previous_fifos.push_back(fifo);
}


void ChannelMux::setNextFifo(std::shared_ptr<Fifo> &fifo)
{
	this->next_fifo = fifo;
}


};

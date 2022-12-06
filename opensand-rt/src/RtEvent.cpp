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
 * @file RtEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The generic event
 */

#include <unistd.h>

#include "RtEvent.h"


namespace Rt
{


Event::Event(EventType type, const std::string &name, int32_t fd, uint8_t priority):
	type{type},
	name{name},
	fd{fd},
	priority{priority}
{
	this->setTriggerTime();
	this->setCustomTime();
}


Event::~Event()
{
	close(this->fd);
}

void Event::setTriggerTime(void)
{
	this->trigger_time = std::chrono::high_resolution_clock::now();
}

void Event::setCustomTime(void) const
{
	this->custom_time = std::chrono::high_resolution_clock::now();
}

time_val_t Event::getTimeFromTrigger(void) const
{
	auto time = std::chrono::high_resolution_clock::now();
	auto duration = time - this->trigger_time;
	return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

time_val_t Event::getTimeFromCustom(void) const
{
	auto time = std::chrono::high_resolution_clock::now();
	auto duration = time - this->custom_time;
	return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

time_val_t Event::getAndSetCustomTime(void) const
{
	auto res = this->getTimeFromCustom();
	this->setCustomTime();
	return res;
}


bool Event::operator <(const Event& event) const
{
	long int delta = 100000000L * (this->priority - event.priority);
	delta += std::chrono::duration_cast<std::chrono::microseconds>(this->trigger_time - event.trigger_time).count();
	return delta < 0;		
}


bool Event::operator ==(const event_id_t id) const
{
  return this->fd == id;
}


bool Event::operator !=(const event_id_t id) const
{
	return this->fd != id;
}


};

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 *
 */


#include "RtEvent.h"

#include <opensand_output/Output.h>

#include <cstdlib>
#include <unistd.h>

RtEvent::RtEvent(event_type_t type, const string &name, int32_t fd, uint8_t priority):
	type(type),
	name(name),
	fd(fd),
	priority(priority)
{
/*	DFLTLOG(LEVEL_DEBUG,
	        "Create new event \"%s\" of type %d\n",
	        name.c_str(), type);*/
	this->setTriggerTime();
	this->setCustomTime();
}

RtEvent::~RtEvent()
{
	close(this->fd);
}

void RtEvent::setTriggerTime(void)
{
	gettimeofday(&this->trigger_time, NULL);
}

void RtEvent::setCustomTime(void) const
{
	gettimeofday(&this->custom_time, NULL);
}

timeval RtEvent::getTimeFromTrigger(void) const
{
	timeval res;
	timeval current;
	gettimeofday(&current, NULL);

	timersub(&current, &this->trigger_time, &res);
	return res;
}

timeval RtEvent::getTimeFromCustom(void) const
{
	timeval res;
	timeval current;
	gettimeofday(&current, NULL);

	timersub(&current, &this->custom_time, &res);
	return res;
}

timeval RtEvent::getAndSetCustomTime(void) const
{
	timeval res = this->getTimeFromCustom();
	this->setCustomTime();
	return res;
}

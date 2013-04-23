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
 * @file Event.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The generic event
 *
 */

#include <cstdlib>

#include "Event.h"

Event::Event(event_type_t type, const string &name, int32_t fd, uint8_t priority):
	type(type),
	name(name),
	fd(fd),
	priority(priority)
{
	this->setCreationTime();
}

Event::~Event()
{
	close(this->fd);
}

void Event::setCreationTime(void)
{
	gettimeofday(&this->creation_time, NULL);
}

timeval Event::getElapsedTime() const
{
	timeval res;
	timeval current;
	gettimeofday(&current, NULL);

	res.tv_sec = abs(current.tv_sec - this->creation_time.tv_sec) ;
	res.tv_usec = abs(current.tv_usec - this->creation_time.tv_usec);
	return res;
}


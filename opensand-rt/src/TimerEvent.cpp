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
 * @file TimerEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The timer event
 *
 */

#include "TimerEvent.h"

#include <opensand_conf/uti_debug.h>

#include <sys/timerfd.h>
#include <unistd.h>

TimerEvent::TimerEvent(const string &name,
                       uint32_t timer_duration_ms,
                       bool auto_rearm,
                       bool start,
                       uint8_t priority):
	RtEvent(evt_timer, name, -1, priority),
	duration_ms(timer_duration_ms),
	enabled(start),
	auto_rearm(auto_rearm)
{
	this->fd = timerfd_create(CLOCK_MONOTONIC, 0);

	if(this->enabled)
	{
		UTI_DEBUG("Timer \"%s\" enabled, start it for the first time "
		          "(duration = %u ms)\n",
		          name.c_str(), timer_duration_ms);
		this->start();
	}
}

TimerEvent::~TimerEvent(void)
{
}

void TimerEvent::start(void)
{
	itimerspec timer_value;
	this->enabled = true;

	// non periodic, restart manually to avoid more than one timer expiration
	timer_value.it_interval.tv_nsec = 0;
	timer_value.it_interval.tv_sec = 0;

	//set value
	if(this->duration_ms < 1000)
	{
		timer_value.it_value.tv_nsec = this->duration_ms * 1000000;
		timer_value.it_value.tv_sec = 0;
	}
	else
	{
		timer_value.it_value.tv_nsec = (this->duration_ms % 1000) * 1000000 ;
		timer_value.it_value.tv_sec = this->duration_ms / 1000;
	}
	//start timer
	timerfd_settime(this->fd, 0, &timer_value, NULL);
}


void TimerEvent::disable(void)
{
	itimerspec timer_value;
	this->enabled = false;

	//non periodic
	timer_value.it_interval.tv_nsec = 0;
	timer_value.it_interval.tv_sec = 0;
	timer_value.it_value.tv_nsec = 0;
	timer_value.it_value.tv_sec = 0;

	//stop timer
	timerfd_settime(this->fd, 0, &timer_value, NULL);
}

bool TimerEvent::handle(void)
{
	// auto rearm ? if so rearm
	if(this->auto_rearm)
	{
		this->start();
	}
	else
	{
		//no auto rearm: disable
		this->disable();
	}
	return true;
}

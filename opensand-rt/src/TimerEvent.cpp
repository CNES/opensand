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
/* $Id: TimerEvent.cpp,v 1.1.1.1 2013/04/04 11:26:39 cgaillardet Exp $ */


#include "TimerEvent.h"

#include <sys/timerfd.h>
#include <unistd.h>

TimerEvent::TimerEvent(uint32_t timer_duration_ms,
                       uint8_t new_priority,
                       bool auto_rearm,
                       bool start):
	duration_ms(timer_duration_ms),
	auto_rearm(auto_rearm),
	enabled(start)
{
	this->fd = timerfd_create(CLOCK_MONOTONIC,0);

	if  (this->enabled ==true)
	{
		this->Start();
	}

	this->last_time_out.tv_sec = 0;
	this->last_time_out.tv_usec = 0;
	this->priority = new_priority;
}


TimerEvent::~TimerEvent(void)
{
	close(this->fd);
}

void TimerEvent::Start(void)
{
	itimerspec timer_value;
	this->enabled = true;

	//non periodic
	timer_value.it_interval.tv_nsec = 0;
	timer_value.it_interval.tv_sec = 0;

	//set value
	if(this->duration_ms < 1000)
	{
		timer_value.it_value.tv_nsec = this->duration_ms *1000000;
		timer_value.it_value.tv_sec = 0;
	}
	else
	{
		timer_value.it_value.tv_nsec = (this->duration_ms % 1000)*1000000 ;
		timer_value.it_value.tv_sec = this->duration_ms / 1000;
	}
	//start timer
	timerfd_settime(this->fd,0,&timer_value,NULL);
}


void TimerEvent::Disable(void)
{
	itimerspec timer_value;
	this->enabled = false;

	//non periodic
	timer_value.it_interval.tv_nsec = 0;
	timer_value.it_interval.tv_sec = 0;
	timer_value.it_value.tv_nsec = 0;
	timer_value.it_value.tv_sec = 0;

	//stop timer
	timerfd_settime(this->fd,0,&timer_value,NULL);
}


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
/* $Id: TimerEvent.h,v 1.1.1.1 2013/03/28 14:52:35 cgaillardet Exp $ */



#ifndef TIMEREVENT_H
#define TIMEREVENT_H

#include "Types.h"
#include "Event.h"

#include <sys/time.h>

class TimerEvent:public Event
{

  public:
	TimerEvent(uint32_t timer_duration_ms,
	           uint8_t new_priority= 4,
	           bool auto_rearm = false,
	           bool start= true);
	~TimerEvent(void);

	void Start(void);
	void Disable(void);
	bool IsAutoRearm(void) { return this->auto_rearm;};
	bool IsRunning(void) { return this->enabled;};
	uint32_t GetDuration(void) {return duration_ms;};


  protected:
	uint32_t duration_ms;
	bool auto_rearm;
	bool enabled;
	timeval last_time_out;


};

#endif

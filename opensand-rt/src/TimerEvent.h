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
#include <sys/time.h>
#include "Event.h"



/**
  * @class TimerEvent
  * @brief Class describing a Timer event. Inherits from Event
  *
  *
  */

class TimerEvent:public Event
{

public:

    /*
	 * Constructor
	 *
     * @param timer_duration_ms timer duration in milliseconds
     *
     * @param new_priority priority of the event, default 4
     *
     * @param auto_rearm tells if timer is automatically launched again when triggered
     *
     * @param start default state, when start the timer starts
	 */
    TimerEvent(uint32_t timer_duration_ms, uint8_t new_priority= 4, bool auto_rearm = false, bool start= true);
    ~TimerEvent(void);


    /*
	 * Start starts the timer
	 *
	 */
    void Start(void);

    /*
	 * Disable disable the timer
	 *
	 */
    void Disable(void);

   /*
	 * IsAutoRearm Getter for auto_rearm
	 *
	 * @return true if timer has to be rearmed automatically, false otherwise
	 */
    bool IsAutoRearm(void) { return this->auto_rearm;};

   /*
	 * IsRunning Getter for enabled
	 *
	 * @return true if timer is enabled (running), false otherwise
	 */
    bool IsRunning(void) { return this->enabled;};


     /*
	 * GetDuration Getter for duration_ms
	 *
	 * @return timer duration
	 */
    uint32_t GetDuration(void) {return duration_ms;};


protected:

    /// timer duration in milliseconds
    uint32_t duration_ms;

    /// boolean enabling this timer
    bool enabled;
    /// boolean telling if the timer is rearmed automatically or not
    bool auto_rearm;

    /// date of last time the timer was triggered
    timeval last_time_out;
private:


};

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file TimerEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The timer event
 *
 */


#ifndef TIMEREVENT_H
#define TIMEREVENT_H

#include "Types.h"
#include "RtEvent.h"

#include <sys/time.h>


/**
  * @class TimerEvent
  * @brief Event describing a timer
  *
  */
class TimerEvent: public RtEvent
{

  public:

	/**
	 * @brief TimerEvent constructor
	 *
	 * @param name               The event name
	 * @param timer_duration_ms  The timer duration (in ms)
	 * @param auto_rearm         Whether the timer is automatically launched again
	 *                           when triggered
	 * @param start              default state when created
	 * @param priority           The priority of the event
	 */
	TimerEvent(const string &name,
	           double timer_duration_ms,
	           bool auto_rearm = false,
	           bool start = true,
	           uint8_t priority = 2);
	~TimerEvent(void);


	/**
	 * @brief Start the timer
	 */
	void start(void);

	/**
	 * @brief Trigger a timer immediately
	 *        In fact, we set the minimum time and start it
	 *        because there is no way to trigger it manually
	 */
	void raise(void);

	/**
	 * @brief Disable the timer
	 *
	 */
	void disable(void);

	/*
	 * @brief Check if timer is enabled
	 *
	 * @return true if timer is enabled, false otherwise
	 */
	bool isEnabled(void) const {return this->enabled;};

	/**
	 * @brief Get the timer duration
	 *
	 * @return the timer duration (in ms)
	 */
	double getDuration(void) const {return this->duration_ms;};

	/**
	 * @brief Set the timer duration
	 *
	 * @param new_duration the timer duration (in ms)
	 */
	void setDuration(double new_duration)
	{
		this->duration_ms = new_duration;
	};

	virtual bool handle(void);

  protected:

	/// Timer duration in milliseconds
	double duration_ms;

	/// Whether the timer is enabled
	bool enabled;

	/// Whether the timer is rearmed automatically or not
	bool auto_rearm;


};

#endif

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
 * @file SignalEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The signal event
 *
 */


#ifndef SIGNALEVENT_H
#define SIGNALEVENT_H

#include <sys/time.h>
#include <sys/signalfd.h>

#include "Types.h"
#include "RtEvent.h"

/**
  * @class SignalEvent
  * @brief Event decribing a sighandlers on block
  */
class SignalEvent: public RtEvent
{

  public:
	/**
	 * @brief Event constructor
	 *
	 * @param name         The signal name
	 * @param signal_mask  Sigset_t containing signal(s) triggering this event
	 * @param priority     The priority of the event
	 */
	SignalEvent(const string &name, sigset_t signal_mask, uint8_t priority = 1);
	~SignalEvent(void);

	/*
	 * @brief Get triggered signal information
	 *
	 * @return the information on the triggered signal
	 */
	signalfd_siginfo getTriggerInfo(void) {return this->sig_info;};

	virtual bool handle(void);

  protected:

	/// The signal(s) to trigger this event
	sigset_t mask;
	/// The information that come when a signal triggers the event
	signalfd_siginfo sig_info;
};

#endif

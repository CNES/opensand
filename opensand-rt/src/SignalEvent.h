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
 * @file SignalEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The signal event
 *
 */


#ifndef SIGNALEVENT_H
#define SIGNALEVENT_H

#include <sys/signalfd.h>

#include "RtEvent.h"
#include "Types.h"


namespace Rt
{


/**
  * @class SignalEvent
  * @brief Event decribing a sighandlers on block
  */
class SignalEvent: public Event
{
 public:
	/**
	 * @brief Event constructor
	 *
	 * @param name         The signal name
	 * @param signal_mask  Sigset_t containing signal(s) triggering this event
	 * @param priority     The priority of the event
	 */
	SignalEvent(const std::string &name,
	            sigset_t signal_mask,
	            uint8_t priority = 1);

	/*
	 * @brief Get triggered signal information
	 *
	 * @return the information on the triggered signal
	 */
	inline signalfd_siginfo getTriggerInfo() {return this->sig_info;};

	/**
	 * @brief Handle a signal event
	 * @warning be careful, if you read signal here, it won't be accessible by
	 *          any other thread catching it
	 *          If only a thread catch this signal, it may read data on the
	 *          signalfd in order to empty it
	 *
	 * @return true on success, false otherwise
	 */
	bool handle() override;

	// This is the only case where it is critical as stop event is a signal
	bool isCritical() const override { return true; };

 protected:
	/**
	 * @brief Read a signal event
	 *        This function should be used in handle if you want to read
	 *        the signal information (see \ref handle for warning)
	 *
	 * @return true on success, false otherwise
	 */
	bool readHandler();

	/// The signal(s) to trigger this event
	sigset_t mask;
	/// The information that come when a signal triggers the event
	signalfd_siginfo sig_info;

 private:
	bool advertiseEvent(ChannelBase& channel) override;
};


};  // namespace Rt


#endif

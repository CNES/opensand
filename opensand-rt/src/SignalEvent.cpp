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
 * @file SignalEvent.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The signal event
 *
 */

#include "SignalEvent.h"
#include "Rt.h"

#include <signal.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>


SignalEvent::SignalEvent(const string &name, sigset_t signal_mask, uint8_t priority):
	Event(evt_signal, name, -1, priority),
	mask(signal_mask)
{
	int ret;
	this->fd = signalfd(-1, &(this->mask), 0);

	// block the signal(s) so only our handler gets it
	ret = pthread_sigmask(SIG_BLOCK, &this->mask, NULL);
	if(ret != 0)
	{
		Rt::reportError("signal constructor", pthread_self(),
		                true, "Cannot block signal", ret);
	}
}

SignalEvent::~SignalEvent(void)
{
}

bool SignalEvent::handle(void)
{
	int rlen;

	// signal structure size is constant
	rlen = read(this->fd, &this->sig_info, sizeof(struct signalfd_siginfo));
	if(rlen != sizeof(struct signalfd_siginfo))
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot read signal", ((rlen < 0) ? errno : 0));
		return false;
	}
	return true;
}

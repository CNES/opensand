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
/* $Id: SignalEvent.cpp,v 1.1.1.1 2013/04/08 9:10:31 cgaillardet Exp $ */


#include <cstring> //memcpy
#include <unistd.h> //close

#include "SignalEvent.h"



SignalEvent::SignalEvent(sigset_t signalMask, uint8_t new_priority)
{
	this->mask = signalMask;
    this->fd = signalfd(-1, &(this->mask),0);
    this->event_type = Signal;
    this->priority = new_priority;
}



void SignalEvent::SetData(unsigned char *data, int32_t size) //converts data into signalfd_siginfo
{
    memcpy(&(this->sig_info), data, size);
}

SignalEvent::~SignalEvent(void)
{
    close(this->fd);
}

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
/* $Id: SignalEvent.h,v 1.1.1.1 2013/04/08 10:20:15 cgaillardet Exp $ */



#ifndef SIGNALEVENT_H
#define SIGNALEVENT_H

#include <sys/time.h>
#include <sys/signalfd.h>

#include "Types.h"
#include "Event.h"

class SignalEvent:public Event
{

public:
    SignalEvent(sigset_t signal_mask, uint8_t new_priority=2);
    ~SignalEvent(void);

    void Enable(void){this->enabled = true;};
    void Disable(void){this->enabled = false;};
    bool IsActive(void) { return this->enabled;};

    void SetData(char *data, int32_t size);
    signalfd_siginfo GetTriggerInfo (void) { return this->sig_info;};

protected:
	sigset_t mask;
    signalfd_siginfo sig_info;
    bool enabled;
private:


};

#endif

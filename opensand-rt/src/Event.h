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
/* $Id: Event.h,v 1.1.1.1 2013/02/04 16:16:54 cgaillardet Exp $ */



#ifndef EVENT_H
#define EVENT_H


#include <sys/time.h>
#include "stdlib.h"

#include "Types.h"

using std::string;

class Event
{

public:
    Event(string new_name="event", uint8_t new_priority = 0);
    ~Event();

    EventType GetType(void) {return this->event_type;};
    timeval GetLifetime(void);
    uint8_t GetPriority(void) {return this->priority;};
	void setPriority(uint8_t new_priority){ this->priority = new_priority;};
	int32_t GetFd(void) {return this->fd;};
	void SetFd(int32_t new_fd) {this->fd = new_fd;};
	void SetCreationTime(void);
    void SetName(string new_name) {this->name = new_name;};
    string GetName (void) {return this->name;};

protected:
    string name;
    int32_t fd;
    timeval time_in;
    EventType event_type;
	uint8_t priority;
private:

};

#endif

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
/* $Id: Event.cpp,v 1.1.1.1 2013/04/04 11:00:02 cgaillardet Exp $ */

#include <cstdlib> // abs, null


#include "Event.h"

Event::Event(uint8_t new_priority)
{
    this->priority = new_priority;
    this->SetCreationTime();
}

void Event::SetCreationTime(void)
{
    gettimeofday(&this->time_in,NULL);
}

timeval Event::GetLifetime()
{
    timeval res;
    timeval current;
    gettimeofday(&current,NULL);

    res.tv_sec = abs(current.tv_sec - this->time_in.tv_sec) ;
    res.tv_usec = abs(current.tv_usec - this->time_in.tv_usec);
    return res;
}

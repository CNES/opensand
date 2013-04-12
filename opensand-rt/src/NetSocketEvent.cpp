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
/* $Id: NetSocketEvent.cpp,v 1.1.1.1 2013/04/04 16:34:27 cgaillardet Exp $ */


#include <cstring> //memcpy
#include <unistd.h> //close

#include "NetSocketEvent.h"


NetSocketEvent::NetSocketEvent(int32_t current_fd, uint8_t priority):
data(NULL),
size(0)
{
    this->input_fd = current_fd;
    this->priority = priority;
    this->event_type = NetSocket;
}

NetSocketEvent::~NetSocketEvent()
{
    // close FD ?
    // close (this->input_fd);

    // delete data
    if (this->data !=NULL)
    {
        delete [] this->data;
    }
}

void NetSocketEvent::SetData(unsigned char *data, uint16_t length)
{
    if (this->size > 0)
    {
        delete this->data;
    }

    if (length <= MAX_DATA_IN_NETSOCKET_EVENT)
    {
        this->data = (unsigned char *)malloc(length + 1);
        memcpy(this->data, data, length);
        this->data[length]=0;
        this->size= length;
    }
    else
    {
        this->size= 0;
    }

}



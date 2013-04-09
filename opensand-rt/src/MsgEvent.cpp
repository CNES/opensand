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
/* $Id: MsgEvent.cpp,v 1.1.1.1 2013/04/04 09:12:15 cgaillardet Exp $ */


#include <cstring> //memcpy
#include "MsgEvent.h"



MsgEvent::MsgEvent(int32_t fd, uint8_t new_priority, char *data, uint16_t size):
    size(size)
{
    this->fd= fd;
    this->priority = new_priority;
    if ( size > 0 )
    {
        this->data = (char *)malloc (size+1);
        memcpy(this->data, data, size);
        this->data[size] = 0;
    }
    else
    {
        this->data = NULL;
    }
    this->event_type= Message;
}


void MsgEvent::SetData(char *data, uint32_t length)
{
	if (this->size > 0)
	{
		delete this->data;
	}
	this->data = (char *)malloc(length + 1);
	memcpy(this->data, data, length);
	this->data[length]=0;
}


MsgEvent::~MsgEvent()
{
    delete[] this->data;
}


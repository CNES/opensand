/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file EventController.c
 * @author TAS
 * @brief The event controller
 */

#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include "EventController_e.h"
#include "EventControllerInterface_e.h"


static T_EVENT_BUFFER event_buffer;
static T_UINT16 event_read_index;
static T_UINT16 event_write_index;
static T_UINT16 event_full_counter;
static T_UINT8 event_buffer_readed_field[EVENT_BUFFER_NB_FIELDS];


void init_event(void)
{
	T_UINT8 index_field = 0;
	T_UINT16 index_buffer = 0;

	event_read_index = 0;
	event_write_index = 0;
	event_full_counter = 0;

	for(index_field = 0; index_field < EVENT_BUFFER_NB_FIELDS; index_field++)
	{
		event_buffer_readed_field[index_field] = 0;
	}

	for(index_buffer = 0; index_buffer < EVENT_BUFFER_SIZE; index_buffer++)
	{
		event_buffer[index_buffer].telemetry_event_category = 0;
		event_buffer[index_buffer].telemetry_event_date = 0;
		strcpy((T_STRING) event_buffer[index_buffer].telemetry_event_name, "");
		strcpy((T_STRING) event_buffer[index_buffer].telemetry_event_index_sign,
				 "");
		event_buffer[index_buffer].telemetry_event_index_value = 0;
		strcpy((T_STRING) event_buffer[index_buffer].telemetry_event_value_sign,
				 "");
		event_buffer[index_buffer].telemetry_event_value = 0;
		strcpy((T_STRING) event_buffer[index_buffer].telemetry_event_unit, "");
	}
}


void update_read_counter_event(T_UINT8 index)
{
	int index_field = 0;
	int total_read = 0;
	event_buffer_readed_field[index] = 1;

	for(index_field = 0; index_field < EVENT_BUFFER_NB_FIELDS; index_field++)
	{
		total_read = total_read + event_buffer_readed_field[index_field];
	}

	if(total_read == EVENT_BUFFER_NB_FIELDS)
	{
		event_read_index++;
		event_read_index = event_read_index % EVENT_BUFFER_SIZE;
		event_full_counter--;

		for(index_field = 0; index_field < EVENT_BUFFER_NB_FIELDS; index_field++)
		{
			event_buffer_readed_field[index_field] = 0;
		}
	}
}



void update_write_counter(void)
{
	event_write_index++;
	event_write_index = event_write_index % EVENT_BUFFER_SIZE;
	event_full_counter++;
}


int main(int argc, char *argv[])
{
	startEventControllerInterface(argc, argv);
	return (0);
}

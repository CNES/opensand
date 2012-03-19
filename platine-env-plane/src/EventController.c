/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe RENAUDY - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    :  
    @HISTORY :
*/
/*--------------------------------------------------------------------------*/

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

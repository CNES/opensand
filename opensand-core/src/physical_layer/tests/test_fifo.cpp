/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file test_fifo.cpp
 * @brief delay fifo test source
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 *
 *
 */


#include <opensand_rt/Rt.h>

#include "DelayFifo.h"
#include "FifoElement.h"
#include "OpenSandCore.h"
#include "NetContainer.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>


time_ms_t elem_times[5] = {time_ms_t(0), time_ms_t(10), time_ms_t(20), time_ms_t(30), time_ms_t(40)};


int main()
{
	int is_failure = 1;
	time_ms_t current_time;
	DelayFifo *fifo = new DelayFifo(1000);

	// Add elements to fifo
	current_time = getCurrentTime();

	for(unsigned int i=0; i < sizeof(elem_times); i++)
	{
		fifo->push(std::make_unique<FifoElement>(Rt::make_ptr<NetContainer>(nullptr),
		                                         current_time,
		                                         current_time + elem_times[i]));
	}

	// everything went fine, so report success
	is_failure = 0;

	while(fifo->getCurrentSize() > 0)
	{
		fifo->pop();
	}

	delete fifo;
	return is_failure;
}

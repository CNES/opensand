/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file CommandThread.cpp
 * @brief Background thread class receiving and parsing incoming messages.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#include "CommandThread.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <opensand_conf/uti_debug.h>

#include "Output.h"
#include "Messages.h"

CommandThread::CommandThread(int sock_fd)
{
	this->sock_fd = sock_fd;
}

bool CommandThread::start()
{
	pthread_t thread;
	if(pthread_create(&thread, NULL, CommandThread::_run, this) < 0)
	{
		UTI_ERROR("Unable to start the command listener thread : %s",
		          strerror(errno));
		return false;
	}

	return true;
}

void *CommandThread::_run(void *arg)
{
	CommandThread *self = (CommandThread*) arg;
	self->run();

	return NULL;
}

void CommandThread::run()
{
	char buffer[4096];

	for(;;)
	{
		uint8_t command_id = receiveMessage(this->sock_fd, buffer,
		                                    sizeof(buffer));

		switch(command_id)
		{
			case 0:
				return;

			case MSG_CMD_ENABLE_PROBE:
			case MSG_CMD_DISABLE_PROBE:
			{
				uint8_t probe_id = buffer[5];
				bool enabling = (command_id == MSG_CMD_ENABLE_PROBE);

				Output::setProbeState(probe_id, enabling);
			}
			break;

			case MSG_CMD_ENABLE:
				Output::enable();
				break;

			case MSG_CMD_DISABLE:
				Output::disable();
				break;

			default:
				UTI_ERROR("Received a message with unknown command ID %d\n", command_id);
		}
	}
}

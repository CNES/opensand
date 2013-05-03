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
 * @file OutputInternal.cpp
 * @brief Class used to hold internal output variables and methods.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#include "OutputInternal.h"

#include "CommandThread.h"
#include "Messages.h"

#include <opensand_conf/uti_debug.h>

#include <vector>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define TIMEOUT 6

static uint32_t getMilis()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

OutputInternal::OutputInternal()
{
	this->enabled = false;
	this->initializing = false;

	memset(&this->daemon_sock_addr, 0, sizeof(this->daemon_sock_addr));
	memset(&this->self_sock_addr, 0, sizeof(this->self_sock_addr));

	this->sock = 0;
}

OutputInternal::~OutputInternal()
{
	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		delete this->probes[i];
	}

	for(size_t i = 0 ; i < this->events.size() ; i++)
	{
		delete this->events[i];
	}

	if(this->sock != 0)
	{
		// Close the command socket
		// (will exit the command thread)
		shutdown(this->sock, SHUT_RDWR);
		close(this->sock);

		// Remove the socket file
		const char *path = this->self_sock_addr.sun_path;
		if(unlink(path) < 0)
		{
			UTI_ERROR("Unable to delete the socket \"%s\": %s",
			          path, strerror(errno));
		}
	}
}

void OutputInternal::init(bool enabled, event_level_t min_level,
                          const char *sock_prefix)
{
	char *path;

	UTI_PRINT(LOG_INFO, "Starting output initialization (%s)\n",
	          enabled ? "enabled" : "disabled");

	this->enabled = enabled;
	this->min_level = min_level;
	this->initializing = true;

	if(sock_prefix == NULL)
	{
		sock_prefix = "/var/run/sand-daemon";
	}

	this->daemon_sock_addr.sun_family = AF_UNIX;
	path = this->daemon_sock_addr.sun_path;
	snprintf(path, sizeof(this->daemon_sock_addr.sun_path),
	         "%s/" DAEMON_SOCK_NAME, sock_prefix);

	this->self_sock_addr.sun_family = AF_UNIX;
	path = this->self_sock_addr.sun_path;
	snprintf(path, sizeof(this->self_sock_addr.sun_path),
	         "%s/" SELF_SOCK_NAME, sock_prefix, getpid());

	UTI_DEBUG("Daemon socket address is \"%s\", own socket address is \"%s\".",
	          this->daemon_sock_addr.sun_path, this->self_sock_addr.sun_path);
}

Event *OutputInternal::registerEvent(const char *identifier, event_level_t level)
{
	if(!this->enabled)
	{
		return 0;
	}

	if(!this->initializing)
	{
		UTI_ERROR("cannot register event %s outside initialization, exit\n",
		          identifier);
	}
	assert(this->initializing);

	uint8_t new_id = this->events.size();
	Event *event = new Event(new_id, identifier, level);
	this->events.push_back(event);

	UTI_DEBUG("Registering event %s with level %d\n", identifier, level);

	return event;
}

bool OutputInternal::finishInit()
{

	if(!this->enabled)
	{
		return true;
	}

	if(!this->initializing)
	{
		UTI_ERROR("initialization already done\n");
	}

	UTI_PRINT(LOG_INFO, "Opening output communication socket\n");

	// Initialization of the UNIX socket

	char buffer[32];
	const char *path;
	std::string message;
	sockaddr_un address;
	CommandThread *command_thread;
	uint8_t command_id;

	int ret;
	struct timespec tv;
	fd_set readfds;
	sigset_t sigmask;

	this->sock = socket(AF_UNIX, SOCK_DGRAM, 0);

	if(this->sock == -1)
	{
		UTI_ERROR("Socket allocation failed: %s\n", strerror(errno));
		return false;
	}

	path = this->self_sock_addr.sun_path;
	unlink(path);

	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
	if(bind(this->sock, (const sockaddr*)&address, sizeof(address)) < 0)
	{
		UTI_ERROR("Socket binding failed: %s\n", strerror(errno));
		return false;
	}

	// Send the initial probe list
	msgHeaderRegister(message, getpid(), this->probes.size(),
	                  this->events.size());

	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		const char *name = this->probes[i]->getName();
		const char *unit = this->probes[i]->getUnit();

		message.append(1, (((int)this->probes[i]->isEnabled()) << 7) |
		                   this->probes[i]->storageTypeId());
		message.append(1, strlen(name));
		message.append(1, strlen(unit));
		message.append(name);
		message.append(unit);
	}

	for(size_t i = 0 ; i < this->events.size() ; i++)
	{
		const char *identifier = this->events[i]->identifier;

		message.append(1, this->events[i]->level);
		message.append(1, strlen(identifier));
		message.append(identifier);
	}

	if(sendto(this->sock, message.data(), message.size(), 0,
	          (const sockaddr*)&this->daemon_sock_addr,
	          sizeof(this->daemon_sock_addr)) < (signed)message.size())
	{
		UTI_ERROR("Sending initial probe list failed: %s\n", strerror(errno));
		return false;
	}

	// Wait for the ACK response

	// TODO try with select
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);


	FD_ZERO(&readfds);
	FD_SET(this->sock, &readfds);

	tv.tv_sec = TIMEOUT;
	tv.tv_nsec = 0;
	ret = pselect(this->sock + 1, &readfds, NULL, NULL, &tv, &sigmask);
	if(ret <= 0)
	{
		UTI_ERROR("cannot contact daemon or no answer in the "
		          "last %d seconds\n", TIMEOUT);
		this->disable();
		return false;
	}
	command_id = receiveMessage(this->sock, buffer, sizeof(buffer));
	if(command_id != MSG_CMD_ACK)
	{
		if(command_id == MSG_CMD_NACK)
		{
			UTI_INFO("receive NACK for initial probe list, disable output");
			this->disable();
		}
		else
		{
			UTI_ERROR("Incorrect ACK response for initial probe list\n");
		}
		return false;
	}

	// Start the command thread
	command_thread = new CommandThread(this->sock);
	if(!command_thread->start())
	{
		UTI_ERROR("Cannot start command thread\n");
		return false;
	}

	this->initializing = false;

	UTI_PRINT(LOG_INFO, "output initialized.\n");
	this->started_time = getMilis();

	return true;
}

void OutputInternal::sendProbes()
{
	if(!this->enabled)
	{
		return;
	}

	bool needs_sending = false;

	std::string message;
	uint32_t timestamp = getMilis() - this->started_time;
	msgHeaderSendProbes(message, timestamp);

	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		BaseProbe *probe = this->probes[i];
		if(probe->isEnabled() && probe->values_count != 0)
		{
			needs_sending = true;
			message.append(1, (uint8_t)i);
			probe->appendValueAndReset(message);
		}
	}

	if(!needs_sending)
	{
		return;
	}

	if(sendto(this->sock, message.data(), message.size(), 0,
	          (const sockaddr*)&this->daemon_sock_addr,
	           sizeof(this->daemon_sock_addr)) < (signed)message.size())
	{
		UTI_ERROR("Sending probe values failed: %s\n", strerror(errno));
	}
}

void OutputInternal::sendEvent(Event *event, const char *message_text)
{
	if(!this->enabled || event->level < this->min_level)
	{
		return;
	}

	// TODO use a list of message and if finishInit was not called
	//      stock events. Send stocked events when calling finishInit
	std::string message;
	msgHeaderSendEvent(message, event->id);
	message.append(message_text);

	if(sendto(this->sock, message.data(), message.size(), 0,
	          (const sockaddr*)&this->daemon_sock_addr,
	          sizeof(this->daemon_sock_addr)) < (signed)message.size())
	{
		UTI_ERROR("Sending event failed: %s\n", strerror(errno));
	}
}

void OutputInternal::setProbeState(uint8_t probe_id, bool enabled)
{
	UTI_DEBUG("%s probe %s\n", enabled ? "Enabling" : "Disabling",
	          this->probes[probe_id]->getName());
	this->probes[probe_id]->enabled = enabled;
}

void OutputInternal::disable()
{
	this->enabled = false;
}

void OutputInternal::enable()
{
	this->enabled = true;
}

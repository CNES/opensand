/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
 * @file Messages.cpp
 * @brief Functions to help message generation and handle message reception.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#include "Messages.h"

#include "Output.h"

#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/un.h>


static const uint32_t MAGIC_NUMBER = 0x5A7D0001;

#define PACKED __attribute__((packed))

/// the base head of messages
struct msg_base_header_t
{
	uint32_t magic;     ///< magic number
	uint8_t cmd_type;   ///< command type
} PACKED;

// FIXME: Add some padding after cmd_type to correctly align the following items?

/// register message header
struct msg_register_t
{
	uint32_t magic;      ///< magic number
	uint8_t cmd_type;    ///< command type
	uint32_t pid;        ///< process ID
	uint8_t num_probes;  ///< number of probes
	uint8_t num_logs;    ///< number of logs
} PACKED;

/// probe header
struct msg_send_probes_t
{
	uint32_t magic;      ///< magic number
	uint8_t cmd_type;    ///< command type
	uint32_t timestamp;  ///< the time elapsed since startup
} PACKED;

/// log header
struct msg_send_log_t
{
	uint32_t magic;      ///< magic number
	uint8_t cmd_type;    ///< command type
	uint8_t log_id;      ///< log ID
	uint8_t level;       ///< the level
} PACKED;


void msgHeaderRegisterEnd(string &message, pid_t pid,
                          uint8_t num_probes, uint8_t num_logs)
{
	return msgHeaderRegisterAll(message, pid, num_probes, num_logs,
	                            MSG_CMD_REGISTER_END);
}

void msgHeaderRegister(string &message, pid_t pid,
                       uint8_t num_probes, uint8_t num_logs)
{
	return msgHeaderRegisterAll(message, pid, num_probes, num_logs,
	                            MSG_CMD_REGISTER_INIT);
}

void msgHeaderRegisterLive(string &message, pid_t pid,
                           uint8_t num_probes, uint8_t num_logs)
{
	return msgHeaderRegisterAll(message, pid, num_probes, num_logs,
	                            MSG_CMD_REGISTER_LIVE);
}

void msgHeaderRegisterAll(string &message, pid_t pid,
                          uint8_t num_probes, uint8_t num_logs,
                          uint8_t command)
{
	msg_register_t header;
	header.magic = htonl(MAGIC_NUMBER);
	header.cmd_type = command;
	header.pid = htonl(pid);
	header.num_probes = num_probes;
	header.num_logs = num_logs;

	message.append((const char *)&header, sizeof(header));
}

void msgHeaderSendProbes(string &message, uint32_t timestamp)
{
	msg_send_probes_t header;
	header.magic = htonl(MAGIC_NUMBER);
	header.cmd_type = MSG_CMD_SEND_PROBES;
	header.timestamp = htonl(timestamp);

	message.append((const char *)&header, sizeof(header));
}

void msgHeaderSendLog(string &message, uint8_t log_id, log_level_t level)
{
	msg_send_log_t header;
	header.magic = htonl(MAGIC_NUMBER);
	header.cmd_type = MSG_CMD_SEND_LOG;
	header.log_id = log_id;
	header.level = level;

	message.append((const char *)&header, sizeof(header));
}

uint8_t receiveMessage(int sock_fd, char *message_data, size_t max_length, const char *daemon_sun_path)
{
	sockaddr_un address;
	socklen_t address_len = sizeof(address);
	ssize_t got = recvfrom(sock_fd, message_data, max_length, 0,
	                       (struct sockaddr *)&address, &address_len);

	if(got == 0)
	{
		DFLTLOG(LEVEL_WARNING, "Socket closed");
		// The socket was probably closed
		return 0;
	}

	if(address.sun_family != AF_UNIX ||
	   strncmp(address.sun_path, daemon_sun_path,
	           sizeof(address.sun_path)) != 0)
	{
		DFLTLOG(LEVEL_WARNING,
		        "Got unexpected message from “%s”\n",
		        address.sun_path);
		return 0;
	}

	if(got < 0)
	{
		DFLTLOG(LEVEL_ERROR,
		        "Error during message reception: %s\n",
		        strerror(errno));
		return 0;
	}

	if(got < (signed)sizeof(msg_base_header_t))
	{
		DFLTLOG(LEVEL_ERROR, "Got too short message from daemon!\n");
		return 0;
	}

	if(got > (signed)max_length)
	{
		DFLTLOG(LEVEL_ERROR,
		        "Message length overflow (%zd > %zu), please increase "
		        "the message buffer size.", got, max_length);
		return 0;
	}

	const msg_base_header_t *head = (const msg_base_header_t *)message_data;
	if(head->magic != htonl(MAGIC_NUMBER))
	{
		DFLTLOG(LEVEL_ERROR,
		        "Got message with bad magic number %08x\n",
		        ntohl(head->magic));
		return 0;
	}

	return head->cmd_type;
}

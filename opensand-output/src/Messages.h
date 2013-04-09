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
 * @file messages.h
 * @brief Functions to help message generation and handle message reception.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <string>
#include <vector>

#include <stdint.h>

#define MSG_CMD_REGISTER 1
#define MSG_CMD_ACK 2
#define MSG_CMD_SEND_PROBES 3
#define MSG_CMD_SEND_EVENT 4
#define MSG_CMD_ENABLE_PROBE 5
#define MSG_CMD_DISABLE_PROBE 6
#define MSG_CMD_DISABLE 8
#define MSG_CMD_ENABLE 9
#define MSG_CMD_NACK 11

#define DAEMON_SOCK_NAME "sand-daemon.socket"
#define SELF_SOCK_NAME "program-%d.socket"

/**
 * @brief Register a message
 *
 * @param message    The message
 * @param pid        The process ID
 * @param num_probes The number of probes
 * @param num_events The number of events
 */
void msgHeaderRegister(std::string &message, pid_t pid, uint8_t num_probes,
                       uint8_t num_events);


/**
 * @brief send probes
 *
 * @param message    The message
 * @param timestamp  The time ellapsed since startup (ms)
 */
void msgHeaderSendProbes(std::string &message, uint32_t timestamp);

/**
 * @brief send event
 *
 * @param message   The message
 * @param event_id  The ID of the event
 */
void msgHeaderSendEvent(std::string &message, uint8_t event_id);

/**
 *  @brief receive a message
 *
 * @param sock_fd       The socket file descriptor
 * @param message_data  The data contained in the message
 * max_length           The maximum length of the message
 *
 * @return the command type on success, 0 on failure
 */
uint8_t receiveMessage(int sock_fd, char *message_data, size_t max_length);

#endif

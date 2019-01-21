/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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

#include "OutputLog.h"

#include <string>
#include <vector>
#include <stdint.h>

using std::string;

#define MSG_CMD_REGISTER_INIT 1
#define MSG_CMD_REGISTER_END 2
#define MSG_CMD_REGISTER_LIVE 3
#define MSG_CMD_ACK 5
#define MSG_CMD_NACK 6
#define MSG_CMD_DISABLE 8
#define MSG_CMD_ENABLE 9

#define MSG_CMD_SEND_PROBES 10
#define MSG_CMD_SEND_LOG 20

#define MSG_CMD_ENABLE_PROBE 11
#define MSG_CMD_DISABLE_PROBE 12

#define MSG_CMD_SET_LOG_LEVEL 22
#define MSG_CMD_ENABLE_LOGS 23
#define MSG_CMD_DISABLE_LOGS 24
#define MSG_CMD_ENABLE_SYSLOG 25
#define MSG_CMD_DISABLE_SYSLOG 26

#define DAEMON_SOCK_NAME "sand-daemon.socket"
#define SELF_SOCK_NAME "program-%d.socket"

/**
 * @brief Register a message in initialization
 *
 * @param message    The message
 * @param pid        The process ID
 * @param num_probes The number of probes
 * @param num_logs	 The number of logs
 */
void msgHeaderRegister(string &message, pid_t pid, uint8_t num_probes,
                       uint8_t num_logs);

/**
 * @brief Register a message while initialization is already done
 *
 * @param message    The message
 * @param pid        The process ID
 * @param num_probes The number of probes
 * @param num_logs	 The number of logs
 */
void msgHeaderRegisterLive(string &message, pid_t pid, uint8_t num_probes,
                           uint8_t num_logs);

/**
 * @brief Register a message and signal the end of initialization
 *
 * @param message    The message
 * @param pid        The process ID
 * @param num_probes The number of probes
 * @param num_logs	 The number of logs
 */
void msgHeaderRegisterEnd(string &message, pid_t pid, uint8_t num_probes,
                          uint8_t num_logs);

/**
 * @brief Common function to register a message
 *
 * @param message    The message
 * @param pid        The process ID
 * @param num_probes The number of probes
 * @param num_logs	 The number of logs
 * @param command    The register command
 */
void msgHeaderRegisterAll(string &message, pid_t pid, uint8_t num_probes,
                          uint8_t num_logs, uint8_t command);

/**
 * @brief send probes
 *
 * @param message    The message
 * @param timestamp  The time ellapsed since startup (ms)
 */
void msgHeaderSendProbes(string &message, uint32_t timestamp);

/**
 * @brief send log
 *
 * @param message  The message
 * @param log_id   The ID of the log
 * @param level    The log level
 */
void msgHeaderSendLog(string &message, uint8_t log_id, log_level_t level);

/**
 *  @brief receive a message
 *
 * @param sock_fd              The socket file descriptor
 * @param message_data         The data contained in the message
 * @param max_length           The maximum length of the message
 * @param daemon_sun_path      The daemon addr sun path
 *
 * @return the command type on success, 0 on failure
 */
uint8_t receiveMessage(int sock_fd, char *message_data, size_t max_length, const char *daemon_sun_path);

#endif

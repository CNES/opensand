/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file CommandThread.h
 * @brief Background thread class receiving and parsing incoming messages.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#ifndef _COMMAND_THREAD_H
#define _COMMAND_THREAD_H

#include "OutputLog.h"

/**
 * @class thread that while receive an parse incoming messages
 */
class CommandThread
{
 public:
	CommandThread(int sock_fd);

	/**
	 * @brief start the command thread
	 *
	 * @return true on success, false otherwise
	 */
	bool start();

 private:
	/**
	 * @brief run the command thread
	 *
	 * @param arg  The command thread to run.
	 */
	static void *_run(void *arg);

	~CommandThread();
	/**
	 * @brief run the thread loop
	 */
	void run();

	/// The socket for command thread
	int sock_fd;

	/// output log
	OutputLog *log;
};

#endif

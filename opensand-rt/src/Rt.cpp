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

/**
 * @file Rt.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  High level interface for opensand-rt
 *
 */

#include "Rt.h"

#include <stdarg.h>
#include <cstdio>
#include <algorithm>
#include <sstream>


// Create bloc instance
BlockManager Rt::manager;


bool Rt::init(void)
{
	return manager.init();
}


bool Rt::run(bool init)
{
	if(init && !manager.init())
	{
		return false;
	}
	if(!manager.start())
	{
		return false;
	}
	manager.wait();

	return manager.getStatus();
}


void Rt::stop()
{
	manager.stop();
}


void Rt::reportError(const std::string &name, std::thread::id thread_id,
                     bool critical, const char *msg_format, ...)
{
	char msg[512];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(msg, 512, msg_format, args);

	va_end(args);

  std::ostringstream ss;
  ss << "Error in " << name << " (thread: " << thread_id << "): " << msg << "\n";
  std::string message = ss.str();

	manager.reportError(message, critical);
}

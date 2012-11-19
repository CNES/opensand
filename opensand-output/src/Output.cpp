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
 * @file Output.cpp
 * @brief Methods of the Output static class, used by the application to
 *        interact with the output.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */

#include "Output.h"

#include <stdio.h>
#include <stdarg.h>


OutputInternal Output::instance;

void Output::init(bool enabled, event_level_t min_level, const char* sock_prefix)
{
	instance.init(enabled, min_level, sock_prefix);
}

Event* Output::registerEvent(const char* identifier, event_level_t level)
{
	return instance.registerEvent(identifier, level);
}

bool Output::finishInit()
{
	return instance.finishInit();
}

void Output::sendProbes()
{
	instance.sendProbes();
}

void Output::sendEvent(Event* event, const char* msg_format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	instance.sendEvent(event, buf);
}

Output::Output()
{
}

Output::~Output()
{
}

void Output::setProbeState(uint8_t probe_id, bool enabled)
{
	instance.setProbeState(probe_id, enabled);
}

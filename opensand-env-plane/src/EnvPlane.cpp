/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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
 * @file EnvPlane.cpp
 * @brief Methods of the EnvPlane static class, used by the application to interact with the environment plane.
 */

#include "EnvPlane.h"

#include <stdio.h>
#include <stdarg.h>


EnvPlaneInternal EnvPlane::instance;

void EnvPlane::init(bool enabled, event_level min_level, const char* sock_prefix)
{
	instance.init(enabled, min_level, sock_prefix);
}

Event* EnvPlane::register_event(const char* identifier, event_level level)
{
	return instance.register_event(identifier, level);
}

bool EnvPlane::finish_init()
{
	return instance.finish_init();
}

void EnvPlane::send_probes()
{
	instance.send_probes();
}

void EnvPlane::send_event(Event* event, const char* msg_format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	instance.send_event(event, buf);
}

EnvPlane::EnvPlane()
{
}

EnvPlane::~EnvPlane()
{
}

void EnvPlane::set_probe_state(uint8_t probe_id, bool enabled)
{
	instance.set_probe_state(probe_id, enabled);
}

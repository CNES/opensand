#ifndef _ENV_PLANE_H
#define _ENV_PLANE_H

#include <assert.h>
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
 * @file EnvPlane.h
 * @brief Definition of the EnvPlane static class, used by the application to interact with the environment plane.
 */


#include <vector>

#include "Probe.h"
#include "Event.h"
#include "EnvPlaneInternal.h"

#define PRINTFLIKE(fmt_pos, vararg_pos) __attribute__((format(printf,fmt_pos,vararg_pos)))

class EnvPlane
{
	friend class CommandThread;

public:
	static void init(bool enabled, event_level min_level, const char* sock_prefix = NULL);

	template<typename T>
	static Probe<T>* register_probe(const char* name, bool enabled, sample_type type);
	template<typename T>
	static Probe<T>* register_probe(const char* name, const char* unit, bool enabled, sample_type type);

	static Event* register_event(const char* identifier, event_level level);

	static bool finish_init(void);
	static void send_probes(void);
	static void send_event(Event* event, const char* msg_format, ...) PRINTFLIKE(2, 3);

	inline static const sockaddr_un* daemon_sock_addr() { return &instance.daemon_sock_addr; };
	inline static const sockaddr_un* self_sock_addr() { return &instance.self_sock_addr; };

private:
	EnvPlane();
	~EnvPlane();

	static void set_probe_state(uint8_t probe_id, bool enabled);

	static EnvPlaneInternal instance;
};

template<typename T>
Probe<T>* EnvPlane::register_probe(const char* name, bool enabled, sample_type type)
{
	return EnvPlane::register_probe<T>(name, "", enabled, type);
}

template<typename T>
Probe<T>* EnvPlane::register_probe(const char* name, const char* unit, bool enabled, sample_type type)
{
	return EnvPlane::instance.register_probe<T>(name, unit, enabled, type);
}

#endif

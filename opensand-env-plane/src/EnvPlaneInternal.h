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
 * @file EnvPlaneInternal.h
 * @brief Class used to hold internal environment plane variables and methods.
 */


#ifndef _ENV_PLANE_INTERNAL_H
#define _ENV_PLANE_INTERNAL_H

#include <assert.h>
#include <sys/un.h>

#include <vector>

#include "Probe.h"
#include "Event.h"

class EnvPlaneInternal
{
	friend class EnvPlane;
	friend class BaseProbe;

private:
	EnvPlaneInternal();
	~EnvPlaneInternal();

	void init(bool enabled, event_level min_level, const char* sock_prefix);
	template<typename T>
	Probe<T>* register_probe(const char* name, const char* unit, bool enabled, sample_type type);
	Event* register_event(const char* identifier, event_level level);
	bool finish_init(void);
	void send_probes(void);
	void send_event(Event* event, const char* message);
	void set_probe_state(uint8_t probe_id, bool enabled);

	bool enabled;
	bool initializing;
	event_level min_level;
	std::vector<BaseProbe*> probes;
	std::vector<Event*> events;
	int sock;
	uint32_t started_time;

	sockaddr_un daemon_sock_addr;
	sockaddr_un self_sock_addr;
};

template<typename T>
Probe<T>* EnvPlaneInternal::register_probe(const char* name, const char* unit, bool enabled, sample_type type) {
	assert(this->initializing);

	// FIXME: uti_debug.h cannot be included in this file because it does not properly support
	// double inclusion (because of the DBG_PACKAGE constant which may get redefined).
	//UTI_DEBUG("Registering probe %s with type %d\n", name, type);

	uint8_t new_id = this->probes.size();
	Probe<T>* probe = new Probe<T>(new_id, name, unit, enabled, type);
	this->probes.push_back(probe);

	return probe;
}

#endif

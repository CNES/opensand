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

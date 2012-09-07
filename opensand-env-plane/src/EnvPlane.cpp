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

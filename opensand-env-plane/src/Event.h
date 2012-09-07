#ifndef _EVENT_H
#define _EVENT_H

#include <stdint.h>

enum event_level {
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_WARNING,
	LEVEL_ERROR
};

class Event
{
	friend class EnvPlaneInternal;

	inline const char* identifier() const { return this->_identifier; };
	inline event_level level() const { return this->_level; };

protected:
	Event(uint8_t id, const char* identifier, event_level level);
	virtual ~Event();

private:
	uint8_t id;
	char* _identifier;
	event_level _level;
};

#endif

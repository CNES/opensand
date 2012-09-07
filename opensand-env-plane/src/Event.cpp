#include "Event.h"

#include <stdlib.h>
#include <string.h>

Event::Event(uint8_t id, const char* identifier, event_level level) {
	this->id = id;
	this->_identifier = strdup(identifier);
	this->_level = level;
}

Event::~Event() {
	free(this->_identifier);
}

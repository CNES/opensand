#include "BaseProbe.h"

#include <stdlib.h>
#include <string.h>

BaseProbe::BaseProbe(uint8_t id, const char* name, bool enabled, sample_type type)
{
	this->id = id;
	this->_name = strdup(name);
	this->enabled = enabled;
	this->s_type = type;
	this->values_count = 0;
}

BaseProbe::~BaseProbe()
{
	free(this->_name);
}

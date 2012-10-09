#ifndef _PROBE_H
#define _PROBE_H

#include <algorithm>
#include <string>
#include <iostream>

#include <stdint.h>

#include "BaseProbe.h"

template<typename T>
class Probe : public BaseProbe
{
	friend class EnvPlaneInternal;

public:
	void put(T value);

private:
	Probe(uint8_t id, const char* name, const char* unit, bool enabled, sample_type type);
	
	virtual uint8_t storage_type_id();
	virtual void append_value_and_reset(std::string& str);

	T accumulator;
};

template<typename T>
Probe<T>::Probe(uint8_t id, const char* name, const char* unit, bool enabled, sample_type type)
	: BaseProbe(id, name, unit, enabled, type)
{
}

template<typename T>
void Probe<T>::put(T value)
{
	if (this->values_count == 0) {
		this->accumulator = value;
	
	} else {
		switch (this->s_type) {
			case SAMPLE_LAST:
				this->accumulator = value;
			break;
			
			case SAMPLE_MIN:
				this->accumulator = std::min(this->accumulator, value);
			break;
			
			case SAMPLE_MAX:
				this->accumulator = std::max(this->accumulator, value);
			break;
			
			case SAMPLE_AVG:
			case SAMPLE_SUM:
				this->accumulator += value;
			break;
		}
	}
	
	this->values_count++;
}

#endif

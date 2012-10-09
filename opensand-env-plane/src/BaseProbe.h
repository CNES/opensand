#ifndef _BASE_PROBE_H
#define _BASE_PROBE_H

#include <stdint.h>

#include <string>

class EnvPlaneInternal;

enum sample_type
{
	SAMPLE_LAST,
	SAMPLE_MIN,
	SAMPLE_MAX,
	SAMPLE_AVG,
	SAMPLE_SUM
};

class BaseProbe
{
	friend class EnvPlaneInternal;

public:
	inline bool is_enabled() const { return this->enabled; };
	inline const char* name() const { return this->_name; };
	inline const char* unit() const { return this->_unit; };

protected:
	BaseProbe(uint8_t id, const char* name, const char* unit, bool enabled, sample_type type);
	virtual ~BaseProbe();
	
	virtual uint8_t storage_type_id() = 0;
	virtual void append_value_and_reset(std::string& str) = 0;

	uint8_t id;
	char* _name;
	char* _unit;
	sample_type s_type;
	bool enabled;
	
	uint16_t values_count;
};

#endif

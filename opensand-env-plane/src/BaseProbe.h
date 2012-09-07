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

protected:
	BaseProbe(uint8_t id, const char* name, bool enabled, sample_type type);
	virtual ~BaseProbe();
	
	virtual uint8_t storage_type_id() = 0;
	virtual void append_value_and_reset(std::string& str) = 0;

	uint8_t id;
	char* _name;
	sample_type s_type;
	bool enabled;
	
	uint16_t values_count;
};

#endif

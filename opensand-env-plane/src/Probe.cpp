#include <Probe.h>

#include <arpa/inet.h>
#include <endian.h>
#include <string.h>

template<>
uint8_t Probe<int32_t>::storage_type_id()
{
	return 0;
}

template<>
uint8_t Probe<float>::storage_type_id()
{
	return 1;
}

template<>
uint8_t Probe<double>::storage_type_id()
{
	return 2;
}

template<>
void Probe<int32_t>::append_value_and_reset(std::string& str)
{
	int32_t accumulator = this->accumulator;
	if (this->s_type == SAMPLE_AVG)
		accumulator /= this->values_count;
	
	uint32_t value = accumulator;
	value = htonl(value);
	str.append((char*)&value, sizeof(value));
	
	this->values_count = 0;
}

template<>
void Probe<float>::append_value_and_reset(std::string& str)
{
	float accumulator = this->accumulator;
	if (this->s_type == SAMPLE_AVG)
		accumulator /= this->values_count;
	
	uint32_t value;
	memcpy(&value, &accumulator, sizeof(value));
	value = htonl(value);
	str.append((char*)&value, sizeof(value));
	
	this->values_count = 0;
}

template<>
void Probe<double>::append_value_and_reset(std::string& str)
{
	double accumulator = this->accumulator;
	if (this->s_type == SAMPLE_AVG)
		accumulator /= this->values_count;
	
	uint64_t value;
	memcpy(&value, &accumulator, sizeof(value));
	value = htobe64(value);
	str.append((char*)&value, sizeof(value));
	
	this->values_count = 0;
}

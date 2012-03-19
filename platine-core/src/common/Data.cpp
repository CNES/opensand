/**
 * @file Data.cpp
 * @brief A set of data for network packets
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "Data.h"

Data::Data(): std::basic_string<unsigned char>()
{
}

Data::Data(std::basic_string<unsigned char> string):
	std::basic_string<unsigned char>(string)
{
}

Data::Data(unsigned char *data, unsigned int len):
	std::basic_string<unsigned char>(data, len)
{
}

Data::Data(Data data, unsigned int pos, unsigned int len):
	std::basic_string<unsigned char>(data, pos, len)
{
}


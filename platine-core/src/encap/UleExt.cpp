/**
 * @file UleExt.cpp
 * @brief ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "UleExt.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


UleExt::UleExt(): _payload()
{
}

UleExt::~UleExt()
{
}

uint8_t UleExt::type()
{
	return this->_type;
}

bool UleExt::isMandatory()
{
	return this->is_mandatory;
}

Data UleExt::payload()
{
	return this->_payload;
}

uint16_t UleExt::payloadType()
{
	return this->_payloadType;
}


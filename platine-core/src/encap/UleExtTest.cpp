/**
 * @file UleExtTest.cpp
 * @brief Mandatory Test SNDU ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "UleExtTest.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


UleExtTest::UleExtTest(): UleExt()
{
	this->is_mandatory = true;
	this->_type = 0x00;
}

UleExtTest::~UleExtTest()
{
}

ule_ext_status UleExtTest::build(uint16_t ptype, Data payload)
{
	// payload does not change
	this->_payload = payload;

	// type is Test SNDU extension
	//  - 5-bit zero prefix
	//  - 3-bit H-LEN field (= 0 because extension is mandatory)
	//  - 8-bit H-Type field (= 0x00 type of Test SNDU extension)
	this->_payloadType = this->type();

	return ULE_EXT_OK;
}

ule_ext_status UleExtTest::decode(uint8_t hlen, Data payload)
{
	const char FUNCNAME[] = "[UleExtTest::decode]";

	// extension is mandatory, hlen must be 0
	if(hlen != 0)
	{
		UTI_ERROR("%s mandatory extension, but hlen (0x%x) != 0\n",
		          FUNCNAME, hlen);
		goto error;
	}

	// always discard the SNDU according to section 5.1 in RFC 4326
	return ULE_EXT_DISCARD;

error:
	return ULE_EXT_ERROR;
}


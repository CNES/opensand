/**
 * @file UleExtPadding.cpp
 * @brief Optional Padding ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "UleExtPadding.h"

#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"


UleExtPadding::UleExtPadding(): UleExt()
{
	this->is_mandatory = false;
	this->_type = 0x00;
}

UleExtPadding::~UleExtPadding()
{
}

ule_ext_status UleExtPadding::build(uint16_t ptype, Data payload)
{
	// TODO: the length of the extension is currently arbitrary choosen,
	// this should be modified. Length is 5 x 16-bit fields

	// add 4 x 16-bit padding field + 1 16-bit Next-Header field
	this->_payload.clear();
	this->_payload.append(4 * 2, 0x00);
	this->_payload.append(1, (ptype >> 8) & 0xff);
	this->_payload.append(1, ptype & 0xff);

	// add the next header/payload
	this->_payload += payload;

	// build the Next Header field for this extension
	//  - 5-bit zero prefix
	//  - 3-bit H-LEN field (= 5 is choosen for the moment)
	//  - 8-bit H-Type field (= 0x00 type of Padding extension)
	this->_payloadType = ((5 & 0x07) << 8) | (this->type() & 0xff);

	return ULE_EXT_OK;
}

ule_ext_status UleExtPadding::decode(uint8_t hlen, Data payload)
{
	const char FUNCNAME[] = "[UleExtPadding::decode]";

	// extension is optional, hlen must be 1-5
	if(hlen < 1 || hlen > 5)
	{
		UTI_ERROR("%s optional extension, but hlen (0x%x) != 1-5\n", FUNCNAME,
		          hlen);
		goto error;
	}

	// check if payload is large enough
	if(payload.length() < (size_t) hlen * 2)
	{
		UTI_ERROR("%s too few data (%u bytes) for %d-byte extension\n",
		          FUNCNAME, payload.length(), hlen * 2);
		goto error;
	}

	this->_payloadType = (payload.at(hlen * 2 - 2) << 8) |
	                     payload.at(hlen * 2 - 1);
	this->_payload = payload.substr(hlen * 2);

	return ULE_EXT_OK;

error:
	return ULE_EXT_ERROR;
}


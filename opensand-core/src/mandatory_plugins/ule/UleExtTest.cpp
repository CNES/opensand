/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file UleExtTest.cpp
 * @brief Mandatory Test SNDU ULE extension
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "UleExtTest.h"
#include "UlePacket.h"

#include <opensand_output/Output.h>

/** unused macro to avoid compilation warning with unused parameters.
 */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

UleExtTest::UleExtTest(): UleExt()
{
	this->is_mandatory = true;
	this->_type = 0x00;
}

UleExtTest::~UleExtTest()
{
}

ule_ext_status UleExtTest::build(uint16_t UNUSED(ptype), Data payload)
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

ule_ext_status UleExtTest::decode(uint8_t hlen, Data UNUSED(payload))
{
	// extension is mandatory, hlen must be 0
	if(hlen != 0)
	{
		LOG(UlePacket::ule_log, LEVEL_ERROR,
		    "mandatory extension, but hlen (0x%x) != 0\n",
		    hlen);
		goto error;
	}

	// always discard the SNDU according to section 5.1 in RFC 4326
	return ULE_EXT_DISCARD;

error:
	return ULE_EXT_ERROR;
}

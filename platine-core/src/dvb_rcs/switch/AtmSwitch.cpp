/**
 * @file AtmSwitch.cpp
 * @brief ATM switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "AtmSwitch.h"


AtmSwitch::AtmSwitch()
{
}

AtmSwitch::~AtmSwitch()
{
}

long AtmSwitch::find(NetPacket *packet)
{
	AtmCell *atm_cell;
	std::map < long, long >::iterator it;
	long spot_id = 0;

	if(packet == NULL)
		goto error;

	if(packet->type() != NET_PROTO_ATM)
		goto error;

	atm_cell = (AtmCell *) packet;

	it = this->switch_table.find(atm_cell->talId());

	if(it != this->switch_table.end())
		spot_id = (*it).second;

error:
	return spot_id;
}


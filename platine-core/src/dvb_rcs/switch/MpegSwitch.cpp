/**
 * @file MpegSwitch.cpp
 * @brief MPEG switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "MpegSwitch.h"


MpegSwitch::MpegSwitch()
{
}

MpegSwitch::~MpegSwitch()
{
}

long MpegSwitch::find(NetPacket *packet)
{
	MpegPacket *mpeg_packet;
	std::map < long, long >::iterator it;
	long spot_id = 0;

	if(packet == NULL)
		goto error;

	if(packet->type() != NET_PROTO_MPEG)
		goto error;

	mpeg_packet = (MpegPacket *) packet;

	it = this->switch_table.find(mpeg_packet->talId());

	if(it != this->switch_table.end())
		spot_id = (*it).second;

error:
	return spot_id;
}


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */


/**
 * @file     ReturnScheduling.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS return link
 * @author   Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "ReturnSchedulingRcsCommon.h"
#include "MacFifoElement.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

ReturnSchedulingRcsCommon::ReturnSchedulingRcsCommon(
			const EncapPlugin::EncapPacketHandler *packet_handler,
			const fifos_t &fifos):
	Scheduling(packet_handler, fifos, NULL)
{
}


bool ReturnSchedulingRcsCommon::schedule(const time_sf_t current_superframe_sf,
                                         clock_t UNUSED(current_time),
                                         list<DvbFrame *> *complete_dvb_frames,
                                         uint32_t &remaining_allocation)
{
	if(remaining_allocation > (unsigned int)pow(2.0, 8 * sizeof(rate_pktpf_t)))
	{
		LOG(this->log_scheduling, LEVEL_NOTICE,
		    "Remaining allocation (%u) is too long and will be "
		    "truncated\n", remaining_allocation);
	}
	// extract and send encap packets from MAC FIFOs, in function of
	// UL allocation
	if(!this->macSchedule(current_superframe_sf,
	                      complete_dvb_frames,
	                      (rate_pktpf_t &)remaining_allocation))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: MAC scheduling failed\n",
		    current_superframe_sf);
		return false;
	}

	return true;
}

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file Gate.cpp
 * @brief Gate: Class that implements Error Insertion, Attribute class of 
 *                   Channel that manages how bit errors affect the frames.
 *                   -Gate process the packets with an ON/OFF perspective.
 *                   If the Carrier to Noise ratio is below a certain threshold,
 *                   the whole packet will corrupted in all its bits.
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "Gate.h"

#include <math.h>
#include <strings.h>
#include <cstring>

Gate::Gate():ErrorInsertionPlugin()
{
}


Gate::~Gate()
{
}

bool Gate::init()
{
	return true;
}

bool Gate::isToBeModifiedPacket(double cn_uplink, double nominal_cn,
                                double attenuation, double threshold_qef)
{
	double cn_downlink;
	double cn_total;
	double num_downlink;
	double num_uplink;
	double num_total;
	bool do_modify;

	// Get the current download C/N
	cn_downlink = (nominal_cn - attenuation);

	UTI_DEBUG_L3("C/N previous: %f, C/N new: %f \n",
	             cn_uplink, cn_downlink);

	// Calculation of total C/N ratio taking into account cn_uplink and 
	// C/N downlink 
	num_uplink = pow(10, cn_uplink/10);
	num_downlink = pow(10, cn_downlink/10);

	num_total = 1 / ((1/num_uplink) + (1/num_downlink)); 
	cn_total = 10 * log10(num_total);

	UTI_DEBUG_L3("C/N total: %f, required C/N: %f \n",
	          cn_total, threshold_qef);
	// Comparison between current and required C/N values
	if(cn_total >= threshold_qef)
	{
		UTI_DEBUG_L3("Packet is not to be modified \n");
		do_modify = false;
	}
	else
	{
		UTI_DEBUG_L3("Packet should be modified\n");
		do_modify = true;
	}
	return do_modify;
}

void Gate::modifyPacket(T_DVB_META *frame, long length)
{
	T_DVB_META *meta = (T_DVB_META *)frame;
	T_DVB_HDR *dvb_hdr = (T_DVB_HDR *)(meta->hdr);

	UTI_DEBUG_L3("Packet is modified\n");
	memset(frame->hdr, '\0', length);
	dvb_hdr->msg_type = MSG_TYPE_CORRUPTED;
}


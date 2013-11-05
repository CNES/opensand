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
 * @file PhyChannel.cpp
 * @brief PhyChannel
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include  "PhyChannel.h"

#include <math.h>

PhyChannel::PhyChannel():
	status(true),
	attenuation_model(NULL),
	minimal_condition(NULL),
	error_insertion(NULL),
	granularity(0)
{
}

PhyChannel::~PhyChannel()
{
}

bool PhyChannel::update()
{
	const char *FUNCNAME = "[Channel::update]";

	if(!this->status)
	{
		UTI_DEBUG_L3("channel is broken, do not update it");
		goto error;
	}

	UTI_DEBUG("%s Channel updated\n", FUNCNAME);
	if(this->attenuation_model->updateAttenuationModel())
	{
		UTI_DEBUG("%s New attenuation: %f \n",
		          FUNCNAME, this->attenuation_model->getAttenuation());
	}
	else
	{
		UTI_ERROR("channel updating failed, disable it");
		this->status = false;
	}

error:
	return this->status;
}

double PhyChannel::getTotalCN(T_DVB_PHY *phy_frame)
{
	double cn_down, cn_up, cn_total; 
	double num_down, num_up, num_total; 

	/* C/N calculation of downlink, as the substraction of the Nominal C/N
	 * with the Attenuation */
	cn_down = this->nominal_condition - this->attenuation_model->getAttenuation();

	/* C/N of uplink */ 
	cn_up = phy_frame->cn_previous; 

	// if -1 we are in the regenerative case were downlink C/N is entirely 
	// defined here
	if(cn_up != -1)
	{
		// Calculation of the sub total C/N ratio
		num_down = pow(10, cn_down / 10);
		num_up = pow(10, cn_up / 10);

		num_total = 1 / ((1 / num_down) + (1 / num_up)); 
		cn_total = 10 * log10(num_total);
	}
	else
	{
		cn_total = cn_down;
	}

	UTI_DEBUG_L3("Satellite: cn_downlink= %f cn_uplink= %f cn_total= %f \n",
	             cn_down, cn_up, cn_total);

	return cn_total;
}


void PhyChannel::addSegmentCN(T_DVB_PHY *phy_frame)
{

	const char *FUNCNAME = "[Channel::addSegmentCN]";
	double val; 

	/* C/N calculation as the substraction of the Nominal C/N with
	   the Attenuation for this segment(uplink) */

	val = this->nominal_condition - this->attenuation_model->getAttenuation();
	UTI_DEBUG("%s Calculation of C/N: %f \n", FUNCNAME, val);

	phy_frame->cn_previous = val;
}


bool PhyChannel::isToBeModifiedPacket(double cn_total)
{
	return error_insertion->isToBeModifiedPacket(cn_total,
	                                             this->minimal_condition->getMinimalCN());
}

void PhyChannel::modifyPacket(T_DVB_META *frame, long length)
{
	T_DVB_HDR *dvb_hdr = (T_DVB_HDR *)(frame->hdr);
	unsigned char *payload = (unsigned char *)dvb_hdr + sizeof(T_DVB_HDR);
	length -= sizeof(T_DVB_HDR);

	if(error_insertion->modifyPacket(payload, length))
	{
		dvb_hdr->msg_type = MSG_TYPE_CORRUPTED;
	}
}

bool PhyChannel::updateMinimalCondition(T_DVB_HDR *hdr)
{
	T_DVB_BBFRAME *bbheader = (T_DVB_BBFRAME *) hdr;

	UTI_DEBUG_L3("Trace update minimal condition\n");

	if(!this->status)
	{
		UTI_DEBUG("channel is broken, do not update minimal condition");
		goto error;
	}

	// TODO remove when supporting other frames
	if(hdr->msg_type != MSG_TYPE_BBFRAME)
	{
		UTI_DEBUG("updateMinimalCondition called in transparent mode, "
		          "not supported currently\n");
		goto ignore;
	}

	if(!this->minimal_condition->updateThreshold(bbheader->used_modcod))
	{
		UTI_ERROR("Threshold update failed, the channel will be disabled\n");
		this->status = false;
		goto error;     
	}

ignore:
	UTI_DEBUG("Update minimal condition: %f\n",
	          this->minimal_condition->getMinimalCN());
error:
	return this->status;
}


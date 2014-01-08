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
#include "BBFrame.h"
#include "DvbRcsFrame.h"

#include <math.h>

PhyChannel::PhyChannel():
	status(true),
	nominal_condition(0),
	attenuation_model(NULL),
	minimal_condition(NULL),
	error_insertion(NULL),
	granularity(0),
	probe_attenuation(NULL),
	probe_nominal_condition(NULL),
	probe_minimal_condition(NULL),
	probe_total_cn(NULL),
	probe_drops(NULL)
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
		UTI_DEBUG("%s New attenuation: %.2f dB\n",
		          FUNCNAME, this->attenuation_model->getAttenuation());
	}
	else
	{
		UTI_ERROR("channel updating failed, disable it");
		this->status = false;
	}

	this->probe_attenuation->put(this->attenuation_model->getAttenuation());
	this->probe_nominal_condition->put(this->nominal_condition);

error:
	return this->status;
}

double PhyChannel::getTotalCN(DvbFrame *dvb_frame)
{
	double cn_down, cn_up, cn_total; 
	double num_down, num_up, num_total; 

	/* C/N calculation of downlink, as the substraction of the Nominal C/N
	 * with the Attenuation */
	cn_down = this->nominal_condition - this->attenuation_model->getAttenuation();

	/* C/N of uplink */ 
	cn_up = dvb_frame->getCn();

	// Calculation of the sub total C/N ratio
	num_down = pow(10, cn_down / 10);
	num_up = pow(10, cn_up / 10);

	num_total = 1 / ((1 / num_down) + (1 / num_up)); 
	cn_total = 10 * log10(num_total);

	// update CN in frame for DVB block transmission
	dvb_frame->setCn(cn_total);

	UTI_DEBUG_L3("Satellite: cn_downlink= %.2f dB cn_uplink= %.2f dB "
	             "cn_total= %.2f dB\n", cn_down, cn_up, cn_total);
	this->probe_total_cn->put(cn_total);

	return cn_total;
}


void PhyChannel::addSegmentCN(DvbFrame *dvb_frame)
{

	const char *FUNCNAME = "[Channel::addSegmentCN]";
	double val; 

	/* C/N calculation as the substraction of the Nominal C/N with
	   the Attenuation for this segment(uplink) */

	val = this->nominal_condition - this->attenuation_model->getAttenuation();
	UTI_DEBUG("%s Calculation of C/N: %.2f dB\n", FUNCNAME, val);

	dvb_frame->setCn(val);
}


bool PhyChannel::isToBeModifiedPacket(double cn_total)
{
	// we sum all values so we can put 0 here
	this->probe_drops->put(0);
	return error_insertion->isToBeModifiedPacket(cn_total,
	                                             this->minimal_condition->getMinimalCN());
}

void PhyChannel::modifyPacket(DvbFrame *dvb_frame)
{
	Data payload;

	// keep the complete header because we carry useful data
	if(dvb_frame->getMessageType() == MSG_TYPE_BBFRAME)
	{
		// TODO BBFrame *bbframe = dynamic_cast<BBFrame *>(dvb_frame);
		BBFrame *bbframe = dvb_frame->operator BBFrame *();

		payload = bbframe->getPayload();
	}
	else
	{
		// TODO DvbRcsFrame *dvb_rcs_frame = dynamic_cast<DvbRcsFrame *>(dvb_frame);
		DvbRcsFrame *dvb_rcs_frame = dvb_frame->operator DvbRcsFrame *();

		payload = dvb_rcs_frame->getPayload();
	}

	if(error_insertion->modifyPacket(payload))
	{
		dvb_frame->setMessageType(MSG_TYPE_CORRUPTED);
		this->probe_drops->put(1);
	}
}

bool PhyChannel::updateMinimalCondition(DvbFrame *dvb_frame)
{
	// TODO BBFrame *bbframe = dynamic_cast<BBFrame *>(dvb_frame);
	BBFrame *bbframe = (BBFrame *)dvb_frame;
	UTI_DEBUG_L3("Trace update minimal condition\n");

	if(!this->status)
	{
		UTI_DEBUG("channel is broken, do not update minimal condition");
		goto error;
	}

	// TODO remove when supporting other frames
	if(dvb_frame->getMessageType() != MSG_TYPE_BBFRAME)
	{
		// TODO on ne connait pas la source quand on recoit, et les
		// conditions en dépendent...
		UTI_DEBUG("updateMinimalCondition called in transparent mode, "
		          "not supported currently\n");
		goto ignore;
	}

	if(!this->minimal_condition->updateThreshold(bbframe->getModcodId()))
	{
		UTI_ERROR("Threshold update failed, the channel will be disabled\n");
		this->status = false;
		goto error;     
	}

	this->probe_minimal_condition->put(this->minimal_condition->getMinimalCN());

ignore:
	UTI_DEBUG("Update minimal condition: %.2f dB\n",
	          this->minimal_condition->getMinimalCN());
error:
	return this->status;
}


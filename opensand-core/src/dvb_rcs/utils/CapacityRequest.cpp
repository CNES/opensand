/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file    CapacityRequest.cpp
 * @brief   Represent a CR (Capacity Request)
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include "CapacityRequest.h"
#include "OpenSandFrames.h"

#include <cstring>


// RBDC request granularity in SAC (in Kbits/s)
#define DVB_CR_RBDC_GRANULARITY             2
#define DVB_CR_RBDC_SCALING_FACTOR         16
#define DVB_CR_VBDC_SCALING_FACTOR         16
#define DVB_CR_VBDC_SCALING_FACTOR_OFFSET 255
#define DVB_CR_RBDC_SCALING_FACTOR_OFFSET 510


static void getScaleAndValue(cr_info_t cr_info, uint8_t &scale, uint8_t &value);
static uint8_t getEncodedRequestValue(uint16_t value, unsigned int step);
static uint16_t getDecodedCrValue(const emu_cr_t &cr);


CapacityRequest::CapacityRequest(tal_id_t tal_id):
	tal_id(tal_id),
	requests()
{
}

CapacityRequest::~CapacityRequest()
{
	this->requests.clear();
}

void CapacityRequest::addRequest(uint8_t prio, uint8_t type, uint32_t value)
{
	cr_info_t info;
	info.prio = 0;
	info.type = type;
	info.value = value;
	this->requests.push_back(info);

}

bool CapacityRequest::parse(const unsigned char *data, size_t length)
{
	// remove all requests
	this->requests.clear();
	/* check that data contains DVB header, tal_id and cr_number */
	if(length < sizeof(T_DVB_HDR) + 2 * sizeof(uint8_t))
	{
		return false;
	}
	length -= sizeof(T_DVB_HDR) + 2 * sizeof(uint8_t);

	this->sac = *((emu_sac_t *)(data + sizeof(T_DVB_HDR)));
	this->tal_id = this->sac.tal_id;

	/* check that we can read enough cr */
	if(length < this->sac.cr_number * sizeof(emu_cr_t))
	{
		return false;
	}

	for(unsigned int i = 0; i < this->sac.cr_number; i++)
	{
		cr_info_t req;

		req.prio = this->sac.cr[i].prio;
		req.type = this->sac.cr[i].type;
		req.value = getDecodedCrValue(this->sac.cr[i]);

		this->requests.push_back(req);
	}
	return true;
}

void CapacityRequest::build(unsigned char *frame, size_t &length)
{
	T_DVB_SAC_CR dvb_sac;

	// fill T_DVB_SAC fields
	dvb_sac.hdr.msg_length = sizeof(T_DVB_HDR);
	dvb_sac.hdr.msg_type = MSG_TYPE_CR;

	// fill emu_sac_t fields
	this->sac.tal_id = this->tal_id;
	this->sac.cr_number = 0;
	for(unsigned int i = 0; i < this->requests.size() && i < NBR_MAX_CR; i++)
	{
		uint8_t scale;
		uint8_t value;
		this->sac.cr[i].type = this->requests[i].type;
		this->sac.cr[i].prio = this->requests[i].prio;
		getScaleAndValue(this->requests[i],
		                 scale, value);
		this->sac.cr[i].scale = scale;
		this->sac.cr[i].value = value;
		dvb_sac.hdr.msg_length += sizeof(emu_sac_t);
		this->sac.cr_number++;
	}
	dvb_sac.sac = this->sac;
	length = dvb_sac.hdr.msg_length;
	memcpy(frame, &dvb_sac, length);
	memset(&this->sac, '\0', sizeof(emu_sac_t));
	// remove all requests
	this->requests.clear();
}

/**
 * @brief compute the scale and values for a capacity request
 *
 * @param cr_info  the CR useful information (type, value)
 * @param scale    OUT: the scale for the encoded CR value
 * @param value    OUT: the encoded CR value
 */
static void getScaleAndValue(cr_info_t cr_info, uint8_t &scale, uint8_t &value)
{
	scale = 0;
	value = 0;
	switch(cr_info.type)
	{
		case cr_vbdc:
			if(cr_info.value <= DVB_CR_VBDC_SCALING_FACTOR_OFFSET)
			{
				value = cr_info.value;
				scale = 0;
			}
			else
			{
				value = getEncodedRequestValue(cr_info.value,
				                               DVB_CR_VBDC_SCALING_FACTOR);
				scale = 1;
			}
			break;

		case cr_rbdc:
			if(cr_info.value <= DVB_CR_RBDC_SCALING_FACTOR_OFFSET)
			{
				value = getEncodedRequestValue(cr_info.value,
				                               DVB_CR_RBDC_GRANULARITY);
				scale = 0;
			}
			else
			{
				value = getEncodedRequestValue(cr_info.value,
				                               DVB_CR_RBDC_GRANULARITY *
				                               DVB_CR_RBDC_SCALING_FACTOR);
				scale = 1;
			}
			break;
	}
}

/**
 * @brief Compute the number of specified steps within the input value
 *
 * @param value the request value
 * @param the step for encaoded value computation
 * @return the encoded value
 */
static uint8_t getEncodedRequestValue(uint16_t value, unsigned int step)
{
	uint8_t div_quot;
	uint8_t div_rem;

	/* compute quotient and reminder of integer division */
	div_quot = value / step;
	div_rem = value % step;

	/* approximate to the nearest value : */
	/* previous value */
	if(div_rem < (step / 2))
	{
		return div_quot;
	}
	/* or next value */
	else
	{
		return div_quot + 1;
	}
};

/**
 * @brief decode the capacity request in function of the
 *        encoded value  and scaling factor
 *
 * @param cr the emulated capacity request
 * @return the capacity request value
 */
static uint16_t getDecodedCrValue(const emu_cr_t &cr)
{
	uint16_t request = 0;

	switch(cr.type)
	{
		case cr_vbdc:
			if(cr.scale == 0)
				request = cr.value;
			else
				request = cr.value * DVB_CR_VBDC_SCALING_FACTOR;
			break;

		case cr_rbdc:
			if(cr.scale == 0)
				request = cr.value * DVB_CR_RBDC_GRANULARITY;
			else
				request = cr.value * DVB_CR_RBDC_GRANULARITY
				                   * DVB_CR_RBDC_SCALING_FACTOR;
			break;
	}

	return request;
}



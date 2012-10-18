/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file    CapacityRequest.cpp
 * @brief   Represent a CR (Capacity Request)
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include "CapacityRequest.h"
#include "lib_dvb_rcs.h"

#include <cstring>

// RBDC request granularity in SAC (in Kbits/s)
#define DVB_CR_RBDC_GRANULARITY             2
#define DVB_CR_RBDC_SCALING_FACTOR         16
#define DVB_CR_VBDC_SCALING_FACTOR         16
#define DVB_CR_VBDC_SCALING_FACTOR_OFFSET 255
#define DVB_CR_RBDC_SCALING_FACTOR_OFFSET 510


static void getScaleAndValue(cr_info_t cr_info, uint8_t &scale, uint8_t &value);
static uint8_t getEncodedRequestValue(uint8_t value, unsigned int step);

void CapacityRequest::build(unsigned char *frame, size_t &length)
{
	T_DVB_SAC_CR dvb_sac;
	emu_sac_t sac;

	// fill T_DVB_SAC fields
	dvb_sac.hdr.msg_length = sizeof(T_DVB_HDR);
    dvb_sac.hdr.msg_type = MSG_TYPE_CR;

	// fill emu_sac_t fields
	sac.tal_id = this->tal_id;
	sac.cr_number = 0;
	for(unsigned int i = 0; i < this->requests.size() && i < NBR_MAX_CR; i++)
	{
		uint8_t scale;
		uint8_t value;
		sac.cr[i].type = this->requests[i].type;
		sac.cr[i].prio = this->requests[i].prio;
		getScaleAndValue(this->requests[i],
		                 scale, value);
		sac.cr[i].scale = scale;
		sac.cr[i].value = value;
		dvb_sac.hdr.msg_length += sizeof(emu_sac_t);
		sac.cr_number++;
	}
	dvb_sac.sac = sac;
	length = dvb_sac.hdr.msg_length;
	memcpy(frame, &dvb_sac, length);
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
static uint8_t getEncodedRequestValue(uint8_t value, unsigned int step)
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

uint16_t getDecodedCrValue(const emu_cr_t &cr)
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



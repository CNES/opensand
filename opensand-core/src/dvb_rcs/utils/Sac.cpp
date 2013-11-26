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
 * @file    Sac.cpp
 * @brief   Represent a Satellite Access Control message
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include "Sac.h"
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


Sac::Sac(tal_id_t tal_id, group_id_t group_id):
	tal_id(tal_id),
	group_id(group_id),
	cni(-100), // very low as we will force the most robust MODCOD at beginning
	requests()
{
	this->sac = (emu_sac_t *)calloc(this->getMaxSize(),
	                                sizeof(unsigned char));
}

Sac::Sac():
	sac(NULL)
{
}

Sac::~Sac()
{
	this->requests.clear();
	if(this->sac)
	{
		free(this->sac);
	}
}

void Sac::addRequest(uint8_t prio, uint8_t type, uint32_t value)
{
	cr_info_t info;
	info.prio = prio;
	info.type = type;
	info.value = value;
	this->requests.push_back(info);

}

bool Sac::parse(const unsigned char *data, size_t length)
{
	// remove all requests
	this->requests.clear();
	/* check that data contains DVB header, tal_id, acm  and cr_number */
	if(length < sizeof(T_DVB_HDR) + 2 * sizeof(uint8_t) + sizeof(emu_acm_t))
	{
		return false;
	}
	length -= sizeof(T_DVB_HDR) + 2 * sizeof(uint8_t) - sizeof(emu_acm_t);

	this->sac = (emu_sac_t *)(data + sizeof(T_DVB_HDR));
	this->tal_id = ntohs(this->sac->tal_id);
	this->group_id = this->sac->group_id;
	this->cni = ncntoh(this->sac->acm.cni);

	/* check that we can read enough cr */
	if(length < this->sac->cr_number * sizeof(emu_cr_t))
	{
		return false;
	}

	for(unsigned int i = 0; i < this->sac->cr_number; i++)
	{
		cr_info_t req;

		req.prio = this->sac->cr[i].prio;
		req.type = this->sac->cr[i].type;
		req.value = getDecodedCrValue(this->sac->cr[i]);

		this->requests.push_back(req);
	}
	// to avoid bad release at destruction
	this->sac = NULL;
	return true;
}

void Sac::build(unsigned char *frame, size_t &length)
{
	T_DVB_SAC *dvb_sac = (T_DVB_SAC *)frame;

	// fill T_DVB_SAC fields
	dvb_sac->hdr.msg_length = sizeof(T_DVB_HDR);
	dvb_sac->hdr.msg_type = MSG_TYPE_SAC;

	// fill emu_sac_t fields
	this->sac->tal_id = htons(this->tal_id);
	dvb_sac->hdr.msg_length += sizeof(tal_id_t);
	this->sac->group_id = this->group_id;
	dvb_sac->hdr.msg_length += sizeof(group_id_t);
	this->sac->acm.cni = hcnton(this->cni);
	dvb_sac->hdr.msg_length += sizeof(emu_acm_t);
	this->sac->cr_number = 0;
	dvb_sac->hdr.msg_length += sizeof(uint8_t);
	for(unsigned int i = 0; i < this->requests.size() && i < NBR_MAX_CR; i++)
	{
		uint8_t scale;
		uint8_t value;
		this->sac->cr[i].type = this->requests[i].type;
		this->sac->cr[i].prio = this->requests[i].prio;
		getScaleAndValue(this->requests[i],
		                 scale, value);
		this->sac->cr[i].scale = scale;
		this->sac->cr[i].value = value;
		dvb_sac->hdr.msg_length += sizeof(emu_cr_t);
		this->sac->cr_number++;
	}
	length = dvb_sac->hdr.msg_length;
	memcpy(&dvb_sac->sac, this->sac, length - sizeof(T_DVB_HDR));
	// TODO we should be able to remove that
	memset(this->sac, '\0', this->getMaxSize());
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



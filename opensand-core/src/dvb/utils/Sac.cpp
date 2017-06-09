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

#include <opensand_output/Output.h>

#include <cstring>


// RBDC request granularity in SAC (in Kbits/s)
#define DVB_CR_RBDC_GRANULARITY             2
#define DVB_CR_RBDC_SCALING_FACTOR         16
#define DVB_CR_RBDC_SCALING_FACTOR2        32
#define DVB_CR_VBDC_SCALING_FACTOR         16
#define DVB_CR_VBDC_SCALING_FACTOR_OFFSET 255
#define DVB_CR_RBDC_SCALING_FACTOR_OFFSET 510

OutputLog *Sac::sac_log = NULL;

static void getScaleAndValue(cr_info_t cr_info, uint8_t &scale, uint8_t &value);
static uint8_t getEncodedRequestValue(uint16_t value, unsigned int step);
static uint16_t getDecodedCrValue(const emu_cr_t &cr);

Sac::Sac(tal_id_t tal_id, group_id_t group_id):
	DvbFrameTpl<T_DVB_SAC>(),
	request_nbr(0)
{
	this->setMessageType(MSG_TYPE_SAC);
	this->setMessageLength(sizeof(T_DVB_SAC));
	this->setMaxSize(sizeof(T_DVB_SAC) + (sizeof(emu_cr_t) * NBR_MAX_CR));
	this->frame()->sac.tal_id = htons(tal_id);
	this->frame()->sac.group_id = group_id;
	// very low as we will force the most robust MODCOD at beginning
	this->frame()->sac.acm.cni = hcnton(-100);
	this->frame()->sac.cr_number = 0;
}

Sac::~Sac()
{
}

bool Sac::addRequest(uint8_t prio, uint8_t type, uint32_t value)
{
	uint8_t scale;
	uint8_t val;
	cr_info_t info;
	info.prio = prio;
	info.type = type;
	info.value = value;
	emu_cr_t cr;

	if(this->request_nbr + 1 >= NBR_MAX_CR)
	{
		LOG(sac_log, LEVEL_ERROR, 
		    "Cannot add more request\n");
		return false;
	}
	this->request_nbr++;

	cr.type = type;
	cr.prio = prio;
	getScaleAndValue(info, scale, val);
	cr.scale = scale;
	cr.value = val;
	this->data.append((unsigned char *)&cr, sizeof(emu_cr_t));
	// increase cr_number
	this->frame()->sac.cr_number++;
	// increase message length
	this->setMessageLength(this->getMessageLength() + sizeof(emu_cr_t));
	return true;
}


tal_id_t Sac::getTerminalId(void) const 
{
	return ntohs(this->frame()->sac.tal_id);
}

group_id_t Sac::getGroupId(void) const
{
	return this->frame()->sac.group_id;
}

double Sac::getCni() const
{
	return ncntoh(this->frame()->sac.acm.cni);
}

vector<cr_info_t> Sac::getRequests(void) const
{
	vector<cr_info_t> requests;

	for(unsigned int i = 0; i < this->frame()->sac.cr_number; i++)
	{
		cr_info_t req;

		req.prio = this->frame()->sac.cr[i].prio;
		req.type = this->frame()->sac.cr[i].type;
		req.value = getDecodedCrValue(this->frame()->sac.cr[i]);

		requests.push_back(req);
	}
	return requests;
};


void Sac::setAcm(double cni)
{
	LOG(sac_log, LEVEL_INFO, 
	    "Set CNI value %f in SAC\n", cni);
	this->frame()->sac.acm.cni = hcnton(cni);
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
		case access_dama_vbdc:
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

		case access_dama_rbdc:
			if(cr_info.value <= DVB_CR_RBDC_SCALING_FACTOR_OFFSET)
			{
				value = getEncodedRequestValue(cr_info.value,
				                               DVB_CR_RBDC_GRANULARITY);
				scale = 0;
			}
			else if(cr_info.value <= DVB_CR_RBDC_SCALING_FACTOR_OFFSET *
			                         DVB_CR_RBDC_SCALING_FACTOR)
			{
				value = getEncodedRequestValue(cr_info.value,
				                               DVB_CR_RBDC_GRANULARITY *
				                               DVB_CR_RBDC_SCALING_FACTOR);
				scale = 1;
			}
			else
			{
				value = getEncodedRequestValue(cr_info.value,
				                               DVB_CR_RBDC_GRANULARITY *
				                               DVB_CR_RBDC_SCALING_FACTOR2);
				scale = 2;
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
		case access_dama_vbdc:
			if(cr.scale == 0)
				request = cr.value;
			else
				request = cr.value * DVB_CR_VBDC_SCALING_FACTOR;
			break;

		case access_dama_rbdc:
			if(cr.scale == 0)
				request = cr.value * DVB_CR_RBDC_GRANULARITY;
			else if(cr.scale == 1)
				request = cr.value * DVB_CR_RBDC_GRANULARITY
				                   * DVB_CR_RBDC_SCALING_FACTOR;
			else
				request = cr.value * DVB_CR_RBDC_GRANULARITY
				                   * DVB_CR_RBDC_SCALING_FACTOR2;
			break;
	}

	return request;
}



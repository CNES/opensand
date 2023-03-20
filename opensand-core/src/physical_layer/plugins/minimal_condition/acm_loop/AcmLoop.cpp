/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
 * Copyright © 2019 TAS
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
 * @file AcmLoop.cpp
 * @brief AcmLoop
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */


#include "AcmLoop.h"
#include "OpenSandFrames.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


AcmLoop::AcmLoop():
		MinimalConditionPlugin(),
		modcod_table_rcs(),
		modcod_table_s2()
{
}


AcmLoop::~AcmLoop()
{
}


void AcmLoop::generateConfiguration(const std::string &,
                                    const std::string &,
                                    const std::string &)
{
}


bool AcmLoop::init(void)
{
	auto Conf = OpenSandModelConf::Get();
	std::vector<OpenSandModelConf::fmt_definition_parameters> modcod_params;

	vol_sym_t req_burst_length;
	if(!Conf->getRcs2BurstLength(req_burst_length))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "missing parameter 'RCS2 burst length'\n");
		return false;
	}

	modcod_params.clear();
	if(!Conf->getRcs2WaveFormsDefinition(modcod_params, req_burst_length))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to load the acm_loop definition table for RCS2");
		return false;
	}
	for(auto& param : modcod_params)
	{
		if(!this->modcod_table_rcs.add(std::make_unique<FmtDefinition>(param.id,
		                                                               param.modulation_type,
		                                                               param.coding_type,
		                                                               param.spectral_efficiency,
		                                                               param.required_es_no,
		                                                               req_burst_length)))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to create MODCOD table for RCS2 waveforms\n");
			return false;
		}
	}

	modcod_params.clear();
	if(!Conf->getS2WaveFormsDefinition(modcod_params))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to load the acm_loop definition table for S2");
		return false;
	}
	for(auto& param : modcod_params)
	{
		if(!this->modcod_table_s2.add(std::make_unique<FmtDefinition>(param.id,
		                                                              param.modulation_type,
		                                                              param.coding_type,
		                                                              param.spectral_efficiency,
		                                                              param.required_es_no)))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to create MODCOD table for S2 waveforms\n");
			return false;
		}
	}

	return true;
}


bool AcmLoop::updateThreshold(uint8_t modcod_id, EmulatedMessageType message_type)
{
	double threshold = this->minimal_cn; // Default, keep previous threshold
	switch(message_type)
	{
		case EmulatedMessageType::DvbBurst:
			threshold = double(this->modcod_table_rcs.getRequiredEsN0(modcod_id));
			LOG(this->log_minimal, LEVEL_DEBUG,
			    "Required Es/N0 for ACM loop %u --> %.2f dB\n",
			    modcod_id, this->modcod_table_rcs.getRequiredEsN0(modcod_id));
			break;
		default:
			threshold = double(this->modcod_table_s2.getRequiredEsN0(modcod_id));
			LOG(this->log_minimal, LEVEL_DEBUG,
			    "Required Es/N0 for ACM loop %u --> %.2f dB\n",
			    modcod_id, this->modcod_table_s2.getRequiredEsN0(modcod_id));
	}

	this->minimal_cn = threshold;
	return true;
}

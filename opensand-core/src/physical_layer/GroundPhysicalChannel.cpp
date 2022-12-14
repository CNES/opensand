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
 * @file GroundPhysicalChannel.cpp
 * @brief Ground Physical Layer Channel
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 * @author Aurélien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "GroundPhysicalChannel.h"
#include "Plugin.h"
#include "FifoElement.h"
#include "OpenSandCore.h"
#include "OpenSandModelConf.h"
#include "NetContainer.h"

#include <math.h>
#include <algorithm>
#include <string>

#include <opensand_rt/RtChannelBase.h>


GroundPhysicalChannel::GroundPhysicalChannel(PhyLayerConfig config):
	clear_sky_condition{0},
	delay_fifo{},
	mac_id{config.mac_id},
	entity_type{config.entity_type},
	spot_id{config.spot_id},
	attenuation_update_timer{-1},
	fifo_timer{-1}
{
	// Initialize logs
	this->log_channel = Output::Get()->registerLog(LEVEL_WARNING, "PhysicalLayer.Channel");
}

void GroundPhysicalChannel::generateConfiguration()
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();

	auto conf = Conf->getOrCreateComponent("physical_layer", "Physical Layer", "The Physical layer configuration");
	auto uplink = Conf->getOrCreateComponent("uplink_attenuation", "UpLink Attenuation", conf);
	uplink->addParameter("clear_sky", "Clear Sky Condition", types->getType("double"))->setUnit("dB");

	auto downlink = Conf->getOrCreateComponent("downlink_attenuation", "DownLink Attenuation", conf);
	downlink->addParameter("clear_sky", "Clear Sky Condition", types->getType("double"))->setUnit("dB");

	Plugin::generatePluginsConfiguration(uplink, PluginType::Attenuation, "attenuation_type", "Attenuation Type");
	Plugin::generatePluginsConfiguration(downlink, PluginType::Attenuation, "attenuation_type", "Attenuation Type");
}

void GroundPhysicalChannel::setSatDelay(SatDelayPlugin *satdelay)
{
	this->satdelay_model = satdelay;
}

bool GroundPhysicalChannel::initGround(bool upward_channel, RtChannelBase *channel, std::shared_ptr<OutputLog> log_init)
{
	auto output = Output::Get();
	auto Conf = OpenSandModelConf::Get();

	std::string link = upward_channel ? "Down" : "Up";
	std::string component = upward_channel ? "downlink_attenuation" : "uplink_attenuation";
	std::string component_path = std::string{"physical_layer/"} + component;
	auto phy_layer = Conf->getProfileData()->getComponent("physical_layer");
	auto link_attenuation = phy_layer->getComponent(component);

	// generate probes prefix
	bool is_sat = Conf->getComponentType() == Component::satellite;
	std::string probe_prefix = generateProbePrefix(spot_id, entity_type, is_sat);

	// Get the FIFO max size
	// vol_pkt_t max_size;
	std::size_t max_size;
	if(!Conf->getDelayBufferSize(max_size))
	{
		LOG(log_init, LEVEL_ERROR,
		    "cannot get 'delay_buffer' value");
		return false;
	}
	this->delay_fifo.setMaxSize(max_size);
	LOG(log_init, LEVEL_NOTICE,
	    "delay_fifo_max_size = %d pkt", max_size);

	// Get the delay refresh period
	time_ms_t refresh_period_ms;
	if(!Conf->getDelayTimer(refresh_period_ms))
	{
		LOG(log_init, LEVEL_ERROR,
		    "cannot get 'delay_timer' value");
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "delay_refresh_period = %d ms", refresh_period_ms);

	// Initialize the FIFO event
	this->fifo_timer = channel->addTimerEvent("fifo_timer", refresh_period_ms);

	// Initialize log
	this->log_event = output->registerLog(LEVEL_WARNING, "PhysicalLayer." + link + "ward.Event");

	// Get the refresh period
	if(!Conf->getAcmRefreshPeriod(refresh_period_ms))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section 'timers': missing parameter 'ACM refresh period'");
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "attenuation_refresh_period = %d ms", refresh_period_ms);

	// Get the clear sky condition
	if(!OpenSandModelConf::extractParameterData(link_attenuation->getParameter("clear_sky"), this->clear_sky_condition))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter 'clear sky condition'",
		    component_path.c_str());
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "clear_sky_conditions = %f dB", this->clear_sky_condition);

	// Get the attenuation type
	std::string attenuation_type;
	if(!OpenSandModelConf::extractParameterData(link_attenuation->getParameter("attenuation_type"), attenuation_type))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter 'attenuation type'",
		    component_path.c_str());
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "attenuation_type = %s", attenuation_type.c_str());

	// Get the attenuation plugin
	if(!Plugin::getAttenuationPlugin(attenuation_type, &this->attenuation_model))
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to get the physical layer attenuation plugin");
		return false;
	}

	// Initialize the attenuation plugin
	if(!this->attenuation_model->init(refresh_period_ms, component_path))
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to initialize the physical layer attenuation plugin %s",
		    attenuation_type.c_str());
		return false;
	}

	// Initialize the attenuation event
	std::ostringstream name;
	name << "attenuation_" << link;
	this->attenuation_update_timer = channel->addTimerEvent(name.str(), refresh_period_ms);

	// Initialize attenuation probes
	this->probe_attenuation =
	    output->registerProbe<float>(probe_prefix + "Phy." + link + "link_attenuation",
	                                 "dB", true, SAMPLE_MAX);

	this->probe_clear_sky_condition =
	    output->registerProbe<float>(probe_prefix + "Phy." + link + "link_clear_sky_condition",
	                                 "dB", true, SAMPLE_MAX);

	return true;
}

bool GroundPhysicalChannel::updateAttenuation()
{
	LOG(this->log_channel, LEVEL_DEBUG,
		"Update attenuation");

	if(!this->attenuation_model->updateAttenuationModel())
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Attenuation update failed");
		return false;
	}

	double attenuation = this->attenuation_model->getAttenuation();

	LOG(this->log_channel, LEVEL_INFO,
		"New attenuation: %.2f dB",
		attenuation);
	this->probe_attenuation->put(attenuation);
	this->probe_clear_sky_condition->put(this->clear_sky_condition);

	return true;
}

double GroundPhysicalChannel::getCurrentCn() const
{
	// C/N calculation, as the substraction of the clear sky C/N with the Attenuation
	return this->clear_sky_condition - this->attenuation_model->getAttenuation();
}

double GroundPhysicalChannel::computeTotalCn(double up_cn, double down_cn)
{
	double total_cn; 
	double down_num, up_num, total_num; 

	// Calculation of the sub total C/N ratio
	down_num = pow(10, down_cn / 10);
	up_num = pow(10, up_cn / 10);

	total_num = 1 / ((1 / down_num) + (1 / up_num)); 
	total_cn = 10 * log10(total_num);

	return total_cn;
}

bool GroundPhysicalChannel::pushPacket(NetContainer *pkt)
{
	FifoElement *elem;
	time_ms_t current_time = getCurrentTime();
	time_ms_t delay = this->satdelay_model->getSatDelay();

	// create a new FIFO element to store the packet
	try
	{
		elem = new FifoElement(std::unique_ptr<NetContainer>{pkt}, current_time, current_time + delay);
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Cannot allocate FIFO element, drop data");
		return false;
	}

	// append the data in the fifo
	if(!this->delay_fifo.push(elem))
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "FIFO is full: drop data");
		delete elem;
		return false;
	}

	LOG(this->log_channel, LEVEL_NOTICE,
	    "%s data stored in FIFO (tick_in = %ld, tick_out = %ld, delay = %u ms)",
	    pkt->getName().c_str(),
	    elem->getTickIn(),
	    elem->getTickOut(),
	    delay);
	return true;
}

bool GroundPhysicalChannel::forwardReadyPackets()
{
	time_ms_t current_time = getCurrentTime();

	LOG(this->log_channel, LEVEL_DEBUG,
		"Forward ready packets");

	while (this->delay_fifo.getCurrentSize() > 0 &&
	       ((unsigned long)this->delay_fifo.getTickOut()) <= current_time)
	{
		FifoElement *elem = this->delay_fifo.pop();
		assert(elem != nullptr);

		std::unique_ptr<NetContainer> pkt = elem->getElem();
		delete elem;
		this->forwardPacket(reinterpret_cast<DvbFrame *>(pkt.release()));
	}
	return true;
}

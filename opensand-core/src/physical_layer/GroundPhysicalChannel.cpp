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
#include "DelayFifoElement.h"
#include "OpenSandCore.h"

#include <opensand_conf/conf.h>

#include <math.h>
#include <algorithm>

using std::ostringstream;

GroundPhysicalChannel::GroundPhysicalChannel(tal_id_t mac_id):
	attenuation_model(NULL),
	clear_sky_condition(0),
	delay_fifo(),
	probe_attenuation(NULL),
	probe_clear_sky_condition(NULL),
	mac_id(mac_id),
	log_event(NULL),
	log_channel(NULL),
	satdelay_model(NULL),
	attenuation_update_timer(-1),
	fifo_timer(-1)
{
	// Initialize logs
	this->log_channel = Output::Get()->registerLog(LEVEL_WARNING, "PhysicalLayer.Channel");
}

GroundPhysicalChannel::~GroundPhysicalChannel()
{
}

void GroundPhysicalChannel::setSatDelay(SatDelayPlugin *satdelay)
{
	this->satdelay_model = satdelay;
}

bool GroundPhysicalChannel::initGround(const string &channel_name, RtChannel *channel, std::shared_ptr<OutputLog> log_init)
{
	ostringstream name;
	char probe_name[128];
	vol_pkt_t max_size;
	time_ms_t refresh_period_ms;
	string attenuation_type;
	string phy_layer_section;
	string link, lc_link;
  auto output = Output::Get();

	if(channel_name.compare(UP) == 0)
	{
		link = DOWN;
		lc_link = DOWN_LOWER_CASE;
		phy_layer_section = DOWNLINK_PHYSICAL_LAYER_SECTION;
	}
	else if(channel_name.compare(DOWN) == 0)
	{
		link = UP;
		lc_link = UP_LOWER_CASE;
		phy_layer_section = UPLINK_PHYSICAL_LAYER_SECTION;
	}
	else
	{
		LOG(log_init, LEVEL_ERROR,
		    "Invalid channel type specified");
		return false;
	}

	// Sanity check
	assert(this->satdelay_model != NULL);
	
	// Get the FIFO max size
	if(!Conf::getValue(Conf::section_map[ADV_SECTION],
	                   DELAY_BUFFER, max_size))
	{
		LOG(log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_BUFFER);
		return false;
	}
	this->delay_fifo.setMaxSize(max_size);
	LOG(log_init, LEVEL_NOTICE,
	    "delay_fifo_max_size = %d pkt", max_size);

	// Get the delay refresh period
	if(!Conf::getValue(Conf::section_map[ADV_SECTION],
	                   DELAY_TIMER, refresh_period_ms))
	{
		LOG(log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_TIMER);
		return false;;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "delay_refresh_period = %d ms", refresh_period_ms);

	// Initialize the FIFO event
	this->fifo_timer = channel->addTimerEvent("fifo_timer", refresh_period_ms);

	// Initialize log
	snprintf(probe_name, sizeof(probe_name),
	         "PhysicalLayer.%sward.Event", channel_name.c_str());
	this->log_event = output->registerLog(LEVEL_WARNING, probe_name);

	// Get the refresh period
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   ACM_PERIOD_REFRESH,
	                   refresh_period_ms))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'",
		    PHYSICAL_LAYER_SECTION, ACM_PERIOD_REFRESH);
		return false;
	}

	LOG(log_init, LEVEL_NOTICE,
	    "attenuation_refresh_period = %d ms", refresh_period_ms);

	// Get the clear sky condition
	if(!Conf::getValue(Conf::section_map[phy_layer_section.c_str()],
	                   CLEAR_SKY_CONDITION,
	                   this->clear_sky_condition))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'",
		    phy_layer_section.c_str(),
		    ATTENUATION_MODEL_TYPE);
		return false;
	}
	LOG(log_init, LEVEL_NOTICE,
	    "clear_sky_conditions = %d dB", this->clear_sky_condition);

	// Get the attenuation type
	if(!Conf::getValue(Conf::section_map[phy_layer_section.c_str()],
	                   ATTENUATION_MODEL_TYPE,
	                   attenuation_type))
	{
		LOG(log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'",
		    phy_layer_section.c_str(),
		    ATTENUATION_MODEL_TYPE);
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
	if(!this->attenuation_model->init(refresh_period_ms, lc_link))
	{
		LOG(log_init, LEVEL_ERROR,
		    "Unable to initialize the physical layer attenuation plugin %s",
		    attenuation_type.c_str());
		return false;
	}

	// Initialize the attenuation event
	name << "attenuation_" << link;
	this->attenuation_update_timer = channel->addTimerEvent(name.str(), refresh_period_ms);

	// Initialize attenuation probes
	snprintf(probe_name, sizeof(probe_name),
	         "Phy.%slink_attenuation", link.c_str());
	this->probe_attenuation = output->registerProbe<float>(probe_name, "dB", true, SAMPLE_MAX);

	snprintf(probe_name, sizeof(probe_name),
	         "Phy.%slink_clear_sky_condition", link.c_str());
	this->probe_clear_sky_condition = output->registerProbe<float>(probe_name, "dB", true, SAMPLE_MAX);

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
	DelayFifoElement *elem;
	time_ms_t current_time = getCurrentTime();
	time_ms_t delay = this->satdelay_model->getSatDelay();

	// create a new FIFO element to store the packet
	elem = new DelayFifoElement(pkt, current_time, current_time + delay);
	if(!elem)
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "Cannot allocate FIFO element, drop data");
		goto error;
	}

	// append the data in the fifo
	if(!this->delay_fifo.push(elem))
	{
		LOG(this->log_channel, LEVEL_ERROR,
		    "FIFO is full: drop data");
		goto release_elem;
	}

	LOG(this->log_channel, LEVEL_NOTICE,
	    "%s data stored in FIFO (tick_in = %ld, tick_out = %ld, delay = %u ms)",
	    pkt->getName().c_str(),
	    elem->getTickIn(),
	    elem->getTickOut(),
	    delay);
	return true;

release_elem:
	delete elem;

error:
	delete pkt;
	return false;
}

bool GroundPhysicalChannel::forwardReadyPackets()
{
	time_ms_t current_time = getCurrentTime();

	LOG(this->log_channel, LEVEL_DEBUG,
		"Forward ready packets");

	while (this->delay_fifo.getCurrentSize() > 0 &&
	       ((unsigned long)this->delay_fifo.getTickOut()) <= current_time)
	{
		NetContainer *pkt = NULL;
		DelayFifoElement *elem;

		elem = this->delay_fifo.pop();
		assert(elem != NULL);

		pkt = elem->getElem<NetContainer>();
		delete elem;
		this->forwardPacket((DvbFrame *)pkt);
	}
	return true;
}

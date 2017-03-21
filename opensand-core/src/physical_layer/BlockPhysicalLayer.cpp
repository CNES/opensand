/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 CNES
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
 * @file     BlockPhysicalLayer.cpp
 * @brief    PhysicalLayer bloc
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author   Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "BlockPhysicalLayer.h"

#include "Plugin.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "PhyChannel.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>


BlockPhysicalLayer::BlockPhysicalLayer(const string &name, tal_id_t mac_id):
	Block(name),
	mac_id(mac_id),
	satdelay(NULL)
{
}


BlockPhysicalLayer::~BlockPhysicalLayer()
{
}


bool BlockPhysicalLayer::onInit(void)
{
	if(!this->initSatDelay())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when loading satdelay plugin");
		goto error;
	}
	return true;
error:
	return false;
}

bool BlockPhysicalLayer::initSatDelay()
{
	ConfigurationList delays_list;
	ConfigurationList::iterator iter_list;
  ConfigurationList plugin_conf;
	uint8_t id;
	bool global_constant_delay;
	string satdelay_name;
	time_ms_t refresh_period_ms;

	/// Load de SatDelay Plugin
	// Get the orbit type
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   GLOBAL_CONSTANT_DELAY, global_constant_delay))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", GLOBAL_CONSTANT_DELAY);
		goto error;
	}
  // get the refresh period
  if(!Conf::getValue(Conf::section_map[SAT_DELAYS_SECTION],
                     REFRESH_PERIOD_MS, refresh_period_ms))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "cannot get '%s' value", REFRESH_PERIOD_MS);
    goto error;
  }
  // get all plugins 
  if(!Conf::getListItems(Conf::section_map[SAT_DELAYS_SECTION],
                         DELAYS_LIST, delays_list))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "section '%s': missing list '%s'\n",
        SAT_DELAYS_SECTION, DELAYS_LIST);
    goto error;
  }
  // if global constant delay, get the global delay configuration first
  if(global_constant_delay)
  {
    if(!Conf::getItemNode(Conf::section_map[SAT_DELAYS_SECTION],
                          GLOBAL_DELAY, plugin_conf))
    {
      LOG(this->log_init, LEVEL_ERROR,
          "missing parameter '%s'", GLOBAL_DELAY);
      goto error;
    }
		satdelay_name = CONSTANT_DELAY;
  }
	else
	{
		for(iter_list = delays_list.begin();
				iter_list != delays_list.end();
				iter_list++)
		{
			// get the id
			if(!Conf::getAttributeValue(iter_list, ID, id))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"cannot get delay id");
				goto error;
			}
			if(id != this->mac_id)
				continue;
			if(!Conf::getItemNode(*iter_list, SAT_DELAY_CONF, plugin_conf))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"missing parameter '%s' for delay terminal id %u",
						SAT_DELAY_CONF, id);
				goto error;
			}
			// get plugin name
			if(!Conf::getAttributeValue(iter_list, DELAY_TYPE, satdelay_name))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"missing parameter '%s' for terminal id %u",
						DELAY_TYPE, id);
				goto error;
			}
			break;
		}
	}
	// load plugin
	if(!Plugin::getSatDelayPlugin(satdelay_name,
																&this->satdelay))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"error when getting the sat delay plugin '%s'",
				satdelay_name.c_str());
		goto error;
	}
	// Check if the plugin was found
	if(this->satdelay == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
				"Satellite delay plugin conf was not found for"
				" terminal %s", this->mac_id);
		goto error;
	}
	// init plugin
	if(!this->satdelay->init(plugin_conf))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot initialize sat delay plugin '%s'"
				" for terminal id %u ", satdelay_name.c_str(), id);
		goto error;
	}
	
	// share the plugin to channels
	if(!((Upward *)this->upward)->setSatDelay(this->satdelay, false))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when setting the satdelay on upward channel");
		goto error;
	}
	if(!((Downward *)this->downward)->setSatDelay(this->satdelay, true))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when setting the satdelay on downward channel");
		goto error;
	}

  return true;
error:
  return false;
}

BlockPhysicalLayer::Downward::Downward(const string &name, tal_id_t UNUSED(mac_id)):
	RtDownward(name),
	PhyChannel(),
	attenuation(false),
	log_event(NULL)
{
	// Output Log
	this->log_event = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Downward.Event");
}

bool BlockPhysicalLayer::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message event: forward DVB frames from upper block to lower block
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			// TODO: if other components have to be added to physical layer,
			// think about using an array of pointer to functions, and iterate
			// over this array. The last function should be processDelay, since it 
			// queues the dvb_frame to the delay FIFO, and we lose hold of the 
			// dvb_frame.
			if(this->attenuation)
			{
				if(!this->processAttenuation(dvb_frame))
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "error when processing attenuation\n");
					return false;
				}
			}
			// TODO: this is wasteful, only regenerative satellites use the physical
			// layer. BlockPhysicalLayerSat should override onEvent method, without
			// queueing dvb_frames to FIFO
			if(!this->is_sat)
			{
				return this->pushInFifo((NetContainer *)dvb_frame,
						                    this->satdelay->getSatDelay());
			}
			else
			{
				// send frame to lower block
				if(!this->enqueueMessage((void **)&dvb_frame))
				{
					LOG(this->log_send, LEVEL_ERROR, 
							"failed to send burst of packets to lower layer\n");
					delete dvb_frame;
					return false;
				}
				return true;
			}
		}
		break;

		case evt_timer:
		{
			if (*event == this->fifo_timer)
			{
				// Event handler for delay FIFO
				if(!this->handleFifoTimer())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "downward channel delay fifo handling"
							" error");
					return false;
				}
			}
		  else if(*event == this->att_timer)
			{
				// Event handler for Downward Channel state update
				LOG(this->log_event, LEVEL_DEBUG,
						"downward channel timer expired\n");
				if(!this->update())
				{
					LOG(this->log_event, LEVEL_ERROR,
							"downward channel updating failed, do not "
							"update channels anymore\n");
					return false;
				}
			}
			else if(*event == this->delay_timer)
			{
				// Event handler for update satellite delay
				LOG(this->log_event, LEVEL_DEBUG,
				    "downward channel update satellite delay event");
				if(!this->satdelay->updateSatDelay())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "error when updating satellite delay");
					return false;
				}
				this->probe_delay->put(this->satdelay->getSatDelay());
			}
			else
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "unknown timer event received");
				return false;
			}
		}
		break;

		default:
			LOG(this->log_event, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

BlockPhysicalLayer::Upward::Upward(const string &name, tal_id_t UNUSED(mac_id)):
	RtUpward(name),
	PhyChannel(),
	attenuation(false),
	log_event(NULL)
{
	// Output Log
	this->log_event = Output::registerLog(LEVEL_WARNING, "PhysicalLayer.Upward.Event");
}

bool BlockPhysicalLayer::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message event: forward DVB frames from upper block to lower block
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			if(!this->is_sat)
			{
				return this->pushInFifo((NetContainer *)dvb_frame,
						                    this->satdelay->getSatDelay());
			}
			else
			{
				// TODO: if other components have to be added to physical layer,
				// think about using an array of pointer to functions, and iterate
				// over this array. The last function should be processDelay, since it 
				// queues the dvb_frame to the delay FIFO, and we lose hold of the 
				// dvb_frame.
				if(this->attenuation)
				{
					if(!this->processAttenuation(dvb_frame))
					{
						LOG(this->log_event, LEVEL_ERROR,
						    "error when processing attenuation");
						return false;
					}
				}
				// Send frame to upper layer
				if(!this->enqueueMessage((void **)&dvb_frame))
				{
					LOG(this->log_send, LEVEL_ERROR, 
							"failed to send burst of packets to upper layer\n");
					delete dvb_frame;
					return false;
				}
				return true;
			}
		}
		break;

		case evt_timer:
		{
			if(*event == this->fifo_timer)
			{
				// Event handler for delay FIFO
				if(!this->handleFifoTimer())
				{
					LOG(this->log_event, LEVEL_ERROR,
					    "upward channel delay fifo handling"
							" error");
					return false;
				}
			}
		  else if(*event == this->att_timer)
			{
				//Event handler for Upward Channel state update
				LOG(this->log_event, LEVEL_DEBUG,
						"upward channel timer expired\n");
				if(!this->update())
				{
					LOG(this->log_event, LEVEL_ERROR,
							"upward channel updating failed, do not "
							"update channels anymore\n");
					return false;
				}
			}
			else
			{
				LOG(this->log_event, LEVEL_ERROR,
				    "unknown timer event received");
				return false;
			}
		}
		break;
		
		default:
			LOG(this->log_event, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockPhysicalLayer::Upward::onInit(void)
{
	ostringstream name;
	string link("down"); // we are on downlink

	// Intermediate variables for Config file reading
	string attenuation_type;
	string minimal_type;
	string error_type;

	// check if attenuation is enabled
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   ENABLE, this->attenuation))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot check if physical layer is enabled\n");
		goto error;
	}
	if(!this->attenuation)
		return true;

	// get refresh period
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
		               ACM_PERIOD_REFRESH,
	                   this->refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, ACM_PERIOD_REFRESH);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "acm refreshing period = %d\n", this->refresh_period_ms);

	// Initiate Attenuation model
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   ATTENUATION_MODEL_TYPE,
	                   attenuation_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION,
		    ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Clear Sky value
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   CLEAR_SKY_CONDITION,
	                   this->clear_sky_condition))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION,
		    ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Minimal conditions
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   MINIMAL_CONDITION_TYPE,
	                   minimal_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION,
		    MINIMAL_CONDITION_TYPE);
		goto error;
	}

	// Initiate Error Insertion
	if(!Conf::getValue(Conf::section_map[DOWNLINK_PHYSICAL_LAYER_SECTION],
	                   ERROR_INSERTION_TYPE,
	                   error_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DOWNLINK_PHYSICAL_LAYER_SECTION, ERROR_INSERTION_TYPE);
		goto error;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins(attenuation_type,
	                                    minimal_type,
	                                    error_type,
	                                    &this->attenuation_model,
	                                    &this->minimal_condition,
	                                    &this->error_insertion))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting physical layer plugins");
		goto error;
	}
	if(!this->attenuation_model->init(this->refresh_period_ms, link))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize attenuation model plugin %s",
		    attenuation_type.c_str());
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "%slink: attenuation model = %s, clear_sky condition = %u, "
	    "minimal condition type = %s, error insertion type = %s",
	    link.c_str(), attenuation_type.c_str(),
	    this->clear_sky_condition,
	    minimal_type.c_str(), error_type.c_str());

	if(!this->minimal_condition->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize minimal condition plugin %s",
		    minimal_type.c_str());
		goto error;
	}

	if(!this->error_insertion->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize error insertion plugin %s",
		    error_type.c_str());
		goto error;
	}

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->refresh_period_ms);

	this->probe_attenuation = Output::registerProbe<float>("dB", true,
	                                                       SAMPLE_MAX,
	                                                       "Phy.%slink_attenuation (%s)",
	                                                       link.c_str(),
	                                                       attenuation_type.c_str());
	this->probe_minimal_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.minimal_condition (%s)",
	                                                             minimal_type.c_str());
	this->probe_clear_sky_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.%slink_clear_sky_condition",
	                                                             link.c_str());
	// no useful on GW because it depends on terminals and we do not make the difference here
	this->probe_total_cn = Output::registerProbe<float>("dB", true,
	                                                    SAMPLE_MAX,
	                                                    "Phy.%slink_total_cn",
	                                                    link.c_str());
	this->probe_drops = Output::registerProbe<int>("Phy.drops",
	                                               "frame number", true,
	                                               // we need to sum the drops here !
	                                               SAMPLE_SUM);

	return true;

error:
	return false;
}

bool BlockPhysicalLayer::Downward::onInit(void)
{
	ostringstream name;
	string link("up"); // we are on uplink
	string attenuation_type;

	// check if attenuation is enabled
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
										 ENABLE, this->attenuation))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot check if physical layer is enabled\n");
		goto error;
	}
	if(!this->attenuation)
		return true;

	// get refresh_period
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION], 
		               ACM_PERIOD_REFRESH,
	                   this->refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, ACM_PERIOD_REFRESH);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "refresh_period_ms = %d\n", this->refresh_period_ms);

	// Initiate Attenuation model
	if(!Conf::getValue(Conf::section_map[UPLINK_PHYSICAL_LAYER_SECTION],
	                   ATTENUATION_MODEL_TYPE,
	                   attenuation_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    UPLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	// Initiate Clear Sky value
	if(!Conf::getValue(Conf::section_map[UPLINK_PHYSICAL_LAYER_SECTION],
	                   CLEAR_SKY_CONDITION,
	                   this->clear_sky_condition))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    UPLINK_PHYSICAL_LAYER_SECTION, ATTENUATION_MODEL_TYPE);
		goto error;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins(attenuation_type,
	                                    "", "",
	                                    &this->attenuation_model,
	                                    NULL, NULL))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting physical layer plugins");
		goto error;
	}
	if(!this->attenuation_model->init(this->refresh_period_ms, link))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize attenuation model plugin %s",
		    attenuation_type.c_str());
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "%slink: attenuation model = %s, clear_sky condition = %u",
	    link.c_str(), attenuation_type.c_str(),
	    this->clear_sky_condition);

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->refresh_period_ms);

	this->probe_attenuation = Output::registerProbe<float>("dB", true,
	                                                       SAMPLE_LAST,
	                                                       "Phy.%slink_attenuation (%s)",
	                                                       link.c_str(),
	                                                       attenuation_type.c_str());
	this->probe_clear_sky_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.%slink_clear_sky_condition",
	                                                             link.c_str());

	return true;

error:
	return false;

}


bool BlockPhysicalLayer::Upward::processAttenuation(DvbFrame *dvb_frame)
{
	double cn_total;

	if(!IS_DATA_FRAME(dvb_frame->getMessageType()))
	{
		// do not handle signalisation but forward it
		return true;
	}

	// Update of the Threshold CN if Minimal Condition
	// Mde is Modcod dependent
	if(!this->updateMinimalCondition(dvb_frame))
	{
		// debug because it will be very verbose
		LOG(this->log_send, LEVEL_INFO,
		    "Error in Update of Minimal Condition\n");
		goto error;
	}

	LOG(this->log_send, LEVEL_DEBUG,
	    "Received DVB frame on carrier %u: C/N = %.2f\n",
	    dvb_frame->getCarrierId(),
	    dvb_frame->getCn());

	if(this->is_sat)
	{
		cn_total = dvb_frame->getCn();
		this->probe_total_cn->put(cn_total);
	}
	else
	{
		cn_total = this->getTotalCN(dvb_frame);
	}
	LOG(this->log_send, LEVEL_INFO,
	    "Total C/N: %.2f dB\n", cn_total);
	// Checking if the received frame must be affected by errors
	if(this->isToBeModifiedPacket(cn_total))
	{
		// Insertion of errors if necessary
		this->modifyPacket(dvb_frame);
	}

	return true; 
error:
	return false;
}



bool BlockPhysicalLayer::Downward::processAttenuation(DvbFrame *dvb_frame)
{
	if(!IS_DATA_FRAME(dvb_frame->getMessageType()))
	{
		// do not handle signalisation
		return true;
	}

	// Case of outgoing msg: Mark the msg with the C/N of Channel
	if(this->is_sat)
	{
		// we only need physical parameters for factorization on the receving side
		// that is the same for transparent mode
		// we use a very high value for unused C/N as it won't change anything
		dvb_frame->setCn(0x0fff);
	}
	else
	{
		this->addSegmentCN(dvb_frame);
	}

	LOG(this->log_send, LEVEL_DEBUG, 
	    "Send DVB frame on carrier %u: C/N  = %.2f\n",
	    dvb_frame->getCarrierId(),
	    dvb_frame->getCn());
	
	return true;
}


bool BlockPhysicalLayer::Downward::handleFifoTimer()
{
	DelayFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// Get all elements in FIFO ready to be sent
	while(((unsigned long)this->delay_fifo.getTickOut()) <= current_time &&
	      this->delay_fifo.getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;

		elem = this->delay_fifo.pop();
		assert(elem != NULL);

		dvb_frame = elem->getElem<DvbFrame>();

		// send the DVB frame to lower block
		if(!this->enqueueMessage((void **)&dvb_frame))
		{
			LOG(this->log_send, LEVEL_ERROR, 
					"failed to send burst of packets to lower layer\n");
			delete dvb_frame;
			goto error;
		}
		delete elem; // ?
	}
	return true;
error:
	return false;
}

bool BlockPhysicalLayer::Upward::handleFifoTimer()
{
	DelayFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// Get all elements in FIFO ready to be sent
	while(((unsigned long)this->delay_fifo.getTickOut()) <= current_time &&
	      this->delay_fifo.getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;

		elem = this->delay_fifo.pop();
		assert(elem != NULL);

		dvb_frame = elem->getElem<DvbFrame>();

		// TODO: if other components have to be added to physical layer,
		// think about using an array of pointer to functions, and iterate
		// over this array. The last function should be processDelay, since it 
		// queues the dvb_frame to the delay FIFO, and we lose hold of the 
		// dvb_frame.
		if(this->attenuation)
		{
			if(!this->processAttenuation(dvb_frame))
			{
				LOG(this->log_send, LEVEL_ERROR, 
						"failed to process attenuation to dvb frame\n");
				delete dvb_frame;
				goto error;
			}
		}
		// transmit the packet to the upper layer
		if(!this->enqueueMessage((void **)&dvb_frame))
		{
			LOG(this->log_send, LEVEL_ERROR,
					"failed to send burst of packets to upper layer\n");
			delete dvb_frame;
			goto error;
		}
		delete elem; // ???
	}
	return true;
error:
	delete elem; // ????
	return false;
}

bool BlockPhysicalLayer::Upward::setSatDelay(SatDelayPlugin *satdelay, bool update)
{
  vol_pkt_t max_size;
  time_ms_t fifo_timer_period;

  this->satdelay = satdelay;

  // TODO: FIFO could be created here, instead than on constructor
  // Configure FIFO size
  if(!Conf::getValue(Conf::section_map[ADV_SECTION],
                     DELAY_BUFFER, max_size))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "cannot get '%s' value", DELAY_BUFFER);
    goto error;
  }
  this->delay_fifo.setMaxSize(max_size);
  // Init FIFO timer
  if(!Conf::getValue(Conf::section_map[ADV_SECTION],
                     DELAY_TIMER, fifo_timer_period))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "cannot get '%s' value", DELAY_TIMER);
    goto error;
  }
  this->fifo_timer = this->addTimerEvent("fifo_timer", fifo_timer_period);

  if(!update)
    return true;

  // If this channel must update delay, add timer
  this->delay_timer = this->addTimerEvent("delay_timer",
                                          this->satdelay->getRefreshPeriod());
  // Add probe
  this->probe_delay = Output::registerProbe<int>("Delay", "ms", true, SAMPLE_LAST);

  return true;
error:
  return false;
}

bool BlockPhysicalLayer::Downward::setSatDelay(SatDelayPlugin *satdelay, bool update)
{
  vol_pkt_t max_size;
  time_ms_t fifo_timer_period;

  this->satdelay = satdelay;

  // TODO: FIFO could be created here, instead than on constructor
  // Configure FIFO size
  if(!Conf::getValue(Conf::section_map[ADV_SECTION],
                     DELAY_BUFFER, max_size))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "cannot get '%s' value", DELAY_BUFFER);
    goto error;
  }
  this->delay_fifo.setMaxSize(max_size);
  // Init FIFO timer
  if(!Conf::getValue(Conf::section_map[ADV_SECTION],
                     DELAY_TIMER, fifo_timer_period))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "cannot get '%s' value", DELAY_TIMER);
    goto error;
  }
  this->fifo_timer = this->addTimerEvent("fifo_timer", fifo_timer_period);

  if(!update)
    return true;

  // If this channel must update delay, add timer
  this->delay_timer = this->addTimerEvent("delay_timer",
                                          this->satdelay->getRefreshPeriod());
  // Add probe
  this->probe_delay = Output::registerProbe<int>("Delay", "ms", true, SAMPLE_LAST);

  return true;
error:
  return false;
}


bool BlockPhysicalLayerSat::Upward::onInit(void)
{
	ostringstream name;

	string minimal_type;
	string error_type;

	this->is_sat = true;

	// check if attenuation is enabled
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
										 ENABLE, this->attenuation))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot check if physical layer is enabled\n");
		return false;
	}
	if(!this->attenuation)
		return true;

	// Initiate Minimal conditions
	if(!Conf::getValue(Conf::section_map[SAT_PHYSICAL_LAYER_SECTION],
	                   MINIMAL_CONDITION_TYPE,
	                   minimal_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SAT_PHYSICAL_LAYER_SECTION, MINIMAL_CONDITION_TYPE);
		return false;
	}

	// Initiate Error Insertion
	if(!Conf::getValue(Conf::section_map[SAT_PHYSICAL_LAYER_SECTION],
	            ERROR_INSERTION_TYPE,
	            error_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SAT_PHYSICAL_LAYER_SECTION, ERROR_INSERTION_TYPE);
		return false;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins("",
	                                    minimal_type,
	                                    error_type,
	                                    &this->attenuation_model,
	                                    &this->minimal_condition,
	                                    &this->error_insertion))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "error when getting physical layer plugins");
		return false;
	}
	
	LOG(this->log_init, LEVEL_NOTICE,
	    "uplink: minimal condition type = %s, error insertion "
	    "type = %s", minimal_type.c_str(), error_type.c_str());

	if(!this->minimal_condition->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize minimal condition plugin %s",
		    minimal_type.c_str());
		return false;
	}

	if(!this->error_insertion->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot initialize error insertion plugin %s",
		    error_type.c_str());
		return false;
	}

	this->probe_minimal_condition = Output::registerProbe<float>("dB", true,
	                                                             SAMPLE_MAX,
	                                                             "Phy.minimal_condition (%s)",
	                                                             minimal_type.c_str());
	// TODO these probes are not really relevant as we should get probes per source
	//      terminal
	this->probe_drops = Output::registerProbe<int>("Phy.drops",
	                                               "frame number", true,
	                                               // we need to sum the drops here !
	                                               SAMPLE_SUM);
	this->probe_total_cn = Output::registerProbe<float>("dB", true,
	                                                    SAMPLE_MAX,
	                                                    "Phy.uplink_total_cn");

	return true;

}


bool BlockPhysicalLayerSat::Downward::onInit(void)
{
	this->is_sat = true;
	// check if attenuation is enabled
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
										 ENABLE, this->attenuation))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot check if physical layer is enabled\n");
		return false;
	}
	if(!this->attenuation)
		return true;

	return true;
}



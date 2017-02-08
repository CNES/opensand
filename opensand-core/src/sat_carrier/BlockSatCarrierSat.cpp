/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 * @file BlockSatCarrierSat.cpp
 * @brief This bloc implements a satellite carrier emulation.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "BlockSatCarrierSat.h"

#include <opensand_output/Output.h>

#include "DvbFrame.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"
#include "SatCarrierFifoElement.h"

/**
 * Constructor
 */
BlockSatCarrierSat::BlockSatCarrierSat(const string &name,
                                 struct sc_specific UNUSED(specific)):
	Block(name),
	sat_delay_map()
{
}

BlockSatCarrierSat::~BlockSatCarrierSat()
{
}

bool BlockSatCarrierSat::Downward::handleFifoTimer()
{
	SatCarrierFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// Get all elements in FIFO ready to be sent
	while(((unsigned long)this->fifo.getTickOut()) <= current_time &&
	      this->fifo.getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;

		elem = this->fifo.pop();
		assert(elem != NULL);

		dvb_frame = elem->getElem<DvbFrame>();

		// send the DVB frame
		if(!this->sendFrame(dvb_frame))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to send message, drop the DVB frame");
			goto error;
		}
		LOG(this->log_receive, LEVEL_INFO,
		    "Frame sent");
		delete elem;
	}
	return true;
error:
	delete elem;
	return false;
}

bool BlockSatCarrierSat::Downward::sendFrame(DvbFrame *dvb_frame)
{
	if(!this->out_channel_set.send(dvb_frame->getCarrierId(),
																 dvb_frame->getData().c_str(),
																 dvb_frame->getTotalLength()))
	{
		LOG(this->log_receive, LEVEL_ERROR,
				"error when sending data\n");
		delete dvb_frame;
		return false;
	}
	delete dvb_frame;
	return true;
}

// TODO: factorize: Downward and Upward could share this code
bool BlockSatCarrierSat::Downward::pushInFifo(NetContainer *data, time_ms_t delay)
{
	SatCarrierFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// create a new FIFO element to store the packet
	elem = new SatCarrierFifoElement(data, current_time, current_time + delay);
	if(!elem)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot allocate FIFO element, drop data\n");
		goto error;
	}
	// append the data in the fifo
	if(!this->fifo.push(elem))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "FIFO is full: drop data\n");
		goto release_elem;
	}
	LOG(this->log_receive, LEVEL_NOTICE,
	    "%s data stored in FIFO (tick_in = %ld, tick_out = %ld)\n",
	    data->getName().c_str(), elem->getTickIn(), elem->getTickOut());
	return true;
release_elem:
	delete elem;
error:
	delete data;
	return false;
}

bool BlockSatCarrierSat::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();
			uint8_t msg_type = dvb_frame->getMessageType();

			LOG(this->log_receive, LEVEL_DEBUG,
			    "%u-bytes %s message event received\n",
			    dvb_frame->getMessageLength(),
			    event->getName().c_str());

			// push frame to FIFO to implement delay (except SOF)
			if(msg_type == MSG_TYPE_SOF)
			{
				return this->sendFrame(dvb_frame);
			}
			else
			{
				time_ms_t delay;
				if(this->sat_delay_map->getDelayOut(dvb_frame->getCarrierId(),
				                                    msg_type, delay))
				{
					return this->pushInFifo((NetContainer *)dvb_frame, delay);
				}
				else
				{
					return false;
				}
			}
		}
		break;
		case evt_timer:
		{
			if(*event == this->fifo_timer)
			{
				return this->handleFifoTimer();
			}
			else if(*event == this->delays_timer)
			{
				if(!this->sat_delay_map->updateSatDelays())
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "error when updating satellite delays");
					return false;
				}
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}
	return true;
}

bool BlockSatCarrierSat::Upward::sendFrame(DvbFrame *dvb_frame)
{
	if(!this->enqueueMessage((void **)(&dvb_frame)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send frame from carrier %u to upper layer\n",
		    dvb_frame->getCarrierId());
		goto release;
	}
	LOG(this->log_receive, LEVEL_DEBUG,
	    "Message from carrier %u sent to upper layer\n",
	    dvb_frame->getCarrierId());
	return true;
release:
	delete dvb_frame;
	return false;
}

bool BlockSatCarrierSat::Upward::handleFifoTimer()
{
	SatCarrierFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// Get all elements in FIFO ready to be sent
	while(((unsigned long)this->fifo.getTickOut()) <= current_time &&
	      this->fifo.getCurrentSize() > 0)
	{
		DvbFrame *dvb_frame;

		elem = this->fifo.pop();
		assert(elem != NULL);

		dvb_frame = elem->getElem<DvbFrame>();

		// send the DVB frame
		if(!this->sendFrame(dvb_frame))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to send message, drop the DVB frame");
			goto error;
		}
		LOG(this->log_receive, LEVEL_INFO,
		    "Frame sent");
		delete elem;
	}
	return true;
error:
	delete elem;
	return false;
}

bool BlockSatCarrierSat::Upward::onEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in Sat_Carrier socket buffer
			size_t length;
			unsigned char *buf = NULL;

			unsigned int carrier_id;
			spot_id_t spot_id;
			int ret;

			LOG(this->log_receive, LEVEL_DEBUG,
			    "FD event received\n");

			// for UDP we need to retrieve potentially desynchronized
			// datagrams => loop on receive function
			do
			{
				ret = this->in_channel_set.receive((NetSocketEvent *)event,
				                                    carrier_id,
				                                    spot_id,
				                                    &buf, length);
				if(ret < 0)
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to receive data on any "
					    "input channel (code = %zu)\n", length);
					status = false;
				}
				else
				{
					LOG(this->log_receive, LEVEL_DEBUG,
					    "%zu bytes of data received on carrier ID %u\n",
					    length, carrier_id);

					if(length > 0)
					{
						this->onReceivePktFromCarrier(carrier_id, spot_id,  
						                              buf, length);
					}
				}
			} while(ret > 0);
		}
		break;
		case evt_timer:
		{
			if(*event == this->fifo_timer)
			{
				status = this->handleFifoTimer();
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s\n", 
			    event->getName().c_str());
			return false;
	}

	return status;
}


bool BlockSatCarrierSat::onInit(void)
{
	if(!this->sat_delay_map.init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to init satellite delays map");
		goto error;
	}
	// share the map to channels
	((Upward *)this->upward)->setSatDelay(&this->sat_delay_map);
	((Downward *)this->downward)->setSatDelay(&this->sat_delay_map);
	return true;
error:
	return false;
}

bool BlockSatCarrierSat::Upward::onInit(void)
{
	vector<sat_carrier_udp_channel *>::iterator it;
	sat_carrier_udp_channel *channel;
	vol_pkt_t max_size;
	time_ms_t fifo_timer_period;

	// initialize all channels from the configuration file
	if(!this->in_channel_set.readInConfig(this->ip_addr,
	                                      this->interface_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(it = this->in_channel_set.begin(); it != this->in_channel_set.end(); it++)
	{
		channel = *it;

		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			ostringstream name;

			LOG(this->log_init, LEVEL_NOTICE,
			    "Listen on fd %d for channel %d\n",
			    channel->getChannelFd(), channel->getChannelID());
			name << "Channel_" << channel->getChannelID();
			this->addNetSocketEvent(name.str(),
			                        channel->getChannelFd(),
			                        MSG_BBFRAME_SIZE_MAX);
		}
	}
	// Configure FIFO size
	if(!Conf::getValue(Conf::section_map[ADV_SECTION],
		                 DELAY_BUFFER, max_size))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_BUFFER);
		goto error;
	}
	this->fifo.setMaxSize(max_size);
	// Init FIFO timer
	if(!Conf::getValue(Conf::section_map[ADV_SECTION],
		                 DELAY_TIMER, fifo_timer_period))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_TIMER);
		goto error;
	}
	this->fifo_timer = this->addTimerEvent("fifo_timer", fifo_timer_period);
	
	return true;
error:
	return false;
}

void BlockSatCarrierSat::Upward::setSatDelay(SatDelayMap *sat_delay_map)
{
	this->sat_delay_map = sat_delay_map;
}

bool BlockSatCarrierSat::Downward::onInit()
{
	vol_pkt_t max_size;
	time_ms_t fifo_timer_period;
	
	// initialize all channels from the configuration file
	if(!this->out_channel_set.readOutConfig(this->ip_addr,
	                                        this->interface_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Wrong channel set configuration\n");
		return false;
	}
	// Configure FIFO size
	if(!Conf::getValue(Conf::section_map[ADV_SECTION],
		                 DELAY_BUFFER, max_size))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_BUFFER);
		goto error;
	}
	this->fifo.setMaxSize(max_size);
	// Init FIFO timer
	if(!Conf::getValue(Conf::section_map[ADV_SECTION],
		                 DELAY_TIMER, fifo_timer_period))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", DELAY_TIMER);
		goto error;
	}
	this->fifo_timer = this->addTimerEvent("fifo_timer", fifo_timer_period);
	this->delays_timer = this->addTimerEvent("delays_timer",
	                                         this->sat_delay_map->getRefreshPeriod());
	
	return true;
error:
	return false;
}

void BlockSatCarrierSat::Downward::setSatDelay(SatDelayMap *sat_delay_map)
{
	this->sat_delay_map = sat_delay_map;
}

// TODO: factorize: Downward and Upward could share this code
bool BlockSatCarrierSat::Upward::pushInFifo(NetContainer *data, time_ms_t delay)
{
	SatCarrierFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// create a new FIFO element to store the packet
	elem = new SatCarrierFifoElement(data, current_time, current_time + delay);
	if(!elem)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot allocate FIFO element, drop data\n");
		goto error;
	}
	// append the data in the fifo
	if(!this->fifo.push(elem))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "FIFO is full: drop data\n");
		goto release_elem;
	}
	LOG(this->log_receive, LEVEL_NOTICE,
	    "%s data stored in FIFO (tick_in = %ld, tick_out = %ld)\n",
	    data->getName().c_str(), elem->getTickIn(), elem->getTickOut());
	return true;
release_elem:
	delete elem;
error:
	delete data;
	return false;
}

void BlockSatCarrierSat::Upward::onReceivePktFromCarrier(uint8_t carrier_id,
                                                      spot_id_t spot_id,
                                                      unsigned char *data,
                                                      size_t length)
{
	DvbFrame *dvb_frame = new DvbFrame(data, length);
	uint8_t msg_type = dvb_frame->getMessageType();
	free(data);

	dvb_frame->setCarrierId(carrier_id);
	dvb_frame->setSpot(spot_id);
	
	// push frame to FIFO to implement delay (except SOF)
	if(msg_type == MSG_TYPE_SOF)
	{
		this->sendFrame(dvb_frame);
	}
	else
	{
		// TODO: maybe pushInFifo, if delay==0, could do the sendFrame directly
		// (getDelay returns 0 for SOF)
		time_ms_t delay;
		if(this->sat_delay_map->getDelayIn(carrier_id, msg_type, delay))
		{
			this->pushInFifo((NetContainer *)dvb_frame, delay);
		}
	}
	return;
}

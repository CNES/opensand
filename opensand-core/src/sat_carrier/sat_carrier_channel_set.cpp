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
 * @file sat_carrier_channel_set.cpp
 * @brief This implements a set of satellite carrier channels
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_SAT_CARRIER
#include <opensand_conf/uti_debug.h>

#include "sat_carrier_channel_set.h"


/**
 * Create an empty set of satellite carrier channels
 */
sat_carrier_channel_set::sat_carrier_channel_set():
	std::vector < sat_carrier_channel * >()
{
}

sat_carrier_channel_set::~sat_carrier_channel_set()
{
	std::vector < sat_carrier_channel * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
		delete(*it);
}


/**
 * Read data from the configuration file and create channels
 * @return -1 if failed, 0 if succeed
 */
int sat_carrier_channel_set::readConfig(component_t host,
                                        const string local_ip_addr,
                                        const string interface_name)
{
	string strConfig;

	int i;
	sat_carrier_channel *channel;
	ConfigurationList carrier_list;
	ConfigurationList::iterator iter;

	// get transmission type
	if(!globalConfig.getValue(SATCAR_SECTION, SOCKET_TYPE,
	                          this->socket_type))
	{
		UTI_ERROR("Can't get socket type from section %s, %s\n",
		          SATCAR_SECTION, SOCKET_TYPE);
		goto error;
	}

	// get satellite channels from configuration
	if(!globalConfig.getListItems(SATCAR_SECTION, CARRIER_LIST, carrier_list))
	{
		UTI_ERROR("section '%s, %s': missing satellite channels\n",
		          SATCAR_SECTION, CARRIER_LIST);
		goto error;
	}

	i = 0;
	for(iter = carrier_list.begin(); iter != carrier_list.end(); iter++)
	{
		int carrier_id;
		long carrier_port;
		bool carrier_up;
		bool carrier_down;
		bool carrier_multicast;
		string carrier_ip;
		string carrier_disabled;

		i++;
		// get carrier ID
		if(!globalConfig.getAttributeValue(iter, CARRIER_ID, carrier_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_ID, i);
			goto error;
		}
		// get IP address
		if(!globalConfig.getAttributeValue(iter, CARRIER_IP, carrier_ip))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_IP, i);
			goto error;
		}

		// get port
		if(!globalConfig.getAttributeValue(iter, CARRIER_PORT, carrier_port))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_PORT, i);
			goto error;
		}
		// get up
		if(!globalConfig.getAttributeValue(iter, CARRIER_UP, carrier_up))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_UP, i);
			goto error;
		}
		// get down
		if(!globalConfig.getAttributeValue(iter, CARRIER_DOWN, carrier_down))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_DOWN, i);
			goto error;
		}
		// get multicast
		if(!globalConfig.getAttributeValue(iter, CARRIER_MULTICAST,
		                                   carrier_multicast))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_MULTICAST, i);
			goto error;
		}
		// get disabled_on
		if(!globalConfig.getAttributeValue(iter, CARRIER_DISABLED,
		                                   carrier_disabled))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_DISABLED, i);
			goto error;
		}

		if(carrier_disabled.c_str() == getComponentName(host))
		{
			continue;
		}

		UTI_DEBUG("Line: %d, Carrier ID: %u, IP address: %s, "
		          "port: %ld, up: %s, down: %s, multicast: %s, "
		          "disabled on: %s\n",
		          i, carrier_id, carrier_ip.c_str(),
		          carrier_port, (carrier_up ? "true" : "false"),
		          (carrier_down ? "true" : "false"),
		          (carrier_multicast ? "true" : "false"),
		          carrier_disabled.c_str());

		// if for a a channel in=false and out=false channel is not active
		if(carrier_down || carrier_up)
		{
			if(this->socket_type == UDP)
			{
				// create a new udp channel configure it, with information from file
				// and insert it in the channels vector
				channel = new sat_carrier_udp_channel(carrier_id,
				                                      (host == satellite) ?
				                                            carrier_up : carrier_down,
				                                      (host == satellite) ?
				                                            carrier_down : carrier_up,
				                                      interface_name,
				                                      carrier_port,
				                                      carrier_multicast,
				                                      local_ip_addr,
				                                      carrier_ip);
				if(!channel->isInit())
				{
					UTI_ERROR("failed to create UDP channel %d\n", i);
					goto error;
				}
				this->push_back(channel);
			}
			else
			{
				UTI_ERROR("Wrong socket type: %s\n", this->socket_type.c_str());
				goto error;
			}
		}
	}

	return 0;

error:
	return -1;
}


/**
 * Send a variable length buffer on the specified satellite Carrier.
 *
 * @param channel  Satellite Carrier id
 * @param buf      pointer to a char buffer
 * @param len      length of the buffer
 * @return         the size of sent data if successful, -1 otherwise
 */
int sat_carrier_channel_set::send(unsigned int channel,
                                  unsigned char *buf,
                                  unsigned int len)
{
	std::vector < sat_carrier_channel * >::iterator it;
	int ret = -1;

	for(it = this->begin(); it != this->end(); it++)
	{
		if(channel == (*it)->getChannelID() && (*it)->isOutputOk())
		{
			ret = (*it)->send(buf, len);
			break;
		}
	}

	if(it == this->end())
	{
		UTI_ERROR("failed to send %u bytes of data through channel %d: "
		          "channel not found\n", len, channel);
	}

	return ret;
}


int sat_carrier_channel_set::receive(NetSocketEvent *const event,
                                     unsigned int &op_carrier,
                                     unsigned char **op_buf,
                                     size_t &op_len)
{
	int ret = -1;
	std::vector < sat_carrier_channel * >::iterator it;

	op_len = 0;

	UTI_DEBUG_L3("try to receive a packet from satellite channel "
	             "associated with the file descriptor %d\n", event->getFd());

	for(it = this->begin(); it != this->end(); it++)
	{
		// does the channel accept input and does the channel file descriptor
		// match with the given file descriptor?
		if((*it)->isInputOk() && *event == (*it)->getChannelFd())
		{
			// the file descriptors match, try to receive data for the channel
			ret = (*it)->receive(event, op_buf, op_len);

			// Stop the task on data or error
			if(op_len != 0 || ret < 0)
			{
				UTI_DEBUG_L3("data/error received, set op_carrier to %d\n",
				             (*it)->getChannelID());
				op_carrier = (*it)->getChannelID();
				break;
			}
		}
	}
	UTI_DEBUG_L3("Receive packet: size %zu, carrier %d\n", op_len, op_carrier);

	if(it == this->end())
		ret = 0;

	return ret;
}

/**
* Return the file descriptor coresponding to a channel
* This allow the caler to manage itself select() fuctions
* @param i_channel Channel number
* @return fd
*/
int sat_carrier_channel_set::getChannelFdByChannelId(unsigned int i_channel)
{
	std::vector < sat_carrier_channel * >::iterator it;
	int ret = -1;

	for(it = this->begin(); it != this->end(); it++)
	{
		if(i_channel == (*it)->getChannelID())
		{
			ret = (*it)->getChannelFd();
			break;
		}
	}

	if(ret < 0)
	{
		UTI_ERROR("SAT_Carrier_Get_Channel_Fd : Channel not found (%d) \n",
		          i_channel);
	}

	return (ret);
}

/**
 * Get the number of channels in the set
 * @return the number of channel
 */
unsigned int sat_carrier_channel_set::getNbChannel()
{
	return this->size();
}

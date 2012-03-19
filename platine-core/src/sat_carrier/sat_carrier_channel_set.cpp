/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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

#include "sat_carrier_channel_set.h"

#define DBG_PACKAGE PKG_SAT_CARRIER
#include "platine_conf/uti_debug.h"


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
int sat_carrier_channel_set::readConfig()
{
	string interfaceName;
	string strConfig;
	char localIPaddress[16];

	int i;
	sat_carrier_channel *channel;
	ConfigurationList carrier_list;
	ConfigurationList::iterator iter;

	if(!globalConfig.getStringValue(GLOBAL_SECTION, SAT_ETH_IFACE,
	                                interfaceName))
	{
		UTI_ERROR("Can't get satelliteEthInterface from section Global\n");
		goto error;
	}

	// get transmission type
	if(!globalConfig.getStringValue(SATCAR_SECTION, SOCKET_TYPE,
	                               this->socket_type))
	{
		UTI_ERROR("Can't get socket type from section %s, %s\n",
		          SATCAR_SECTION, SOCKET_TYPE);
		goto error;
	}

	// get local IP address
	if(globalConfig.getStringValue(SATCAR_SECTION, IPADDR, strConfig))
	{
		sscanf(strConfig.c_str(), "%15s", localIPaddress);
	}
	else
	{
		UTI_ERROR("Error can't get IP address from section : %s \n", SATCAR_SECTION);
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
		string carrier_ip;
		bool carrier_in;
		bool carrier_out;
		bool carrier_multicast;

		i++;
		// get carrier ID
		if(!globalConfig.getAttributeIntegerValue(iter, CARRIER_ID,
		                                          carrier_id))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_ID, i);
			goto error;
		}
		// get IP address
		if(!globalConfig.getAttributeStringValue(iter, CARRIER_IP,
		                                         carrier_ip))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_IP, i);
			goto error;
		}

		// get port
		if(!globalConfig.getAttributeLongIntegerValue(iter, CARRIER_PORT,
		                                              carrier_port))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_PORT, i);
			goto error;
		}
		// get in
		if(!globalConfig.getAttributeBoolValue(iter, CARRIER_IN,
		                                       carrier_in))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_IN, i);
			goto error;
		}
		// get out
		if(!globalConfig.getAttributeBoolValue(iter, CARRIER_OUT,
		                                       carrier_out))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_OUT, i);
			goto error;
		}
		// get multicast
		if(!globalConfig.getAttributeBoolValue(iter, CARRIER_MULTICAST,
		                                       carrier_multicast))
		{
			UTI_ERROR("section '%s, %s': failed to retrieve %s at "
			          "line %d\n", SATCAR_SECTION, CARRIER_LIST,
			          CARRIER_MULTICAST, i);
			goto error;
		}

		UTI_DEBUG("Line: %d, Carrier ID : %u, IP address: %s, "
		          "port: %ld, in : %s, out : %s, multicast: %s\n",
		          i, carrier_id, carrier_ip.c_str(),
		          carrier_port, (carrier_in ? "true" : "false"),
		          (carrier_out ? "true" : "false"),
		          (carrier_multicast ? "true" : "false"));

		// if for a a channel in=false and out=false channel is not active
		if(carrier_in || carrier_out)
		{
			if(this->socket_type == UDP)
			{
				// create a new udp channel configure it, with information from file
				// and insert it in the channels vector
				channel = new sat_carrier_udp_channel(carrier_id, carrier_in,
				                                      carrier_out,
				                                      interfaceName.c_str(),
				                                      carrier_port,
				                                      carrier_multicast,
				                                      localIPaddress,
				                                      carrier_ip.c_str());
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


/**
 * @brief Receive data on a channel set
 *
 * The function works in blocking mode, so call it only when you are sure
 * some data is ready to be received.
 *
 * @param fd            the file descriptor on which the event has been catched
 * @param op_carrier    Satellite Carrier id
 * @param op_buf        pointer to a char buffer
 * @param op_len        the received data length
 * @param op_max_len    length of the buffer
 * @param timeout_ms    the time out for the select function
 * @return
 */
int sat_carrier_channel_set::receive(int fd,
                                     unsigned int *op_carrier,
                                     unsigned char *op_buf,
                                     unsigned int *op_len,
                                     unsigned int op_max_len,
                                     long timeout_ms)
{
	int ret = -1;
	std::vector < sat_carrier_channel * >::iterator it;

	UTI_DEBUG_L3("try to receive a packet from satellite channel "
	             "associated with the file descriptor %d\n", fd);

	for(it = this->begin(); it != this->end(); it++)
	{
		// does the channel accept input and does the channel file descriptor
		// match with the given file descriptor?
		if((*it)->isInputOk() && fd == (*it)->getChannelFd())
		{
			// the file descriptors match, try to receive data for the channel
			ret = (*it)->receive(op_buf, op_len, op_max_len, timeout_ms);

			// received data must not be too large
			if(ret == 0 && *op_len > op_max_len)
			{
				UTI_ERROR("too much data received: %u bytes "
				          "received while only %u desired\n",
				          *op_len, op_max_len);
				ret = -1;
			}

			// Stop the task on data or error
			if(*op_len != 0 || ret < 0)
			{
				UTI_DEBUG_L3("data/error received, set op_carrier to %i\n",
				             (*it)->getChannelID());
				*op_carrier = (*it)->getChannelID();
				break;
			}
		}
	}

	UTI_DEBUG_L3("Receive packet: size %i, carrier %i\n", *op_len, *op_carrier);

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

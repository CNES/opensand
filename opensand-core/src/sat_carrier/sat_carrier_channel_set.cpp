/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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


#include "sat_carrier_channel_set.h"

#include <opensand_output/Output.h>


/**
 * Create an empty set of satellite carrier channels
 */
sat_carrier_channel_set::sat_carrier_channel_set(tal_id_t tal_id):
	std::vector < sat_carrier_udp_channel * >(),
	tal_id(tal_id)
{
	this->log_init = Output::registerLog(LEVEL_WARNING, "SatCarrier.init");
	this->log_sat_carrier = Output::registerLog(LEVEL_WARNING,
	                                            "SatCarrier.Channel");
}

sat_carrier_channel_set::~sat_carrier_channel_set()
{
	std::vector < sat_carrier_udp_channel * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
		delete(*it);
}


bool sat_carrier_channel_set::readConfig(const string local_ip_addr,
                                         const string interface_name,
                                         bool in)
{

	int i = 0;
	ConfigurationList spot_list;
	ConfigurationList::iterator iter;
	ConfigurationList::iterator iter_spots;

	// get satellite channels from configuration
	if(!Conf::getListNode(Conf::section_map[SATCAR_SECTION], SPOT_LIST,
	                      spot_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, SPOT_LIST);
		goto error;
	}

	for(iter_spots = spot_list.begin(); iter_spots != spot_list.end();
	    ++iter_spots)
	{
		string compo_name = "";
		component_t host = unknown_compo;
		ConfigurationList current_spot;
		ConfigurationList carrier_list ; 
		xmlpp::Node* spot_node = *iter_spots;
		current_spot.push_front(spot_node);
		spot_id_t spot_id = 0;
		Conf::getAttributeValue(iter_spots, SPOT_ID, spot_id);

		// get satellite channels from configuration
		if(!Conf::getListItems(current_spot, CARRIER_LIST, carrier_list))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': missing satellite channels\n",
			    SATCAR_SECTION, CARRIER_LIST);
			goto error;
		}

		// get host type
		if(!Conf::getComponent(compo_name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get component type\n");
			goto error;
		}
		LOG(this->log_init, LEVEL_INFO,
		    "host type = %s\n", compo_name.c_str());
		host = getComponentType(compo_name);

		// TODO why not first get the spot according to terminal id
		//      this will avoid doing all og this for each spot
		//      - first get the spot id
		//      - then call a function that return a list of spot with
		//        optionnal spot_id, if spot_id the list will contain only one
		//        spot but the treatment will be the same
		if(host == terminal)
		{
			// terminal is not in the current spot, continue
			if(Conf::terminal_map[this->tal_id] != spot_id)
			{
				continue;
			}
		}

		for(iter = carrier_list.begin(); iter != carrier_list.end(); iter++)
		{
			sat_carrier_udp_channel *channel;
			string strConfig;
			int carrier_id = 0;
			long carrier_port = 0;
			bool carrier_up = false;
			bool carrier_down = false;
			string carrier_type;
			bool is_input = false;
			bool is_output = false;
			bool carrier_multicast = false;
			string carrier_ip("");
			string carrier_disabled("");

			i++;

			// get carrier ID
			if(!Conf::getAttributeValue(iter, CARRIER_ID, carrier_id))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s at "
				    "line %d\n", SPOT_LIST, spot_id,
				    SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_ID, i);
				goto error;
			}
			
			// get IP address
			if(!Conf::getAttributeValue(iter, CARRIER_IP, carrier_ip))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s at "
				    "line %d\n", SPOT_LIST, spot_id,
				    SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_IP, i);
				goto error;
			}

			// get port
			if(!Conf::getAttributeValue(iter, CARRIER_PORT, carrier_port))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s at "
				    "line %d\n", SPOT_LIST, spot_id,
				    SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_PORT, i);
				goto error;
			}
	
			// get type
			if(!Conf::getAttributeValue(iter, CARRIER_TYPE, carrier_type))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s at "
				    "line %d\n",SPOT_LIST, spot_id,
				    SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_TYPE, i);
				goto error;
			}

			// get up and down
			if(carrier_type.find("in") != std::string::npos)
			{
				carrier_up = true;
			}
			else if(carrier_type.find("out") != std::string::npos)
			{
				carrier_down = true;
			}
			is_input = (host == satellite) ? carrier_up : carrier_down;
			is_output = (host == satellite) ? carrier_down : carrier_up;
			if((in && !is_input) || (!in && !is_output))
			{
				continue;
			}
			
			
			// get Disabled
			if(strcmp(carrier_type.c_str(), LOGON_IN) == 0)
			{
				carrier_disabled = DISABLED_GW;
			}
			else if(strcmp(carrier_type.c_str(), LOGON_OUT) == 0)
			{
				carrier_disabled = DISABLED_ST;
			}
			else
			{
				carrier_disabled = DISABLED_NONE;
			}
			
			if(carrier_disabled == compo_name)
			{
				continue;
			}

			// get multicast
			if(!Conf::getAttributeValue(iter, CARRIER_MULTICAST,
			                             carrier_multicast))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s at "
				    "line %d\n", SPOT_LIST, spot_id,
				    SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_MULTICAST, i);
				goto error;
			}

			LOG(this->log_init, LEVEL_INFO,
			    "Line: %d, Carrier ID: %u, IP address: %s, "
			    "port: %ld, up: %s, down: %s, multicast: %s, "
			    "disabled on: %s\n",
			    i, carrier_id, carrier_ip.c_str(),
			    carrier_port, (carrier_up ? "true" : "false"),
			    (carrier_down ? "true" : "false"),
			    (carrier_multicast ? "true" : "false"),
			    carrier_disabled.c_str());

			// if for a a channel in=false and out=false channel is not active
			if(is_input || is_output)
			{
				unsigned int stack;
				unsigned int rmem;
				unsigned int wmem;

				// get UDP stack
				if(!Conf::getValue(Conf::section_map[ADV_SECTION], 
				   UDP_STACK, stack))
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "Section %s, %s %d, %s missing\n",
					    ADV_SECTION, SPOT_LIST, spot_id, UDP_STACK);
					goto error;
				}
				// get rmem
				if(!Conf::getValue(Conf::section_map[ADV_SECTION],
				                   UDP_RMEM, rmem))
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "Section %s, %s %d, %s missing\n",
					    ADV_SECTION, SPOT_LIST, spot_id, UDP_RMEM);
					goto error;
				}
				// get wmem
				if(!Conf::getValue(Conf::section_map[ADV_SECTION],
				                   UDP_WMEM, wmem))
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "Section %s, %s %d, %s missing\n",
					    ADV_SECTION, SPOT_LIST, spot_id, UDP_WMEM);
					goto error;
				}
				// create a new udp channel configure it, with information from file
				// and insert it in the channels vector
				channel = new sat_carrier_udp_channel(spot_id,
				                                      carrier_id,
				                                      is_input,
				                                      is_output,
				                                      interface_name,
				                                      carrier_port,
				                                      carrier_multicast,
				                                      local_ip_addr,
				                                      carrier_ip,
				                                      stack, rmem, wmem);
				
				
				
				if(!channel->isInit())
				{
					LOG(this->log_init, LEVEL_ERROR,
					    "failed to create UDP channel %d\n", i);
					goto error;
				}
				this->push_back(channel);
			}
		}
	}

	return true;

error:
	return false;
}

bool sat_carrier_channel_set::readInConfig(const string local_ip_addr,
                                           const string interface_name)
{
	return this->readConfig(local_ip_addr, interface_name, true);
}

bool sat_carrier_channel_set::readOutConfig(const string local_ip_addr,
                                            const string interface_name)
{
	return this->readConfig(local_ip_addr, interface_name, false);
}

bool sat_carrier_channel_set::send(uint8_t carrier_id,
                                   const unsigned char *data,
                                   size_t length)
{
	std::vector <sat_carrier_udp_channel *>::const_iterator it;
	bool status;

	for(it = this->begin(); it != this->end(); ++it)
	{
		if(carrier_id == (*it)->getChannelID() && (*it)->isOutputOk())
		{
			if((*it)->send(data, length))
			{
				status = true;
			}
			break;
		}
	}

	if(it == this->end())
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "failed to send %zu bytes of data through channel %u: "
		    "channel not found\n", length, carrier_id);
	}

	return status;
}


int sat_carrier_channel_set::receive(NetSocketEvent *const event,
                                     unsigned int &op_carrier,
                                     spot_id_t &op_spot,
                                     unsigned char **op_buf,
                                     size_t &op_len)
{
	int ret = -1;
	std::vector < sat_carrier_udp_channel * >::iterator it;

	op_len = 0;
	op_carrier = 0;

	LOG(this->log_sat_carrier, LEVEL_DEBUG,
	    "try to receive a packet from satellite channel "
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
				LOG(this->log_sat_carrier, LEVEL_DEBUG,
				    "data/error received, set op_carrier to %d\n",
				    (*it)->getChannelID());
				op_carrier = (*it)->getChannelID();
				op_spot = (*it)->getSpotId();
				break;
			}
		}
	}
	LOG(this->log_sat_carrier, LEVEL_DEBUG,
	    "Receive packet: size %zu, carrier %d\n", op_len,
	    op_carrier);

/*	if(it == this->end())
		ret = 0;*/

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
	std::vector < sat_carrier_udp_channel * >::iterator it;
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
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "SAT_Carrier_Get_Channel_Fd : Channel not "
		    "found (%d) \n", i_channel);
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



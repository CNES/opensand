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
	string compo_name = "";
	component_t host = unknown_compo;
	ConfigurationList spot_list;
	ConfigurationList::iterator iter;
	ConfigurationList::iterator iter_spots;
	
	// get host type
	if(!Conf::getComponent(compo_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot get component type\n");
		goto error;
	}
	host = getComponentType(compo_name);

	// get satellite channels from configuration
	if(!Conf::getListNode(Conf::section_map[SATCAR_SECTION], SPOT_LIST,
				spot_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"section '%s, %s': missing satellite channels\n",
				SATCAR_SECTION, SPOT_LIST);
		goto error;
	}

	// TODO why not first get the spot according to terminal id
	//      this will avoid doing all og this for each spot
	//      - first get the spot id
	//      - then call a function that return a list of spot with
	//        optionnal spot_id, if spot_id the list will contain only one
	//        spot but the treatment will be the same
	// for terminal get the corresponding spot
	if(host == terminal)
	{
		ConfigurationList temp_spot;
		ConfigurationList temp_gw;
		spot_id_t spot_id;
		tal_id_t gw_id;

		if(Conf::spot_table.find(this->tal_id) == Conf::spot_table.end())
		{
			if(!Conf::getValue(Conf::section_map[SPOT_TABLE_SECTION], 
						       DEFAULT_SPOT, spot_id))
			{
				LOG(this->log_init, LEVEL_ERROR, 
						"couldn't find spot for tal %d", 
						this->tal_id);
				goto error;
			}
		}
		else
		{
			spot_id = Conf::spot_table[this->tal_id];
		}

		if(Conf::gw_table.find(this->tal_id) == Conf::gw_table.end())
		{
			if(!Conf::getValue(Conf::section_map[GW_TABLE_SECTION], 
						       DEFAULT_GW, gw_id))
			{
				LOG(this->log_init, LEVEL_ERROR, 
						"couldn't find gw for tal %d", 
						tal_id);
				goto error;
			}
		}
		else
		{
			gw_id = Conf::gw_table[this->tal_id];
		}
		
		if(!Conf::getElementWithAttributeValue(spot_list, ID, spot_id, temp_spot))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "couldn't get spot %d into %s/%s",
			    spot_id, SATCAR_SECTION, SPOT_LIST);
			goto error;
		}

		if(!Conf::getElementWithAttributeValue(temp_spot, GW, gw_id, temp_gw))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "couldn't get spot %d gw %d into %s/%s",
			    spot_id, gw_id, SATCAR_SECTION, SPOT_LIST);
			goto error;
		}

		spot_list = temp_gw;
	}
	else if(host == gateway)
	{
		ConfigurationList temp_gw;

		if(!Conf::getElementWithAttributeValue(spot_list, GW, this->tal_id, temp_gw))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "couldn't get spot for gw %d into %s/%s",
			    this->tal_id, SATCAR_SECTION, SPOT_LIST);
			goto error;
		}

		spot_list = temp_gw;
	}
	
	for(iter_spots = spot_list.begin(); iter_spots != spot_list.end();
	    ++iter_spots)
	{
		ConfigurationList carrier_list ; 
		spot_id_t spot_id = 0;
		
		if(!Conf::getAttributeValue(iter_spots, ID, spot_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "there is not attribute %s in %s/%s",
			    ID, SATCAR_SECTION, SPOT_LIST);
			goto error;
		}

		// get satellite channels from configuration
		if(!Conf::getListItems(*iter_spots, CARRIER_LIST, carrier_list))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': missing satellite channels\n",
			    SATCAR_SECTION, CARRIER_LIST);
			goto error;
		}

		LOG(this->log_init, LEVEL_INFO,
		    "host type = %s\n", compo_name.c_str());
		
		// get all carrier from this spot
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
	bool status =false;

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



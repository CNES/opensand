/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file sat_carrier_channel_set.cpp
 * @brief This implements a set of satellite carrier channels
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#include "sat_carrier_channel_set.h"

#include "OpenSandModelConf.h"
#include <opensand_output/Output.h>


/**
 * Create an empty set of satellite carrier channels
 */
sat_carrier_channel_set::sat_carrier_channel_set(tal_id_t tal_id):
	std::vector < UdpChannel * >(),
	tal_id(tal_id)
{
	auto output = Output::Get();
	this->log_init = output->registerLog(LEVEL_WARNING, "SatCarrier.init");
	this->log_sat_carrier = output->registerLog(LEVEL_WARNING, "SatCarrier.Channel");
}

sat_carrier_channel_set::~sat_carrier_channel_set()
{
	std::vector < UdpChannel * >::iterator it;

	for(it = this->begin(); it != this->end(); it++)
		delete(*it);
}


bool sat_carrier_channel_set::readCarrier(const string &local_ip_addr,
                                          tal_id_t gw_id,
                                          const OpenSandModelConf::carrier_socket &carrier,
                                          bool is_input)
{
	int carrier_id = carrier.id;
	long carrier_port = carrier.port;
	bool carrier_multicast = carrier.is_multicast;
	string carrier_ip = carrier.address;
	
	LOG(this->log_init, LEVEL_INFO,
	    "Creating carrier with ID: %u, IP address: %s, "
	    "port: %ld, input: %s, multicast: %s\n",
	    carrier_id, carrier_ip.c_str(),
	    carrier_port,
	    (is_input ? "true" : "false"),
	    (carrier_multicast ? "true" : "false"));

	// create a new udp channel configure it, with information from file
	// and insert it in the channels vector
	UdpChannel *channel = new UdpChannel("SatCarrier",
	                                     gw_id,
	                                     carrier_id,
	                                     is_input,
	                                     !is_input,
	                                     carrier_port,
	                                     carrier_multicast,
	                                     local_ip_addr,
	                                     carrier_ip,
	                                     carrier.udp_stack,
	                                     carrier.udp_rmem,
	                                     carrier.udp_wmem);

	if(!channel->isInit())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create UDP channel %d\n", carrier_id);
		delete channel;
		return false;
	}
	this->push_back(channel);

	return true;
}

bool sat_carrier_channel_set::readSpot(const string &local_ip_addr,
                                       bool in,
                                       Component host,
                                       tal_id_t gw_id,
                                       bool is_satellite)
{
	auto Conf = OpenSandModelConf::Get();
	OpenSandModelConf::spot_infrastructure carriers;
	if(!Conf->getSpotInfrastructure(gw_id, carriers))
	{
		LOG(this->log_init, LEVEL_ERROR,
			"couldn't create spot infrastructure for gw %d",
			gw_id);
		return false;
	}

	std::string config_string;
	config_string = getComponentName(host);
	if (is_satellite) {
		config_string = "sat (" + config_string + " side)";
	}
	
	LOG(this->log_init, LEVEL_INFO,
	    "Creating carriers for %s\n", config_string.c_str());
	
	bool create_in_carriers = in && is_satellite || !in && !is_satellite;

	switch (host)
	{
		case Component::terminal:
			if (create_in_carriers) 
			{
				if (!readCarrier(local_ip_addr, gw_id, carriers.ctrl_in_st, is_satellite)) return false;
				if (!readCarrier(local_ip_addr, gw_id, carriers.data_in_st, is_satellite)) return false;
				if (!readCarrier(local_ip_addr, gw_id, carriers.logon_in, is_satellite)) return false;
			} else {
				if (!readCarrier(local_ip_addr, gw_id, carriers.ctrl_out_st, !is_satellite)) return false;
				if (!readCarrier(local_ip_addr, gw_id, carriers.data_out_st, !is_satellite)) return false;
			}
			break;
		case Component::gateway:
			if (create_in_carriers) 
			{
				if (!readCarrier(local_ip_addr, gw_id, carriers.ctrl_in_gw, is_satellite)) return false;
				if (!readCarrier(local_ip_addr, gw_id, carriers.data_in_gw, is_satellite)) return false;
			} else {
				if (!readCarrier(local_ip_addr, gw_id, carriers.ctrl_out_gw, !is_satellite)) return false;
				if (!readCarrier(local_ip_addr, gw_id, carriers.logon_out, !is_satellite)) return false;
				if (!readCarrier(local_ip_addr, gw_id, carriers.data_out_gw, !is_satellite)) return false;
			}
			break;
		default:
			LOG(this->log_init, LEVEL_ERROR,
			    "Host should be either terminal or gateway");
			return false;
	}

	return true;
}

bool sat_carrier_channel_set::readConfig(const string local_ip_addr,
                                         component_t destination_host,
                                         spot_id_t spot_id,
                                         bool in)
{
	auto Conf = OpenSandModelConf::Get();

	// get host type
	Component host = Conf->getComponentType();

	// for terminal get the corresponding spot
	if (host == Component::terminal)
	{
		tal_id_t gw_id;
		if(!Conf->getGwWithTalId(this->tal_id, gw_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "couldn't find gateway for tal %d",
			    this->tal_id);
			return false;
		}
		return readSpot(local_ip_addr, in, host, gw_id, false);
	}
	else if (host == Component::gateway)
	{
		return readSpot(local_ip_addr, in, host, this->tal_id, false);
	}
	else if (host == Component::satellite)
	{
		return readSpot(local_ip_addr, in, destination_host, spot_id, true);
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "couldn't get component type\n");
		return false;
	}
	return true;
}

bool sat_carrier_channel_set::readInConfig(const string local_ip_addr,
                                           component_t destination_host,
                                           spot_id_t spot_id)
{
	return this->readConfig(local_ip_addr, destination_host, spot_id, true);
}

bool sat_carrier_channel_set::readOutConfig(const string local_ip_addr,
                                            component_t destination_host,
                                            spot_id_t spot_id)
{
	return this->readConfig(local_ip_addr, destination_host, spot_id, false);
}

bool sat_carrier_channel_set::send(uint8_t carrier_id,
                                   const unsigned char *data,
                                   size_t length)
{
	std::vector <UdpChannel *>::const_iterator it;
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
	std::vector < UdpChannel * >::iterator it;

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
	std::vector < UdpChannel * >::iterator it;
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



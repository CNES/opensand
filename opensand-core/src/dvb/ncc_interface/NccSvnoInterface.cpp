/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
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
 * @file     NccSvnoInterface.cpp
 * @author   Adrien THIBAUD / <athibaud@toulouse.viveris.com>
 * @brief    Interface between NCC and SVNO components
 */

#include "NccSvnoInterface.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <sstream>


/**
 * @brief Initialize the interface between NCC and SVNO components
 */
NccSvnoInterface::NccSvnoInterface():
	NccInterface(),
	requests_list()
{
}


/**
 * @brief Destroy the interface between NCC and SVNO components
 */
NccSvnoInterface::~NccSvnoInterface()
{
	// free all SVNO requests stored in list
	this->requests_list.clear();
}


/**
 * @brief Get the TCP socket that listens for incoming SVNO connections
 *
 * @return  the listen socket or -1 if not initialized
 */
int NccSvnoInterface::getSvnoListenSocket()
{
	return this->getSocketListen();
}


/**
 * @brief Get the TCP socket connected to the the SVNO component
 *
 * @return  the client socket or -1 if not connected
 */
int NccSvnoInterface::getSvnoClientSocket()
{
	return this->getSocketClient();
}

/**
 * @brief Get the first request of the list of SVNO requests
 *
 * @return  the first request of the list of SVNO requests,
 *          NULL if no request is available
 */
std::unique_ptr<SvnoRequest> NccSvnoInterface::getNextSvnoRequest()
{
	if (!this->requests_list.empty())
	{
		// get the first request of the list and make a copy of it.
		// the erase() method calls the object's destructor,
		// so we have to deference the request object in order to preserve it.
		auto it = this->requests_list.begin();
		std::unique_ptr<SvnoRequest> request = std::move(*it);

		// remove the first request of the list
		this->requests_list.erase(it);
		return request;
	}

	return {nullptr};
}


/**
 * @brief Create a TCP socket that listens for incoming SVNO connections
 *
 * @return  true on success, false on failure
 */
bool NccSvnoInterface::initSvnoSocket(int tcp_port)
{
	if(tcp_port <= 0 && tcp_port >= 0xffff)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad value for parameter 'SVNO NCC port'\n");
		return false;
	}

	LOG(this->log_ncc_interface, LEVEL_NOTICE,
	    "TCP port to listen for SVNO connections = %d\n", tcp_port);

	return this->initSocket(tcp_port);
}


bool NccSvnoInterface::readSvnoMessage(const Rt::NetSocketEvent& event)
{
	// a SVNO must be connected to read a message from it!
	if(!this->is_connected)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "trying to read on SVNO socket while no SVNO "
		    "component is connected\n");
		return false;
	}

	Rt::Data recv_buffer = event.getData();

	// parse message received from SVNO
	return this->parseSvnoMessage(recv_buffer);
}


/**
 * @brief Parse a message sent by the SVNO component
 *
 * A message contains one or more lines. Every line is a command. There are
 * allocation commands or release commands. All the commands in a message
 * must be of the same type.
 *
 * @param message   the message sent by the SVNO component
 * @return          true if message was successfully parsed, false otherwise
 */
bool NccSvnoInterface::parseSvnoMessage(const Rt::Data& message)
{
	unsigned char cmd[64];
	int all_cmds_type = -1; /* initialized because GCC is not smart enough
	                           to find that the variable can not be used
	                           uninitialized */

	// for every command in the message...
	unsigned int nb_cmds = 0;
	Rt::DataStream stream(message);
	while(stream.getline(cmd, 64))
	{
		// parse the command
		std::unique_ptr<SvnoRequest> request = this->parseSvnoCommand(cmd);
		if(!request)
		{
			LOG(this->log_ncc_interface, LEVEL_ERROR,
			    "failed to parse command #%d in SVNO message, "
			    "skip the command\n", nb_cmds + 1);
			continue;
		}

		// check that all commands are of of the same type
		// (ie. all allocations or all de-allocations)
		if(nb_cmds == 0)
		{
			// first command, set the type
			all_cmds_type = request->getType();
		}
		else if(request->getType() != all_cmds_type)
		{
			LOG(this->log_ncc_interface, LEVEL_ERROR,
			    "command #%d is not of the same type "
			    "as command #1, this is not accepted, "
			    "so ignore the command\n", nb_cmds);
			continue;
		}

		// store the command parameters in context
		this->requests_list.push_back(std::move(request));

		nb_cmds++;
	}

	if(nb_cmds == 0)
	{
		// no request correctly processed
		return false;
	}

	return true;
}


/**
 * @brief Parse one of the commands sent in a message by the SVNO component
 *
 * @param cmd       a command sent by the SVNO component
 * @return          the created SVNO request if command was successfully parsed,
 *                  NULL in case of failure
 */
std::unique_ptr<SvnoRequest> NccSvnoInterface::parseSvnoCommand(const Rt::Data& cmd)
{
	unsigned int spot_id;
	unsigned int type;      // allocation or release request
	unsigned int band;      // band
	Rt::Data label;      // label 
	rate_kbps_t new_rate_kbps; // new rate

	// retrieve values in the command
	Rt::IDataStream cmd_s(cmd);
	cmd_s >> spot_id >> type >> band >> label >> new_rate_kbps;
	if(cmd_s.fail())
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad formated SVNO command received: '%s'\n", cmd);
		return nullptr;
	}

	// request type must be 1 for allocation or 0 for de-allocation
	if(type != SVNO_REQUEST_ALLOCATION && type != SVNO_REQUEST_RELEASE)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad request type in SVNO command '%s', "
		    "type should be %u or %u\n", cmd,
		    SVNO_REQUEST_ALLOCATION, SVNO_REQUEST_RELEASE);
		return nullptr;
	}

	// request band must be 0 for forward or 1 for return
	if(band != FORWARD && band != RETURN)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad request band in SVNO command '%s', "
		    "band is %u but should be %u or %u\n", cmd,
		    band, FORWARD, RETURN);
		return nullptr;
	}

	LOG(this->log_ncc_interface, LEVEL_INFO,
	    "SVNO %s received for category %s on spot %u: "
	    "new rate = %u kbits/s, band = %s\n",
	    ((type == SVNO_REQUEST_ALLOCATION) ? "allocation" : "release"),
	    label.c_str(), spot_id, new_rate_kbps,
	    ((band == FORWARD) ? "Forward" : "Upward"));

	// build SVNO request object
	return std::make_unique<SvnoRequest>(spot_id,
	                                     (svno_request_type_t)type,
	                                     (band_t)band,
	                                     reinterpret_cast<const char*>(label.c_str()),
	                                     new_rate_kbps);
}

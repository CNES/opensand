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
 * @file NccPepInterface.cpp
 * @brief Interface between NCC and PEP components
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "NccPepInterface.h"

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
 * @brief Initialize the interface between NCC and PEP components
 */
NccPepInterface::NccPepInterface():
	NccInterface(),
	requests_list()
{
}


/**
 * @brief Destroy the interface between NCC and PEP components
 */
NccPepInterface::~NccPepInterface()
{
	// free all PEP requests stored in list
	this->requests_list.clear();
	this->~NccInterface();
}


/**
 * @brief Get the TCP socket that listens for incoming PEP connections
 *
 * @return  the listen socket or -1 if not initialized
 */
int NccPepInterface::getPepListenSocket()
{
	return this->getSocketListen();
}


/**
 * @brief Get the TCP socket connected to the the PEP component
 *
 * @return  the client socket or -1 if not connected
 */
int NccPepInterface::getPepClientSocket()
{
	return this->getSocketClient();
}


/**
 * @brief Get the type of current PEP requests
 *
 * @return  PEP_REQUEST_ALLOCATION or PEP_REQUEST_RELEASE
 */
pep_request_type_t NccPepInterface::getPepRequestType()
{
	pep_request_type_t type;

	if(this->requests_list.empty())
	{
		type = PEP_REQUEST_UNKNOWN;
	}
	else
	{
		type = this->requests_list.front()->getType();
	}

	return type;
}


/**
 * @brief Get the first request of the list of PEP requests
 *
 * @return  the first request of the list of PEP requests,
 *          NULL if no request is available
 */
std::unique_ptr<PepRequest> NccPepInterface::getNextPepRequest()
{
	if (!this->requests_list.empty())
	{
		// get the first request of the list and make a copy of it.
		// the erase() method calls the object's destructor,
		// so we have to deference the request object in order to preserve it.
		auto it = this->requests_list.begin();
		std::unique_ptr<PepRequest> request = std::move(*it);

		// remove the first request of the list
		this->requests_list.erase(it);
		return request;
	}

	return {nullptr};
}


/**
 * @brief Create a TCP socket that listens for incoming PEP connections
 *
 * @return  true on success, false on failure
 */
bool NccPepInterface::initPepSocket(uint16_t tcp_port)
{
	LOG(this->log_ncc_interface, LEVEL_NOTICE,
	    "TCP port to listen for PEP connections = %d\n", tcp_port);

	return this->initSocket(tcp_port);
}


bool NccPepInterface::readPepMessage(const Rt::NetSocketEvent& event, tal_id_t &tal_id)
{
	// a PEP must be connected to read a message from it!
	if(!this->is_connected)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "trying to read on PEP socket while no PEP "
		    "component is connected yet\n");
		return false;
	}

	Rt::Data recv_buffer = event.getData();

	// parse message received from PEP
	if(this->parsePepMessage(recv_buffer, tal_id) != true)
	{
		// an error occured when parsing the PEP message
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "failed to parse message received from PEP "
		    "component\n");
		goto close;
	}

	return true;

close:
	LOG(this->log_ncc_interface, LEVEL_ERROR,
	    "close PEP client socket because of previous errors\n");
	this->is_connected = false;
	return false;
}


/**
 * @brief Parse a message sent by the PEP component
 *
 * A message contains one or more lines. Every line is a command. There are
 * allocation commands or release commands. All the commands in a message
 * must be of the same type.
 *
 * @param message   the message sent by the PEP component
 * @return          true if message was successfully parsed, false otherwise
 */
bool NccPepInterface::parsePepMessage(const Rt::Data& message, tal_id_t &tal_id)
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
		std::unique_ptr<PepRequest> request = this->parsePepCommand(cmd);
		if(!request)
		{
			LOG(this->log_ncc_interface, LEVEL_ERROR,
			    "failed to parse command #%d in PEP message, "
			    "skip the command\n", nb_cmds + 1);
			continue;
		}

		tal_id = request->getStId();

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
 * @brief Parse one of the commands sent in a message by the PEP component
 *
 * @param cmd       a command sent by the PEP component
 * @return          the created PEP request if command was successfully parsed,
 *                  NULL in case of failure
 */
std::unique_ptr<PepRequest> NccPepInterface::parsePepCommand(const Rt::Data& cmd)
{
	Rt::IDataStream stream(cmd);
	unsigned char c;        // the colon separator char
	bool good = true;

	// retrieve values in the command
	unsigned int type;      // allocation or release request
	stream >> type >> c;
	good = good && stream.good() && c == ':';

	unsigned int st_id;     // the ID of the ST the request is for
	stream >> st_id >> c;
	good = good && stream.good() && c == ':';

	unsigned int cra;       // the CRA value
	stream >> cra >> c;
	good = good && stream.good() && c == ':';

	unsigned int rbdc;      // the RBDC value
	stream >> rbdc >> c;
	good = good && stream.good() && c == ':';

	unsigned int rbdc_max;  // the RBDCmax value
	stream >> rbdc_max;
	good = good && stream.good();

	if(!good)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad formated PEP command received: '%s'\n", cmd);
		return {nullptr};
	}
	else
	{
		LOG(this->log_ncc_interface, LEVEL_INFO,
		    "well received\n");
	}

	// request type must be 1 for allocation or 0 for de-allocation
	if(type != PEP_REQUEST_ALLOCATION && type != PEP_REQUEST_RELEASE)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad request type in PEP command '%s', "
		    "should be %u or %u\n", cmd,
		    PEP_REQUEST_ALLOCATION, PEP_REQUEST_RELEASE);
		return {nullptr};
	}

	LOG(this->log_ncc_interface, LEVEL_INFO,
	    "PEP %s received for ST #%u: new CRA = %u kbits/s, "
	    "new RBDC = %u kbits/s, new RBDC Max = %u kbits/s ",
	    ((type == PEP_REQUEST_ALLOCATION) ? "allocation" : "release"),
	    st_id, cra, rbdc, rbdc_max);

	// build PEP request object
	return std::make_unique<PepRequest>((pep_request_type_t) type, st_id, cra, rbdc, rbdc_max);
}

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

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

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
	std::vector<PepRequest *>::iterator it;

	// free all PEP requests stored in list
	for(it = this->requests_list.begin();
	    it != this->requests_list.end();
		it = requests_list.erase(it));

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
PepRequest * NccPepInterface::getNextPepRequest()
{
	PepRequest *request;
	std::vector<PepRequest *>::iterator it;

	if(this->requests_list.empty())
	{
		request = NULL;
	}
	else
	{
		// get the first request of the list and make a copy of it.
		// the erase() method calls the object's destructor,
		// so we have to deference the request object in order to preserve it.
		it = this->requests_list.begin();
		request = *it;
		*it = NULL; // FIXME *it = NULL before erase it ?

		// remove the first request of the list
		this->requests_list.erase(it);
	}

	return request;
}


/**
 * @brief Create a TCP socket that listens for incoming PEP connections
 *
 * @return  true on success, false on failure
 */
bool NccPepInterface::initPepSocket()
{
	int tcp_port;

	// retrieve the TCP communication port dedicated
	// for NCC/PEP communications
    // TODO move configuration reading in bloc
	if(!Conf::getValue(Conf::section_map[NCC_SECTION_PEP], PEP_DAMA_PORT, tcp_port))
	{
		LOG(this->log_ncc_interface, LEVEL_NOTICE,
		    "section '%s': missing parameter '%s'\n",
		    NCC_SECTION_PEP, PEP_DAMA_PORT);
		return false;
	}

	if(tcp_port <= 0 && tcp_port >= 0xffff)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "section '%s': bad value for parameter '%s'\n",
		    NCC_SECTION_PEP, PEP_DAMA_PORT);
		return false;
	}

	LOG(this->log_ncc_interface, LEVEL_NOTICE,
	    "TCP port to listen for PEP connections = %d\n", tcp_port);

	return this->initSocket(tcp_port);
}


bool NccPepInterface::readPepMessage(NetSocketEvent *const event, tal_id_t &tal_id)
{
	char *recv_buffer;

	// a PEP must be connected to read a message from it!
	if(!this->is_connected)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "trying to read on PEP socket while no PEP "
		    "component is connected yet\n");
		goto error;
	}

	recv_buffer = (char *)(event->getData());

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
error:
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
bool NccPepInterface::parsePepMessage(const char *message, tal_id_t &tal_id)
{
	stringstream stream;
	char cmd[64];
	unsigned int nb_cmds;
	int all_cmds_type = -1; /* initialized because GCC is not smart enough
	                           to find that the variable can not be used
	                           uninitialized */

	// for every command in the message...
	nb_cmds = 0;
	stream << message;
	while(stream.getline(cmd, 64))
	{
		PepRequest *request;

		// parse the command
		request = this->parsePepCommand(cmd);
		if(request == NULL)
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
			delete request;
			continue;
		}

		// store the command parameters in context
		this->requests_list.push_back(request);

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
PepRequest * NccPepInterface::parsePepCommand(const char *cmd)
{
	unsigned int type;      // allocation or release request
	unsigned int st_id;     // the ID of the ST the request is for
	unsigned int cra;       // the CRA value
	unsigned int rbdc;      // the RBDC value
	unsigned int rbdc_max;  // the RBDCmax value
	int ret;

	// retrieve values in the command
	ret = sscanf(cmd, "%u:%u:%u:%u:%u", &type, &st_id, &cra, &rbdc, &rbdc_max);
	if(ret != 5)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "bad formated PEP command received: '%s'\n", cmd);
		return NULL;
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
		return NULL;
	}

	LOG(this->log_ncc_interface, LEVEL_INFO,
	    "PEP %s received for ST #%u: new CRA = %u kbits/s, "
	    "new RBDC = %u kbits/s, new RBDC Max = %u kbits/s ",
	    ((type == PEP_REQUEST_ALLOCATION) ? "allocation" : "release"),
	    st_id, cra, rbdc, rbdc_max);

	// build PEP request object
	return new PepRequest((pep_request_type_t) type, st_id, cra, rbdc, rbdc_max);
}

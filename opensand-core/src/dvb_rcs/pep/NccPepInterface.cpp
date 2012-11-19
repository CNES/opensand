/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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


#define DBG_PACKAGE PKG_DEFAULT
#include "opensand_conf/uti_debug.h"
#include "opensand_conf/conf.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <sstream>


/** The size of the receive buffer */
#define RCVBUFSIZE 200


/* static */
Event* NccPepInterface::error_sock_open = NULL;

/**
 * @brief Initialize the interface between NCC and PEP components
 */
NccPepInterface::NccPepInterface(): requests_list()
{
	this->socket_listen = -1;
	this->socket_client = -1;
	this->is_connected = false;
	
	if(error_sock_open == NULL)
	{
		error_sock_open = Output::registerEvent("ncc_pep_interface",
		                                           LEVEL_ERROR);
	}
}


/**
 * @brief Destroy the interface between NCC and PEP components
 */
NccPepInterface::~NccPepInterface()
{
	std::vector<PepRequest *>::iterator it;

	// close connection with PEP if any
	if(this->is_connected)
	{
		this->is_connected = false;
		close(this->socket_client);
	}

	// close listen socket
	if(this->socket_listen != -1)
	{
		close(this->socket_listen);
	}

	// free all PEP requests stored in list
	for(it = this->requests_list.begin();
	    it != this->requests_list.end();
		it = requests_list.erase(it));
}


/**
 * @brief Get the TCP socket that listens for incoming PEP connections
 *
 * @return  the listen socket or -1 if not initialized
 */
int NccPepInterface::getPepListenSocket()
{
	return this->socket_listen;
}


/**
 * @brief Get the TCP socket connected to the the PEP component
 *
 * @return  the client socket or -1 if not connected
 */
int NccPepInterface::getPepClientSocket()
{
	return this->socket_client;
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
		*it = NULL;

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
bool NccPepInterface::listenForPepConnections()
{
	int tcp_port;
	struct sockaddr_in local_addr;
	int ret;

	// retrieve the TCP communication port dedicated
	// for NCC/PEP communications
    // TODO move configuration reading in bloc
	if(!globalConfig.getValue(NCC_SECTION_PEP, PEP_DAMA_PORT, tcp_port))
	{
		UTI_INFO("section '%s': missing parameter '%s'\n",
		         NCC_SECTION_PEP, PEP_DAMA_PORT);
		goto error;
	}

	if(tcp_port <= 0 && tcp_port >= 0xffff)
	{
		UTI_ERROR("section '%s': bad value for parameter '%s'\n",
		          NCC_SECTION_PEP, PEP_DAMA_PORT);
		goto error;
	}

	UTI_INFO("TCP port to listen for PEP connections = %d\n", tcp_port);

	// create socket for incoming connections
	this->socket_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(this->socket_listen < 0)
	{
		UTI_ERROR("failed to create socket to listen for PEP "
		          "connections: %s (%d)\n", strerror(errno), errno);
		Output::sendEvent(error_sock_open, "failed to create socket"
		                     " to listen for PEP connections: %s (%d)\n",
		                     strerror(errno), errno);
		goto error;
	}

	// set the socket in non blocking mode
	ret = fcntl(this->socket_listen, F_SETFL, O_NONBLOCK);
	if(ret != 0)
	{
		UTI_ERROR("failed to set the PEP socket in non blocking mode: "
		          "%s (%d)\n", strerror(errno), errno);
		goto close_socket;
	}

	// bind on incoming port
	memset(&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(tcp_port);
	ret = bind(this->socket_listen, (struct sockaddr *) &local_addr,
	           sizeof(struct sockaddr_in));
	if(ret != 0)
	{
		UTI_ERROR("failed to bind PEP socket on TCP port %d: "
		          "%s (%d)\n", tcp_port, strerror(errno), errno);
		goto close_socket;
	}

	// listen for incoming connections from PEP components
	ret = listen(this->socket_listen, 1);
	if(ret != 0)
	{
		UTI_ERROR("failed to listen on PEP socket: %s (%d)\n",
		          strerror(errno), errno);
		goto close_socket;
	}

	return true;

close_socket:
	close(this->socket_listen);
error:
	return false;
}


/**
 * @brief Accept a new incoming connection from a PEP component
 *
 * @return  \li 0 on success,
 *          \li -1 on general failure,
 *          \li -2 on failure because already connected
 */
int NccPepInterface::acceptPepConnection()
{
	struct sockaddr_in pep_addr;
	socklen_t addr_length;
	long option;

	addr_length = sizeof(struct sockaddr_in);

	// drop the new connection if already connected to one PEP
	if(this->is_connected)
	{
		int othersock;
		othersock = accept(this->socket_listen,
		                   (struct sockaddr *) &pep_addr,
		                   &addr_length);
		close(othersock);
		goto already_connected;
	}

	// wait for a client to connect (this should not block because the
	// function is called only when there is an event on the listen socket)
        this->socket_client = accept(this->socket_listen,
	                             (struct sockaddr *) &pep_addr,
	                             &addr_length);
	if(this->socket_client < 0)
	{
		UTI_ERROR("failed to accept new connection on PEP socket: "
		          "%s (%d)\n", strerror(errno), errno);
		goto error;
	}

	// set the new socket in non blocking mode
	option = O_NONBLOCK;
	if(fcntl(this->socket_client, F_SETFL, option) != 0)
	{
		UTI_ERROR("set PEP socket in non blocking mode failed: %s (%d)\n",
		          strerror(errno), errno);
		goto close;
	}

	// the new socket is now connected to a client!
	this->is_connected = true;
	UTI_INFO("NCC is now connected to PEP %s\n",
	         inet_ntoa(pep_addr.sin_addr));

	return 0;

already_connected:
	return -2;
close:
	close(this->socket_client);
error:
	return -1;
}


/**
 * @brief Read a set of commands sent by the connected PEP component
 *
 * @return              the status of the action:
 *                      \li true if command is read and parsed successfully
 *                      \li false if a problem is encountered
 */
bool NccPepInterface::readPepMessage()
{
	char recv_buffer[RCVBUFSIZE];
	int recv_msg_size;

	// a PEP must be connected to read a message from it!
	if(!this->is_connected)
	{
		UTI_ERROR("trying to read on PEP socket while no PEP "
		          "component is connected yet\n");
		goto error;
	}

	// receive message from the connected PEP
	recv_msg_size = recv(this->socket_client, recv_buffer,
	                     RCVBUFSIZE - 1, 0);
	if(recv_msg_size < 0)
	{
		// read failure
		UTI_ERROR("failed to receive data on PEP socket: "
		          "%s (%d)\n", strerror(errno), errno);
		goto close;
	}
	else if(recv_msg_size == 0)
	{
		// empty message
		UTI_ERROR("no data received from PEP, is the PEP "
		          "in trouble ?\n");
		goto close;
	}
	else if(recv_msg_size >= RCVBUFSIZE)
	{
		// message too large
		UTI_ERROR("too much data received on PEP socket (%d bytes)\n",
		          recv_msg_size);
		goto close;
	}

	// terminate the string before parsing it
	recv_buffer[recv_msg_size] = '\0';

	// parse message received from PEP
	if(this->parsePepMessage(recv_buffer) != true)
	{
		// an error occured when parsing the PEP message
		UTI_ERROR("failed to parse message received from PEP component\n");
		goto close;
	}

	return true;

close:
	UTI_ERROR("close PEP client socket because of previous errors\n");
	this->is_connected = false;
	close(this->socket_client);
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
bool NccPepInterface::parsePepMessage(const char *message)
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
			UTI_ERROR("failed to parse command #%d in PEP message, "
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
			UTI_ERROR("command #%d is not of the same type "
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
		UTI_ERROR("bad formated PEP command received: '%s'\n", cmd);
		goto error;
	}

	// request type must be 1 for allocation or 0 for de-allocation
	if(type != PEP_REQUEST_ALLOCATION && type != PEP_REQUEST_RELEASE)
	{
		UTI_ERROR("bad request type in PEP command '%s', "
		          "should be %u or %u\n", cmd,
		          PEP_REQUEST_ALLOCATION, PEP_REQUEST_RELEASE);
		goto error;
	}

	UTI_INFO("PEP %s received for ST #%u: "
	         "new CRA = %u kbits/s, "
	         "new RBDC = %u kbits/s, "
	         "new RBDC Max = %u kbits/s ",
	         ((type == PEP_REQUEST_ALLOCATION) ? "allocation" : "release"),
	         st_id, cra, rbdc, rbdc_max);

	// build PEP request object
	return new PepRequest((pep_request_type_t) type, st_id, cra, rbdc, rbdc_max);

error:
	return NULL;
}

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file     NccInterface.cpp
 * @author   Adrien THIBAUD / <athibaud@toulouse.viveris.com>
 * @brief    File that describes the TCP Socket
 *
 */

#include "NccInterface.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

NccInterface::NccInterface()
{
	this->socket_listen = -1;
	this->socket_client = -1;
	this->is_connected = false;

	// Output log
	this->log_ncc_interface = Output::registerLog(LEVEL_WARNING,
	                                              "Dvb.Ncc.Interface");

}


NccInterface::~NccInterface()
{
	// Close connextion if any
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
}

int NccInterface::getSocketListen()
{
	return this->socket_listen;
}

int NccInterface::getSocketClient()
{
	return this->socket_client;
}

bool NccInterface::getIsConnected()
{
	return this->is_connected;
}

void NccInterface::setSocketClient(int socket_client)
{
	this->socket_client = socket_client;
}

void NccInterface::setIsConnected(bool is_connected)
{
	this->is_connected = is_connected;
}

bool NccInterface::initSocket(int tcp_port)
{
	struct sockaddr_in local_addr;
	int ret;
	int one = 1;

	// create socket for incoming connections
	this->socket_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(this->socket_listen < 0)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "failed to create socket to listen for "
		    "connections: %s (%d)\n", strerror(errno), errno);
		goto error;
	}

	// set options
	ret = fcntl(this->socket_listen, F_SETFL, O_NONBLOCK);
	if(ret != 0)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "failed to set the socket in non blocking mode: "
		    "%s (%d)\n", strerror(errno), errno);
		goto close_socket;
	}
	if(setsockopt(this->socket_listen, SOL_SOCKET, SO_REUSEADDR,
	              (char *)&one, sizeof(one))<0)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "Error in reusing addr\n");
		goto error;
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
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "failed to bind socket on TCP port %d: "
		    "%s (%d)\n", tcp_port, strerror(errno), errno);
		goto close_socket;
	}

	// listen for incoming connections from components
	ret = listen(this->socket_listen,1);
	if(ret != 0)
	{
		LOG(this->log_ncc_interface, LEVEL_ERROR,
		    "failed to listen on socket: %s (%d)\n",
		    strerror(errno), errno);
		goto close_socket;
	}

	return true;

close_socket:
	close(this->socket_listen);
error:
	return false;
}



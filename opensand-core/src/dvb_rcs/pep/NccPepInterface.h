/*
 *
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
 * @file NccPepInterface.h
 * @brief Interface between NCC and PEP components
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef NCC_PEP_ITF_H
#define NCC_PEP_ITF_H

#include "PepRequest.h"
#include <vector>

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

/**
 * @class NccPepInterface
 * @brief Interface between NCC and PEP components
 */
class NccPepInterface
{
 private:

	/**
	 * @brief The TCP socket that listens for a connection from a PEP
	 */
	int socket_listen;

	/**
	 * @brief The TCP socket established with a PEP
	 */
	int socket_client;

	/** Whether a PEP is connected or not */
	bool is_connected;

	/** The list of commands received from the PEP component */
	std::vector<PepRequest *> requests_list;

 public:

	/**** constructor/destructor ****/

	/* initialize the interface between NCC and PEP components */
	NccPepInterface();

	/* destroy the interface between NCC and PEP components */
	~NccPepInterface();


	/**** accessors ****/

	/* get the TCP socket that listens for incoming PEP connections */
	int getPepListenSocket();

	/* get the TCP socket connected to the the PEP component */
	int getPepClientSocket();

	/* get the type of current PEP requests */
	pep_request_type_t getPepRequestType();

	/* get the list of PEP requests */
	PepRequest * getNextPepRequest();


	/**** socket management ****/

	/* create a TCP socket that listens for incoming PEP connections */
	bool listenForPepConnections();

	/* accept a new incoming connection from a PEP component */
	int acceptPepConnection();

	/**
	 * @brief Read a set of commands sent by the connected PEP component
	 *
	 * @param event  The NetSocketEvent for PEP fd
	 * @return  The status of the action:
	 *            \li true if command is read and parsed successfully
	 *            \li false if a problem is encountered
	 */
	bool readPepMessage(NetSocketEvent *const event);

 private:
 
	/* error event to report an already opened socket */
	static Event *error_sock_open;

	/* parse a message sent by the PEP component */
	bool parsePepMessage(const char *message);

	/* parse one of the commands sent in a message by the PEP component */
	PepRequest * parsePepCommand(const char *cmd);
};

#endif

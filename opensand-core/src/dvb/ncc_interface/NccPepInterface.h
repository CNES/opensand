/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
#include "NccInterface.h"
#include <vector>

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

/**
 * @class NccPepInterface
 * @brief Interface between NCC and PEP components
 */
class NccPepInterface: public NccInterface
{
 private:

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
	bool initPepSocket();

	/**
	 * @brief Read a set of commands sent by the connected PEP component
	 *
	 * @param event  The NetSocketEvent for PEP fd
	 * @return  The status of the action:
	 *            \li true if command is read and parsed successfully
	 *            \li false if a problem is encountered
	 */
	bool readPepMessage(NetSocketEvent *const event, tal_id_t &tal_id);

 private:

	/* parse a message sent by the PEP component */
	bool parsePepMessage(const char *message, tal_id_t & tal_id);

	/* parse one of the commands sent in a message by the PEP component */
	PepRequest * parsePepCommand(const char *cmd);
};

#endif

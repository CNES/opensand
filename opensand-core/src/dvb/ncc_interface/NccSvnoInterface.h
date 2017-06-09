/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 CNES
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
 * @file NccSvnoInterface.h
 * @brief Interface between NCC and SVNO components
 * @author Adrien THIBAUD / Viveris Technologies
 */

#ifndef NCC_SVNO_ITF_H
#define NCC_SVNO_ITF_H

#include "SvnoRequest.h"
#include "NccInterface.h"
#include <vector>

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

/**
 * @class NccSvnoInterface
 * @brief Interface between NCC and SVNO components
 */
class NccSvnoInterface: public NccInterface
{
 private:

	/** The list of commands received from the SVNO component */
	std::vector<SvnoRequest *> requests_list;

 public:

	/**** constructor/destructor ****/

	/* initialize the interface between NCC and SVNO components */
	NccSvnoInterface();

	/* destroy the interface between NCC and SVNO components */
	~NccSvnoInterface();


	/**** accessors ****/

	/* get the TCP socket that listen for incoming SVNO connections */
	int getSvnoListenSocket();

	/* get the TCP socket connected to the SVNO component */
	int getSvnoClientSocket();

	/* get the list of SVNO requests */
	SvnoRequest * getNextSvnoRequest();


	/**** socket management ****/

	/* create a TCP socket that listens for incoming SVNO connections */
	bool initSvnoSocket();

	/**
	 * @brief Read a set of commands sent by the connected SVNO component
	 *
	 * @param event  The NetSocketEvent for SVNO fd
	 * @return  The status of the action:
	 *            \li true if command is read and parsed successfully
	 *            \li false if a problem is encountered
	 */
	bool readSvnoMessage(NetSocketEvent *const event);

 private:

	/* parse a message sent by the SVNO component */
	bool parseSvnoMessage(const char *message);

	/* parse one of the commands sent in a message by the SVNO component */
	SvnoRequest * parseSvnoCommand(const char *cmd);
};

#endif

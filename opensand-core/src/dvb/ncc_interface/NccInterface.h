/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file     NccInterface.h
 * @author   Adrien THIBAUD / <athibaud@toulouse.viveris.com>
 * @version  1.0
 * @brief    File that describes the TCP Socket
 *
 */

#ifndef NCC_INTERFACE_H
#define NCC_INTERFACE_H

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

/**
 * @class NccInterface
 * @brief Class that describes the TCP Socket
 */
class NccInterface
{
protected:
	/**
	 * @brief The TCP socket that listens for a connection
	 */
	int socket_listen;

	/**
	 * @brief The TCP socket established
	 */
	int socket_client;

	/** Whether an element is connected or not */
	bool is_connected;

	/** Output Log */
	std::shared_ptr<OutputLog> log_ncc_interface;

public:
	/**** constructor/destructor ****/
	NccInterface();
	~NccInterface();

	/**** accessors ****/
	int getSocketListen();
	int getSocketClient();
	bool getIsConnected();

	/***** settors *****/
	void setSocketClient(int socket_client);
	void setIsConnected(bool is_connected);

	/**** socket management ****/
	
	/*create a TCP socket connected to the component */
	bool initSocket(int tcp_port);
};

#endif

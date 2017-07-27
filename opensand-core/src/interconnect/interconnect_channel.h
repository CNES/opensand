/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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

/*
 * @file sat_carrier_channel_set.h
 * @brief This class implements a TCP channel for interconnecting blocks
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.fr>
 */

#ifndef INTERCONNECT_CHANNEL_H
#define INTERCONNECT_CHANNEL_H

#include "OpenSandCore.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <vector>
#include <linux/if_packet.h>
#include <net/if.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

/**
 * @class interconnect_channel
 * @brief This implements a interconnect channel
 */
class interconnect_channel
{
 public:

	interconnect_channel(bool input, bool output);
	~interconnect_channel();

	bool isInputOk();

	bool isOutputOk();

	/**
	 * Start listening for incoming connections on the channel.
	 *
	 * @param port              The number port to listen at
	 * @return true on success, false otherwise
	 */
	bool listen(uint16_t port);

	/**
	 * Initiates connection with remote socket.
	 *
	 * @param ip_addr           The IP address of remote socket
	 * @param port              The port of remote socket
	 * * @return true on success, false otherwise
	 */
	bool connect(const string ip_addr, uint16_t port);

	/**
	 * @brief Send packet with type via the TCP socket
	 *
	 * @param msg         The packet
	 * @return 0 on success, 1 stored in buffer, -1 error with connection
	 */
	int sendPacket(rt_msg_t msg) { return (this->send((const unsigned char *) msg.data,
                                                          msg.length, msg.type)); };

	/**
	 * @brief Send data via the TCP socket
	 *
	 * @param data        The data to send
	 * @param length      The liength of the data
	 * @param type        The packet type
	 * @param flush       Don't store data, just flush buffer
	 *
	 * @return 0 on success, 1 stored in buffer, -1 error with connection
	 */
	int send(const unsigned char *data, size_t length, uint8_t type=0,
	         bool flush=false);

	/**
	 * @brief Receive from the TCP socket.
	 *
	 * The function works in blocking mode, so call it only when you are sure
	 * some data is ready to be received. Data is stored in recv_buffer.
	 * Packets must be retrieved after by calling getPacket.
	 *
	 * @param event         The event on channel fd
	 * @return  0 on success, -1 on error
	 */
	int receive(NetSocketEvent *const event);

	/**
	 * @brief Try to send data stored in send buffer
	 */
	void flush() {this->send((unsigned char *)NULL, 0, 0, true);};

	/**
	 * Get Packet from receive buffer
	 *
	 * @param buf               Buffer to store the packet
	 * @param data_len          Lenght of packet
	 * @param type              Type of the packet
	 * @return true if a packet was retrieved, false otherwise.
	 */
	bool getPacket(unsigned char **buf, size_t &data_len,
	               uint8_t &type);

	/**
	 * Get socket channel fd
	 *
	 * @return channel fd.
	 */
	int getFd();

	/**
	 * Get socket listen fd
	 *
	 * @return listen fd.
	 */
	int getListenFd();

	/**
	 * Check if socket is closed
	 *
	 * @return true if connection is opened
	 */
	bool isClosed();

	/**
	 * Check if socket is active
	 *
	 * @return true if connection is opened
	 */
	bool isOpen() { return (!this->isClosed()); };

	/**
	 * Set the channel socket
	 *
	 * @param sock  The socket for the channel
	 * */
	void setChannelSock(int sock);

	/**
	 * Set the channel socket to blocking mode
	 *
	 * @return true if could perform the task.
	 */
	bool setSocketBlocking();

	bool isConnected() {return (this->sock_channel > 0);};

	/**
	 * Close sock channel
	 */
	void close();

 private:

	/**
	 *  Get available space in recv_buffer
	 *
	 * @return Number of bytes free on recv_buffer
	 */
	size_t getFreeSpace();

	/**
	 * Get used space in recv_buffer
	 *
	 * @return Number of bytes used on recv_buffer
	 */
	size_t getUsedSpace();

	/**
	 * Store received data into recv_buffer
	 *
	 * @param data              A pointer to the first byte of data
	 * @param len               The length of data to copy
	 * @return number of bytes copied. -1 if error
	 */
	ssize_t storeData(const unsigned char *data,
	                  size_t len);

	/**
	 * Read data from recv_buffer
	 *
	 * @param buf               The buffer to store the data
	 * @param len               The length of data to receive
	 * @param start             The start position
	 * @return the finish buffer position. -1 if error
	 */
	ssize_t readData(unsigned char *buf, size_t len,
	                 size_t start);

	/**
	 * Discard uncomplete packet after a loss of data.
	 */
	void discardPacket();

	// if channel accepts input
	bool m_input;

	// if channel accepts output
	bool m_output;

	// address of the channel
	struct sockaddr_in m_socketAddr;

	// the remote IP address of the channel 
	struct sockaddr_in m_remoteIPAddress;

	// the socket for the listener
	int sock_listen;

	// the socket which defines the channel
	int sock_channel;

	// internal buffer to build and send packets with length
	unsigned char send_buffer[5*MAX_SOCK_SIZE];

	// send buffer position
	size_t send_pos;

	// internal buffer for receiving entire packets (circular buffer)
	// TODO: any way to reuse the other buffer instead of creating two?
	unsigned char recv_buffer[5*MAX_SOCK_SIZE];

	// size of recv buffer
	const size_t recv_size;

	// bytes remaining to complete packet
	size_t pkt_remaining; 

	// current positions in the recv_buffer
	size_t recv_start;
	size_t recv_end;

	// if recv buffer is full
	bool recv_is_full;

	// if recv buffer is empty
	bool recv_is_empty;

	// Output Log
	OutputLog *log_init;
	OutputLog *log_interconnect;
};

#endif

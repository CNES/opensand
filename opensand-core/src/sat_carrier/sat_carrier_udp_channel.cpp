/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file sat_carrier_udp_channel.cpp
 * @brief This implements an UDP satellite carrier channel
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <stdlib.h>

#define DBG_PACKAGE PKG_SAT_CARRIER
#include "opensand_conf/uti_debug.h"

#include "sat_carrier_udp_channel.h"


/**
 * Constructor
 *
 * @param channelID           the Id of the new channel
 * @param input               true if the channel accept incoming data
 * @param output              true if channel send data
 * @param local_interface_name.c_str()  the name of the local network interface to use
 * @param port                the port on which the channel is bind
 * @param multicast           true is this is a multicast channel
 * @param local_ip_addr       the host IP address
 * @param ip_addr             the IP address of the remote host in case of
 *                            output channel or the multicast IP address in
 *                            case of input multicast channel
 *
 * @see sat_carrier_channel::sat_carrier_channel()
 */
sat_carrier_udp_channel::sat_carrier_udp_channel(unsigned int channelID,
                                                 bool input,
                                                 bool output,
                                                 const string local_interface_name,
                                                 unsigned short port,
                                                 bool multicast,
                                                 const string local_ip_addr,
                                                 const string ip_addr):
	sat_carrier_channel(channelID, input, output)
{
	int ifIndex;
	struct ip_mreq imr;
	unsigned char ttl = 1;
	int one = 1;
	int buffer = 0;
	socklen_t size = 4;

	this->m_multicast = multicast;
	this->stack_len = 0;
	this->send_stack = false;

	bzero(&this->m_socketAddr, sizeof(this->m_socketAddr));
	m_socketAddr.sin_family = AF_INET;
	m_socketAddr.sin_port = htons(port);

	bzero(&m_remoteIPAddress,sizeof(m_remoteIPAddress));

	// open the socket
	this->sock_channel = socket(AF_INET, SOCK_DGRAM, 0);
	if(this->sock_channel < 0)
	{
		UTI_ERROR("Can't open the receive socket, errno %d (%s)\n",
		          errno, strerror(errno));
		goto error;
	}

	if(setsockopt(this->sock_channel, SOL_SOCKET, SO_REUSEADDR,
	              (char *)&one, sizeof(one))<0)
	{
		UTI_ERROR("Error in reusing addr\n");
		goto error;
	}

	// get the index of the network interface
	ifIndex = this->getIfIndex(local_interface_name.c_str());

	if(ifIndex < 0)
	{
		UTI_ERROR("cannot get the index for %s\n", local_interface_name.c_str());
		goto error;
	}

	// Set destination characteristics to send the datagramms
	if(this->isOutputOk())
	{
		this->counter = 0;
		// get the remote IP address
		if(inet_aton(ip_addr.c_str(), &(m_remoteIPAddress.sin_addr))<0)
		{
			UTI_ERROR("cannot get the remote IP address for %s \n",
			          ip_addr.c_str());
			goto error;
		}
		m_remoteIPAddress.sin_family = AF_INET;
		m_remoteIPAddress.sin_port = htons(port);
		m_socketAddr.sin_addr.s_addr = inet_addr(local_ip_addr.c_str());

		// creation of the link between the socket and its port
		if(bind(this->sock_channel, (struct sockaddr *) &this->m_socketAddr,
		         sizeof(this->m_socketAddr)) < 0)
		{
			UTI_ERROR("failed to bind to UDP socket: %s (%d)\n",
			          strerror(errno), errno);
			goto error;
		}

		if(this->m_multicast)
		{
			if(setsockopt(sock_channel, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
			              sizeof(ttl)) < 0)
			{
				UTI_ERROR("setsockopt: IP_MULTICAST_TTL activation failed\n");
				goto error;
			}
		}
	}
	else if(this->isInputOk())
	{
		// check if the socket buffer is enough
		if(getsockopt(this->sock_channel, SOL_SOCKET, SO_RCVBUF,
		              (char *)&buffer, &size)<0)
		{
			UTI_ERROR("getsockopt : SO_RCVBUF failed\n");
			goto error;
		}
		UTI_INFO("size of socket buffer: %d \n", buffer);

		if(this->m_multicast)
		{
			if(inet_aton(ip_addr.c_str(), &this->m_socketAddr.sin_addr) < 0)
			{
				perror("inet_aton");
				goto error;
			}

			// creation of the link between the socket and its port
			if(bind(this->sock_channel, (struct sockaddr *) &this->m_socketAddr,
			        sizeof(this->m_socketAddr)) < 0)
			{
				UTI_ERROR("failed to bind to multicast UDP socket: %s (%d)\n",
				          strerror(errno), errno);
				goto error;
			}

			memset(&imr, 0, sizeof(struct ip_mreq));
			imr.imr_multiaddr.s_addr = inet_addr(ip_addr.c_str());
			imr.imr_interface.s_addr = inet_addr(local_ip_addr.c_str());

			if(setsockopt(this->sock_channel, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			              (void *) &imr, sizeof(struct ip_mreq)) < 0)
			{
				UTI_ERROR("failed to join multicast group with multicast address "
				          "%s and interface address %s: %s (%d)\n",
				          ip_addr.c_str(), local_ip_addr.c_str(), strerror(errno), errno);
				goto error;
			}
		}
		else
		{
			m_socketAddr.sin_addr.s_addr = inet_addr(local_ip_addr.c_str());
			// creation of the link between the socket and its port
			if(bind(this->sock_channel, (struct sockaddr *) &this->m_socketAddr,
			        sizeof(this->m_socketAddr)) < 0)
			{
				UTI_ERROR("failed to bind unicast UDP socket: %s (%d)\n",
				          strerror(errno), errno);
				goto error;
			}
		}
	}
	else
	{
		UTI_ERROR("channel doesn't receive and doesn't send data\n");
		goto error;
	}

	UTI_INFO("UDP channel %u created with local IP %s and local port %u\n",
	         getChannelID(), inet_ntoa(m_socketAddr.sin_addr),
			 ntohs(m_socketAddr.sin_port));

	this->init_success = true;
	return;

error:
	UTI_ERROR("Can't create channel\n");
}

/**
 * Destructor
 */
sat_carrier_udp_channel::~sat_carrier_udp_channel()
{
	close(this->sock_channel);
	this->counterMap.clear();
}

/**
 * Return the network socket of the udp channel
 * @return the network socket of the udp channel
 */
int sat_carrier_udp_channel::getChannelFd()
{
	return this->sock_channel;
}

/**
 * Blocking receive function.
 * @param buf      pointer to a char buffer
 * @param data_len length of the received data
 * @param max_len  length of the buffer
 * @param timeout  maximum amount of time to wait for data (in ms)
 * @return         0 on success, 1 if the function should be
 *                 called another time, -1 on error
 */
int sat_carrier_udp_channel::receive(unsigned char *buf, unsigned int *data_len,
                                     unsigned int max_len, long timeout)
{
	const char FUNCNAME[] = "[sat_carrier_udp_channel::receive]";
	int read_len;
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(struct sockaddr_in);
	ip_to_counter_map::iterator it;
	std::string ip_address;
	uint8_t nb_sequencing;
	uint8_t current_sequencing;

	*data_len = 0;

	if(this->send_stack && this->stack_len > 0)
	{
		UTI_DEBUG("transmit the content of stack\n");
		memcpy(buf, this->stack, this->stack_len);
		*data_len = this->stack_len;
		this->stack_len = 0;
		this->send_stack = false;
		goto ignore;
	}

	UTI_DEBUG("%s try to receive a packet from satellite channel %d\n",
	          FUNCNAME, this->getChannelID());

	// the channel file descriptor must be valid
	if(this->getChannelFd() < 0)
	{
		UTI_ERROR("socket not open !\n");
		goto error;
	}

	// ignore if channel doesn't accept incoming data
	if(!this->isInputOk())
	{
		UTI_ERROR("channel %d does not accept data\n",
		          this->getChannelID());
		goto ignore;
	}

	// retrieve UDP datagramm
	read_len = recvfrom(this->sock_channel, this->recv_buffer,
	                    sizeof(this->recv_buffer), 0,
	                    (struct sockaddr *) &remote_addr, &remote_addr_len);
	if(read_len == -1)
	{
		UTI_ERROR("failed to receive UDP data: %s (%d)\n",
		          strerror(errno), errno);
		goto error;
	}
	else if(read_len <= 1)
	{
		UTI_ERROR("too few data received (%d bytes) on UDP channel\n", read_len);
		goto error;
	}

	// get the IP address of the sender
	ip_address = inet_ntoa(remote_addr.sin_addr);

	// check the sequencing of the datagramm
	nb_sequencing = this->recv_buffer[0];
	it = this->counterMap.find(ip_address);
	if(it == this->counterMap.end())
	{
		this->counterMap[ip_address] = nb_sequencing;
		if(nb_sequencing != 0)
		{
			UTI_NOTICE("force synchronisation on UDP channel %d "
			           "from %s at startup: received counter is %d "
			           "while it should have been 0\n",
			           this->getChannelID(), ip_address.c_str(),
			           nb_sequencing);
		}
	}
	else
	{
		if(it->second == 255)
			it->second = 0;
		else
			it->second++;
		current_sequencing = it->second;

		if(nb_sequencing != current_sequencing)
		{
			// authorize the overtaking of 3 packets
			if((current_sequencing > 252 && (nb_sequencing > current_sequencing)) ||
               (nb_sequencing < 4 && nb_sequencing <= (current_sequencing + 3) % 256) ||
			   (nb_sequencing <= (current_sequencing + 3)))
			{
				UTI_DEBUG("sequence desynchronisation on UDP channel %d "
				          "due to IP reassembly on attended datagram, "
				          "keep the current datagram in buffer "
				          "(counter is %u)\n", this->getChannelID(),
				          it->second);
				memcpy(this->stack, this->recv_buffer + 1, read_len - 1);
				this->stack_len = read_len - 1;
				this->stack_sequ = nb_sequencing;
				--(it->second) % 256;
				goto ignore;
			}
			else
			{
				UTI_ERROR("sequence desynchronisation on UDP channel %d "
				          "from %s: received counter is %d while it "
				          "should have been %d\n", this->getChannelID(),
				          ip_address.c_str(), nb_sequencing,
				          current_sequencing);
				it->second = nb_sequencing;
				goto error;

			}
		}
	}

	ip_address.clear();
	*data_len = read_len - 1;
	if(*data_len > max_len)
	{
		UTI_ERROR("received packet (%d bytes) too large for buffer "
		          "(%d bytes)\n", *data_len, max_len);
		goto error;
	}
	memcpy(buf, this->recv_buffer + 1, *data_len);

	if(this->stack_len > 0 && (it->second + 1) % 256 == this->stack_sequ)
	{
		++(it->second) % 256;
		this->send_stack = true;
		goto stacked;
	}

ignore:
	return 0;

stacked:
	return 1;

error:
	return -1;
}


/**
* Send a variable length buffer on the specified satellite carrier.
* @param buf pointer to a char buffer
* @param len length of the buffer
* @return -1 if failed, the size of data send if succeed
*/
int sat_carrier_udp_channel::send(unsigned char *buf, unsigned int len)
{
	int lg;

	UTI_DEBUG("data are trying to be send on channel %d\n",m_channelID);

	// check that the channel sends data
	if(!this->isOutputOk())
	{
		UTI_ERROR("Channel %d is not configure to send data\n", m_channelID);
		goto error;
	}

	// check if the socket is open
	if(this->getChannelFd() < 0)
	{
		UTI_ERROR("Socket not open !\n");
		goto error;
	}

	// add a sequencing field
	bzero(this->send_buffer, sizeof(this->send_buffer));
	this->send_buffer[0] = this->counter;
	memcpy(send_buffer + 1, buf, len);
	lg = len + 1;

	if(sendto(this->sock_channel, this->send_buffer, lg, 0,
	          (struct sockaddr *) &this->m_remoteIPAddress,
	          sizeof(this->m_remoteIPAddress)) < lg)
	{
		UTI_ERROR("Error:  sendto(..,0,..) errno %s (%d)\n",
		          strerror(errno),errno);
		goto error;
	}

	// update of the counter
	if(this->counter == 255)
		this->counter = 0;
	else
		this->counter++;

	UTI_DEBUG("==> SAT_Channel_Send [%d] (%s:%d): len=%d, counter: %d\n",
	          m_channelID, inet_ntoa(this->m_remoteIPAddress.sin_addr),
	          ntohs(this->m_remoteIPAddress.sin_port), lg, this->counter);

	return lg;

 error:
	return (-1);
}

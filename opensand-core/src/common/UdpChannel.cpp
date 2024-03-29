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
 * @file UdpChannel.cpp
 * @brief This implements an UDP satellite carrier channel
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>

#include <opensand_output/Output.h>
#include <opensand_rt/NetSocketEvent.h>

#include "UdpChannel.h"


/**
 * Constructor
 *
 * @param channelID           the Id of the new channel
 * @param input               true if the channel accept incoming data
 * @param output              true if channel send data
 * @param port                the port on which the channel is bind
 * @param multicast           true is this is a multicast channel
 * @param local_ip_addr       the host IP address
 * @param ip_addr             the IP address of the remote host in case of
 *                            output channel or the multicast IP address in
 *                            case of input multicast channel
 * @param stack               The maximum number of packets buffered in the software
 *                            stack before sending content
 * @param rmem                The size of the reception UDP buffers in kernel
 * @param wmem                The size of the emission UDP buffers in kernel
 */
UdpChannel::UdpChannel(std::string name,
                       spot_id_t s_id,
                       unsigned int channel_id,
                       bool input,
                       bool output,
                       unsigned short port,
                       bool multicast,
                       const std::string local_ip_addr,
                       const std::string ip_addr,
                       unsigned int stack,
                       unsigned int rmem,
                       unsigned int wmem):
	spot_id(s_id),
	m_channel_id(channel_id),
	m_input(input),
	m_output(output),
	init_success(false),
	sock_channel(-1),
	m_multicast(multicast),
	stacked_ip(""),
	max_stack(stack)
{
	struct ip_mreq imr;
	unsigned char ttl = 1;
	int one = 1;

	// Output log
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, name + ".init");
	this->log_sat_carrier = Output::Get()->registerLog(LEVEL_WARNING, name + ".Channel");

	bzero(&this->m_socketAddr, sizeof(this->m_socketAddr));
	m_socketAddr.sin_family = AF_INET;
	m_socketAddr.sin_port = htons(port);

	bzero(&m_remoteIPAddress, sizeof(m_remoteIPAddress));

	// open the socket
	this->sock_channel = socket(AF_INET, SOCK_DGRAM, 0);
	if(this->sock_channel < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Can't open the receive socket, errno %d (%s)\n",
		    errno, strerror(errno));
		goto error;
	}

	if(setsockopt(this->sock_channel, SOL_SOCKET, SO_REUSEADDR,
	              (char *)&one, sizeof(one))<0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Error in reusing addr\n");
		goto error;
	}

	// Set destination characteristics to send the datagramms
	if(this->isOutputOk())
	{
		// set UDP socket size
		if(setsockopt(this->sock_channel, SOL_SOCKET, SO_SNDBUF,
		              &wmem, sizeof(wmem))<0)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "setsockopt : SO_SNDBUF failed\n");
			goto error;
		}
		LOG(this->log_init, LEVEL_NOTICE,
		    "size of socket buffer: %d \n", wmem);

		this->counter = 0;
		// get the remote IP address
		if(inet_aton(ip_addr.c_str(), &(m_remoteIPAddress.sin_addr))<0)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get the remote IP address for %s \n",
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
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to bind to UDP socket: %s (%d)\n",
			    strerror(errno), errno);
			goto error;
		}

		if(this->m_multicast)
		{
			if(setsockopt(sock_channel, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
			              sizeof(ttl)) < 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "setsockopt: IP_MULTICAST_TTL activation failed\n");
				goto error;
			}
		}
	}
	else if(this->isInputOk())
	{
		// set UDP socket size
		if(setsockopt(this->sock_channel, SOL_SOCKET, SO_RCVBUF,
		              &rmem, sizeof(rmem))<0)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "setsockopt : SO_RCVBUF failed\n");
			goto error;
		}
		LOG(this->log_init, LEVEL_NOTICE,
		    "size of socket buffer: %d \n", rmem);

		if(this->m_multicast)
		{
			if(inet_aton(ip_addr.c_str(), &this->m_socketAddr.sin_addr) < 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "error with inet_aton: %s", strerror(errno));
				goto error;
			}

			// creation of the link between the socket and its port
			if(bind(this->sock_channel, (struct sockaddr *) &this->m_socketAddr,
			        sizeof(this->m_socketAddr)) < 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to bind to multicast UDP "
				    "socket: %s (%d)\n", strerror(errno), errno);
				goto error;
			}

			memset(&imr, 0, sizeof(struct ip_mreq));
			imr.imr_multiaddr.s_addr = inet_addr(ip_addr.c_str());
			imr.imr_interface.s_addr = inet_addr(local_ip_addr.c_str());

			if(setsockopt(this->sock_channel, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			              (void *) &imr, sizeof(struct ip_mreq)) < 0)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to join multicast group with multicast"
				    " address %s and interface address %s: "
				    "%s (%d)\n", ip_addr.c_str(),
				    local_ip_addr.c_str(), strerror(errno), errno);
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
				LOG(this->log_init, LEVEL_ERROR,
				    "failed to bind unicast UDP socket: %s (%d)\n",
				    strerror(errno), errno);
				goto error;
			}
		}
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "channel doesn't receive and doesn't send data\n");
		goto error;
	}
	bzero(this->send_buffer, sizeof(this->send_buffer));

	LOG(this->log_init, LEVEL_NOTICE,
	    "UDP channel %u created with local IP %s and local "
	    "port %u\n", getChannelID(),
	    inet_ntoa(m_socketAddr.sin_addr),
	    ntohs(m_socketAddr.sin_port));

	this->init_success = true;
	return;

error:
	LOG(this->log_init, LEVEL_ERROR,
	    "Can't create channel\n");
}

/**
 * Destructor
 */
UdpChannel::~UdpChannel()
{
	close(this->sock_channel);
	this->udp_counters.clear();
	for(std::map<std::string, UdpStack *>::iterator it = this->stacks.begin();
	    it != this->stacks.end(); ++it)
	{
		delete (*it).second;
	}
	this->stacks.clear();
}


/**
 * Check if the channel was correctly created
 */
bool UdpChannel::isInit()
{
	return this->init_success;
}

/**
 * Get the ID of the channel
 * @return the channel ID
 */
unsigned int UdpChannel::getChannelID()
{
	return (m_channel_id);
}

/**
 * Get if the channel accept input
 * @return true if channel accept input
 */
bool UdpChannel::isInputOk()
{
	return (m_input);
}

/**
 * Get if the channel accept output
 * @return true if channel accept output
 */
bool UdpChannel::isOutputOk()
{
	return (m_output);
}

/**
 * Return the network socket of the udp channel
 * @return the network socket of the udp channel
 */
int UdpChannel::getChannelFd()
{
	return this->sock_channel;
}

/**
 * Return the spot id
 * @return the spot id
 */
spot_id_t UdpChannel::getSpotId()
{
	return this->spot_id;
}


/**
 * @brief Get the message in NetSocketEvent
 *
 * @param event    The NetSocketEvent on fd
 * @param buf      pointer to a char buffer
 * @param data_len length of the received data
 * @return         0 on success, 1 if the function should be
 *                 called another time, -1 on error
 */
// TODO why not work directly with Data here instead of buf, length
int UdpChannel::receive(NetSocketEvent *const event,
                        unsigned char **buf, size_t &data_len)
{
	struct sockaddr_in remote_addr;
	std::map<std::string , uint8_t>::iterator ip_count_it;
	std::string ip_address;
	uint8_t nb_sequencing;
	uint8_t current_sequencing;
	unsigned char *data;
	size_t recv_len;
	unsigned char *recv_data;

	if(!this->stacked_ip.empty())
	{
		LOG(this->log_sat_carrier, LEVEL_INFO,
		    "Send content of stack for address %s\n",
		    this->stacked_ip.c_str());
		if(!this->handleStack(buf, data_len))
		{
			goto error;
		}
		if(!this->stacked_ip.empty())
		{
			// we still have packets to send
			goto stacked;
		}
		goto end;
	}

	LOG(this->log_sat_carrier, LEVEL_INFO,
	    "try to receive a packet from satellite channel %d\n",
	    this->getChannelID());

	// the channel file descriptor must be valid
	if(this->getChannelFd() < 0)
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "socket not opened !\n");
		goto error;
	}

	// error if channel doesn't accept incoming data
	if(!this->isInputOk())
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "channel %d does not accept data\n",
		    this->getChannelID());
		goto error;
	}

	// we need to memcpy as the start pointer cannot be reused
	recv_len = event->getSize() - 1;
	recv_data = (unsigned char *)calloc(recv_len, sizeof(unsigned char));
	data = event->getData();
	memcpy(recv_data, data + 1, recv_len);
	remote_addr = event->getSrcAddr();

	// get the IP address of the sender
	ip_address = inet_ntoa(remote_addr.sin_addr);

	// check the sequencing of the datagramm
	nb_sequencing = data[0];
	free(data);
	ip_count_it = this->udp_counters.find(ip_address);
	if(ip_count_it == this->udp_counters.end())
	{
		this->udp_counters[ip_address] = nb_sequencing;
		if(nb_sequencing != 0)
		{
			LOG(this->log_sat_carrier, LEVEL_NOTICE,
			    "force synchronisation on UDP channel %d "
			    "from %s at startup: received counter is %d "
			    "while it should have been 0\n",
			    this->getChannelID(), ip_address.c_str(),
			    nb_sequencing);
		}
		this->stacks[ip_address] = new UdpStack();
		current_sequencing = nb_sequencing;
	}
	else
	{
		current_sequencing = (ip_count_it->second + 1) % 256;
		LOG(this->log_sat_carrier, LEVEL_DEBUG,
		    "Current UDP sequencing for address %s: %u\n",
		    ip_address.c_str(), current_sequencing);
	}
	// add the new packet in stack
	this->stacks[ip_address]->add(nb_sequencing, recv_data, recv_len);
	this->stacked_ip = ip_address;
	// send the current packet
	if(this->stacks[ip_address]->hasNext(current_sequencing))
	{
		LOG(this->log_sat_carrier, LEVEL_DEBUG,
		    "Next UDP packet is in stack\n");
		this->handleStack(buf, data_len, current_sequencing,
		                  this->stacks[ip_address]);
		if(!this->stacked_ip.empty())
		{
			// we still have packets to send
			goto stacked;
		}
		ip_count_it->second = current_sequencing;
	}
	else
	{
		this->stacked_ip = "";
		LOG(this->log_sat_carrier, LEVEL_INFO,
		    "No UDP packet for current sequencing (%u) at IP %s "
		    "wait for next packets (last received %u)\n",
		    current_sequencing, ip_address.c_str(), nb_sequencing);
	}
	// check that we do not have to much packets in stack
	if(this->stacks[ip_address]->getCounter() > this->max_stack)
	{
		// suppose we lost the packet
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "we may have lost UDP packets, check "
		    "and adjust UDP buffers\n");
		// send the next packets from stack
		current_sequencing = (current_sequencing + 1) % 256;
		while(!this->stacks[ip_address]->hasNext(current_sequencing))
		{
			LOG(this->log_sat_carrier, LEVEL_INFO,
			    "packet missing: %u\n", current_sequencing);
			current_sequencing = (current_sequencing + 1) % 256;
		}
		// we should be able to return a packet here
		ip_count_it->second = current_sequencing;
		this->stacked_ip = ip_address;
		goto stacked;
	}

end:
	return 0;

stacked:
	return 1;

error:
	return -1;
}


bool UdpChannel::handleStack(unsigned char **buf, size_t &data_len)
{
	std::map<std::string , uint8_t>::iterator count_it = this->udp_counters.find(this->stacked_ip);
	std::map<std::string, UdpStack *>::iterator stack_it = this->stacks.find(this->stacked_ip);
	uint8_t counter;
	
	if(count_it == this->udp_counters.end())
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "cannot find UDP counter for IP %s\n",
		    this->stacked_ip.c_str());
		return false;
	}
	if(stack_it == this->stacks.end())
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "cannot find UDP stack for IP %s\n",
		    this->stacked_ip.c_str());
		return false;
	}

	counter = (*count_it).second;
	this->handleStack(buf, data_len,
	                  counter,
	                  (*stack_it).second);
	if(!this->stacked_ip.empty())
	{
		// update counter for next stacked packet
		(*count_it).second = (counter + 1) % 256;
	}
	return true;
}
	
	
void UdpChannel::handleStack(unsigned char **buf, size_t &data_len,
                             uint8_t counter, UdpStack *stack)
{
	LOG(this->log_sat_carrier, LEVEL_INFO,
	    "transmit UDP packet for source IP %s at counter %d\n",
	    this->stacked_ip.c_str(), counter);
	stack->remove(counter, buf, data_len);
	counter = (counter + 1) % 256;
	// if we don't have following packets in FIFO reset stacked_ip
	if(!stack->hasNext(counter))
	{
		this->stacked_ip = "";
	}
}


bool UdpChannel::send(const unsigned char *data, size_t length)
{
	ssize_t slen;

	LOG(this->log_sat_carrier, LEVEL_INFO,
	    "data are trying to be send on channel %d\n", m_channel_id);

	// check that the channel sends data
	if(!this->isOutputOk())
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "Channel %d is not configure to send data\n",
		    m_channel_id);
		goto error;
	}

	// check if the socket is open
	if(this->getChannelFd() < 0)
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "Socket not open !\n");
		goto error;
	}

	// add a sequencing field
	bzero(this->send_buffer, sizeof(this->send_buffer));
	this->send_buffer[0] = this->counter;
	memcpy(send_buffer + 1, data, length);
	slen = length + 1;

	if(sendto(this->sock_channel, this->send_buffer, slen, 0,
	          (struct sockaddr *) &this->m_remoteIPAddress,
	          sizeof(this->m_remoteIPAddress)) < slen)
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR,
		    "Error:  sendto(..,0,..) errno %s (%d)\n",
		    strerror(errno), errno);
		goto error;
	}

	// update of the counter
	this->counter = (this->counter + 1) % 256;

	LOG(this->log_sat_carrier, LEVEL_INFO,
	    "==> SAT_Channel_Send [%d] (%s:%d): len=%zd, counter: %d\n",
	    m_channel_id, inet_ntoa(this->m_remoteIPAddress.sin_addr),
	    ntohs(this->m_remoteIPAddress.sin_port), slen,
	    this->counter);

	return true;

error:
	return false;
}


UdpStack::UdpStack()
{
	// Output log
	this->log_sat_carrier = Output::Get()->registerLog(LEVEL_WARNING, "SatCarrier.Channel");
	// reserve space for all UDP counters
	this->reserve(256);
	for(unsigned int i = 0; i < 256; i++)
	{
		this->push_back(std::make_pair<unsigned char *, size_t>(NULL, 0));
	}
	this->counter = 0;
}


UdpStack::~UdpStack()
{
	this->reset();
	this->clear();
}


void UdpStack::add(uint8_t udp_counter, unsigned char *data, size_t data_length)
{
	if(this->at(udp_counter).first)
	{
		LOG(this->log_sat_carrier, LEVEL_ERROR, 
		    "new data for UDP stack at position %u, erase "
		    "previous data\n", udp_counter);
		this->counter--;
		delete (this->at(udp_counter).first);
	}
	this->at(udp_counter).first = data;
	this->at(udp_counter).second = data_length;
	this->counter++;
}


void UdpStack::remove(uint8_t udp_counter, unsigned char **data, size_t &data_length)
{
	*data = this->at(udp_counter).first;
	if(*data)
	{
		this->counter--;
	}
	data_length = this->at(udp_counter).second;
	this->at(udp_counter).first = NULL;
	this->at(udp_counter).second = 0;
}


bool UdpStack::hasNext(uint8_t udp_counter)
{
	auto& value = this->at(udp_counter);
	return (value.first != nullptr && value.second != 0);
}


void UdpStack::reset()
{
	std::vector<std::pair<unsigned char *, size_t> >::iterator it;
	for(it = this->begin(); it != this->end(); ++it)
	{
		if((*it).first)
		{
			delete (*it).first;
			(*it).first = NULL;
			(*it).second = 0;
		}
		this->counter = 0;
	}
}

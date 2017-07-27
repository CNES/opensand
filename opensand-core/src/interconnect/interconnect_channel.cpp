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

/**
 * @file sat_carrier_channel_set.cpp
 * @brief This class implements a TCP channel for interconnecting blocks
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.fr>
 */

#include "interconnect_channel.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <cstring>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <fcntl.h>


/**
 * Constructor
 *
 * @param input         true if the channel accepts incoming data
 * @param output        true if the channel sends data
 */
interconnect_channel::interconnect_channel(bool input, bool output):
	m_input(input),
	m_output(output),
	sock_listen(-1),
	sock_channel(-1),
	send_pos(0),
	recv_size(5*MAX_SOCK_SIZE),
	pkt_remaining(0),
	recv_start(0),
	recv_end(0),
	recv_is_full(false),
	recv_is_empty(true)
{
	// Output log
	this->log_init = Output::registerLog(LEVEL_WARNING, "Interconnect.init");
	this->log_interconnect = Output::registerLog(LEVEL_WARNING,
	                                            "Interconnect.Channel");

}

interconnect_channel::~interconnect_channel()
{
	if (this->sock_listen > 0)
		::close(this->sock_listen); 
	if (this->sock_channel > 0)
		::close(this->sock_channel); 
}

bool interconnect_channel::listen(uint16_t port)
{
	int one = 1;

	// TODO: close or exit if socket is already created

	bzero(&this->m_socketAddr, sizeof(this->m_socketAddr));
	m_socketAddr.sin_family = AF_INET;
	m_socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_socketAddr.sin_port = htons(port); 

	// open the socket
	this->sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(this->sock_listen < 0)
	{
	LOG(this->log_init, LEVEL_ERROR,
	    "Can't open the socket, errno %d (%s)\n",
	    errno, strerror(errno));
	goto error;
	}

	if(setsockopt(this->sock_listen, SOL_SOCKET, SO_REUSEADDR,
	              (char *)&one, sizeof(one)) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Error in reusing addr\n");
		goto error;
	}

	if(setsockopt(this->sock_listen, IPPROTO_TCP, TCP_NODELAY,
	              (char *)&one, sizeof(one)) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to set the socket in no_delay mode: "
		    "%s (%d)\n", strerror(errno), errno);
		goto error;
	}

	// bind socket
	if(bind(this->sock_listen, (struct sockaddr *) &this->m_socketAddr,
	   sizeof(this->m_socketAddr)) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to bind to TCP socket: %s (%d)\n",
		    strerror(errno), errno);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "TCP channel created with local IP %s and local "
	    "port %u\n", inet_ntoa(m_socketAddr.sin_addr),
	    ntohs(m_socketAddr.sin_port));

	if(::listen(this->sock_listen, 1))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to listen on socket: %s (%d)\n",
		    strerror(errno), errno);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "Listening on socket for incoming connections");

	return true;

error:
	LOG(this->log_init, LEVEL_ERROR,
	    "Can't create channel\n");
	return false;
}

bool interconnect_channel::connect(const string ip_addr,
                                   uint16_t port)
{
	int one = 1;

	// TODO: close or exit if socket is already created

	bzero(&this->m_remoteIPAddress, sizeof(this->m_remoteIPAddress));
	m_remoteIPAddress.sin_family = AF_INET;
	m_remoteIPAddress.sin_addr.s_addr = inet_addr(ip_addr.c_str());
	m_remoteIPAddress.sin_port = htons(port); 

	bzero(&this->m_socketAddr, sizeof(this->m_socketAddr));
	m_socketAddr.sin_family = AF_INET;
	m_socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_socketAddr.sin_port = htons(0); 

	// open the socket
	this->sock_channel = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(this->sock_channel < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Can't open the socket, errno %d (%s)\n",
		    errno, strerror(errno));
		goto error;
	}

	if(setsockopt(this->sock_channel, SOL_SOCKET, SO_REUSEADDR,
	   (char *)&one, sizeof(one)) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Error in reusing addr\n");
		goto error;
	}

	if(setsockopt(this->sock_channel, IPPROTO_TCP, TCP_NODELAY,
	   (char *)&one, sizeof(one)) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to set the socket in no_delay mode: "
		    "%s (%d)\n", strerror(errno), errno);
		goto error;
	}

	if(fcntl(this->sock_channel, F_SETFL, O_NONBLOCK))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to set the socket in non blocking mode: "
		    "%s (%d)\n", strerror(errno), errno);
		goto error;
	}

	// bind socket
	if(bind(this->sock_channel, (struct sockaddr *) &this->m_socketAddr,
	        sizeof(this->m_socketAddr)) < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to bind to TCP socket: %s (%d)\n",
		    strerror(errno), errno);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "TCP channel created with local IP %s and local "
	    "port %u\n", inet_ntoa(m_socketAddr.sin_addr),
	    ntohs(m_socketAddr.sin_port));

	while(::connect(this->sock_channel, (struct sockaddr *) &this->m_remoteIPAddress,
	                sizeof(this->m_remoteIPAddress)) < 0)
	{
		if ( errno == ECONNREFUSED ) 
		{
			LOG(this->log_init, LEVEL_DEBUG,
			    "connection refused \n");
			continue;
		}
		if ( errno == EINPROGRESS )
		{
			LOG(this->log_init, LEVEL_DEBUG,
			    "connection in progress \n");
		}
		usleep((useconds_t) 100000);
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "TCP connection established with remote IP %s and remote "
	    "port %u\n", inet_ntoa(m_remoteIPAddress.sin_addr),
	    ntohs(m_remoteIPAddress.sin_port));

	return true;

error:
	LOG(this->log_init, LEVEL_ERROR,
	    "Can't create channel\n");
	return false;
}

int interconnect_channel::send(const unsigned char *data,
                                size_t length, uint8_t type, bool flush)
{
	ssize_t slen = 0;
	unsigned short type_len = sizeof(type);
	size_t total_len = length + type_len;
	unsigned short length_len = sizeof(total_len);
	int ret;
	fd_set WriteFDs;

	// check that the channel sends data
	if(!this->isOutputOk())
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "this channel is not configured to send data\n");
		goto error;
	}

	// NOTE: don't store in buffer. can saturate other blocks
	// check if the socket is open
	if(this->sock_channel <= 0)
	{
		return 0;
	}

	if(!flush && !(type == 0 && length == 0))
	{
		// Check if there is enough space in buffer
		if (this->recv_size - this->send_pos < length + type_len)
		{
			LOG(this->log_interconnect, LEVEL_WARNING,
			    "not enough space in send buffer, discard packet\n");
			return 1;
		}

		// add length and type fields
		memcpy(this->send_buffer + this->send_pos, &total_len, length_len);
		this->send_pos += length_len;
		memcpy(this->send_buffer + this->send_pos, &type, type_len);
		this->send_pos += type_len;
		memcpy(this->send_buffer + this->send_pos, data, length);
		this->send_pos += length;
	}

	if (this->send_pos == 0)
		return 0;

	FD_ZERO(&WriteFDs);
	FD_SET(this->sock_channel, &WriteFDs);

	// check if socket is ready to send
	ret = ::select(this->sock_channel + 1, NULL, &WriteFDs, NULL, 0);
	if (ret > 0)
	{
		slen = ::send(this->sock_channel, this->send_buffer, this->send_pos, MSG_NOSIGNAL);
	}
	else if (!ret)
	{
		// cannot send right now, but stored in buffer
		return 1;
	}
	else
	{
		if ( errno == EBADF )
			goto close_connection;
		goto error;
	}
	if( slen < 0)
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "Error: send errno %s (%d)\n",
		    strerror(errno), errno);
		if ( errno == EPIPE || errno == ECONNRESET )
			goto close_connection;
		goto error;
	}
	else if (slen == 0)
	{
		goto close_connection;
	}
	else
	{
		this->send_pos -= slen;
		LOG(this->log_interconnect, LEVEL_INFO,
		    "==> Interconnect_Send: len=%zd\n",
		    slen);
	}
	return 0;
close_connection:
	return -1;
error:
	return 1;

}

int interconnect_channel::receive(NetSocketEvent *const event)
{
	unsigned short length_len = sizeof(this->pkt_remaining);
	size_t recv_len;
	size_t recv_pos = 0;
	size_t data_len;
	int ret;
	unsigned char *data;

	LOG(this->log_interconnect, LEVEL_INFO,
	    "try to receive a packet from interconnect channel");

	// the channel fd must be valid
	if(this->sock_channel < 0)
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "socket not opened !\n");
		goto error;
	}

	// error if channel doesn't accept incoming data
	if(!this->isInputOk())
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "channel does not accept data\n");
		goto error;
	}

	data = event->getData();
	recv_len = event->getSize();
	data_len = recv_len;

	// TODO: this won't work if the lenght is not completely
	// received in one packet. 
	while (data_len > 0)
	{
		// if no packet is partially stored
		if(this->pkt_remaining == 0)
		{
			if(data_len < length_len)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "no enough data received to read the "
				    "packet length.");
				goto free;
			}
			ret = this->storeData(data+recv_pos, length_len);
			if(ret < 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "not enough space in receive buffer to "
				    "store data. Discarding packet.");
					goto discard;
			}
			memcpy(&this->pkt_remaining, data+recv_pos, length_len);
			recv_pos += ret;
			data_len -= ret;
		}
		// check if packet can be completed
		if(data_len>=this->pkt_remaining)
		{
			ret = this->storeData(data+recv_pos, this->pkt_remaining);
			if(ret < 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "not enough space in receive buffer to "
				    "store data. Discarding packet.");
				goto discard;
			}
		}
		else
		{
			ret = this->storeData(data+recv_pos, data_len);
			if(ret < 0)
			{
				LOG(this->log_interconnect, LEVEL_ERROR,
				    "not enough space in receive buffer to "
				    "store data. Discarding packet.");
				goto discard;
			}
		}
		recv_pos += ret;
		data_len -= ret;
		this->pkt_remaining -= ret;
	}
	LOG(this->log_interconnect, LEVEL_INFO,
	    "successfully stored %zu bytes in receive buffer.",
	    recv_len);
	free(data);
	return 0;
discard:
	discardPacket();
free:
	free(data);
error:
	return -1; 
}

/**
 * Store data in buffer
 *
 * @return true if data was stored
 */
// TODO: read returns of memcpy function
ssize_t interconnect_channel::storeData(const unsigned char *data,
                                     size_t len)
{
	size_t aux;

	if (len == 0)
		return 0;

	if(len > this->getFreeSpace())
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "not enough space in recv for storing data, "
		    "discard packet.");
		return -1;
	}
	if(this->recv_end >= this->recv_start)
	{
		aux = this->recv_size - this->recv_end;
		if (aux >= len)
		{
			memcpy(this->recv_buffer + this->recv_end, data, len);
		}
		else
		{
			memcpy(this->recv_buffer + this->recv_end, data, aux);
			memcpy(this->recv_buffer, data + aux, len-aux);
		}
	}
	else
	{
		memcpy(this->recv_buffer + this->recv_end, data, len);
	}
	this->recv_end = (this->recv_end + len)%(this->recv_size);
	this->recv_is_empty = false;
	if (this->recv_end == this->recv_start)
		this->recv_is_full = true;
	return (int) len;
}


/**
 * Get if the channel accepts input
 * @return true if channel accepts input
 */
bool interconnect_channel::isInputOk()
{
	return (m_input);
}

/**
 * Get if the channel accepts output
 * @return true if channel accepts output
 */
bool interconnect_channel::isOutputOk()
{
	return (m_output);
}

/**
 * Get available space in recv buffer
 * @return Number of bytes free on recv_buffer
 */
size_t interconnect_channel::getFreeSpace()
{
	size_t free_space;
	if (recv_is_full)
		free_space = 0;
	else if (recv_is_empty)
		free_space = recv_size;
	else if (recv_start >= recv_end)
		free_space = recv_start - recv_end;
	else
		free_space = ( recv_start + recv_size ) - recv_end;
	return (free_space);
}

/**
 * Get used space in recv buffer
 * @return Number of bytes used on recv_buffer
 */
size_t interconnect_channel::getUsedSpace()
{
	return (recv_size - this->getFreeSpace());
}

/**
 * Receive packet from buffer
 *
 * @param buf            Pointer to char buffer
 * @param data_len        Length of the received packet
 * @return True if a packet was retreived, false otherwise.
 */
bool interconnect_channel::getPacket(unsigned char **buf,
                                     size_t &data_len,
                                     uint8_t &type)
{
	size_t pkt_len;
	uint8_t pkt_type;
	ssize_t ret;
	size_t recv_start_tmp = this->recv_start;
	unsigned int length_len=sizeof(this->pkt_remaining);
	unsigned int type_len=sizeof(type);

	if (this->recv_is_empty)
		goto no_pkt;

	// Fetch packet length    
	if (this->getUsedSpace() < length_len)
		goto error;

	ret = this->readData((unsigned char *) &pkt_len, length_len, recv_start_tmp);
	// TODO: check for any errors?
	recv_start_tmp = ret; 

	// Check if packet lenght exceeds used buffer space
	if (pkt_len > this->getUsedSpace()-length_len)
	{
		// If no pkt is being received, there's a problem
		if (this->pkt_remaining == 0)
			goto error;
		goto no_pkt;
	}

	// Remove packet type from data
	ret = this->readData((unsigned char *) &pkt_type, type_len, recv_start_tmp);
	recv_start_tmp = ret;
	pkt_len -= type_len;

	(*buf) = NULL;
	if (pkt_len > 0)
	{
		// Allocate memory for buffer;
		(*buf) = (unsigned char *)calloc(pkt_len, sizeof(unsigned char));
		// Receive packet data
		ret = this->readData((*buf), pkt_len, recv_start_tmp);
	}
	// Refresh start position
	this->recv_start = ret;

	if ((length_len + type_len + pkt_len) > 0)
		this->recv_is_full = false;

	if (this->recv_start == this->recv_end)
		this->recv_is_empty = true; 

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "fetched packet of %zu bytes\n", pkt_len);

	data_len = pkt_len;
	type = pkt_type;
	return true;
error:
	discardPacket();
no_pkt:
	return false;
}

/**
 * Read data from recv_buffer into buffer
 */
ssize_t interconnect_channel::readData(unsigned char *buf,
                                       size_t len,
                                       size_t start)
{
	size_t aux;

	if ( start + len > this->recv_size)
	{
		aux = this->recv_size - start;
		memcpy(buf, this->recv_buffer + start, aux);
		memcpy(buf + aux, this->recv_buffer, len - aux);
	}
	else
	{
		memcpy(buf, this->recv_buffer + start, len);
	}

	LOG(this->log_interconnect, LEVEL_DEBUG,
	    "read %zu bytes from recv buffer\n", len);
	return ( (start + len) % this->recv_size );
}

/**
 * Discard incomplete packet, by chaging recv_end
 * position.
 */
void interconnect_channel::discardPacket()
{
	size_t pos_a = this->recv_start;
	size_t pos_b = this->recv_start;
	size_t pkt_size;
	unsigned int length_len=sizeof(this->pkt_remaining);

	// No enough space even for the length
	if (this->recv_is_empty)
		return;

	if (this->getUsedSpace() < length_len)
	{
		this->recv_end = this->recv_start;
		this->recv_is_empty = true;
		return;
	}

	do
	{
		memcpy(&pkt_size, this->recv_buffer + pos_a, length_len);
		pos_b = pos_a + length_len  + pkt_size;
		// check if pos_b is unused zone
		if (this->recv_start >= this->recv_end)
		{
			if (pos_b > this->recv_end + this->recv_size)
			{
				this->recv_end = pos_a;
				if (this->recv_start == this->recv_end)
					this->recv_is_empty = true;
				this->recv_is_full = false;
				LOG(this->log_interconnect, LEVEL_DEBUG,
				    "discarded incomplete packet\n");
				return;
			}
		}
		else
		{
			if (pos_b > this->recv_end)
			{
				this->recv_end = pos_a;
				if (this->recv_start == this->recv_end)
					this->recv_is_empty = true;
				this->recv_is_full = false;
				LOG(this->log_interconnect, LEVEL_DEBUG,
				    "discarded incomplete packet\n");
				return;
			}
		}
		pos_a = pos_b % this->recv_size; 
	}
	while (pos_a != this->recv_end);
}

/**
 * Set the channel socket
 *
 * @param sock  The socket for the channel
 */
void interconnect_channel::setChannelSock(int sock)
{
	this->sock_channel = sock;
}

/**
 * Set the channel socket to blocking mode
 *
 * @return true if could perform the task.
 */
bool interconnect_channel::setSocketBlocking()
{
	int flags;

	if (!this->isConnected())
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "socket is not connected, cannot set"
		    " to blocking mode\n");
		return false;
	}

	// Get the current flags
	flags = fcntl(this->sock_channel, F_GETFL);
	if(fcntl(this->sock_channel, F_SETFL, flags & (~O_NONBLOCK)))
	{
		LOG(this->log_interconnect, LEVEL_ERROR,
		    "failed to set the socket on blocking mode\n");
		return false;
	}
	return true;
}

/**
 * Get socket channel fd
 *
 * @return channel fd.
 */
int interconnect_channel::getFd()
{
	return this->sock_channel;
}

/**
 * Get listen socket fd
 *
 * @return listen fd.
 */
int interconnect_channel::getListenFd()
{
	return this->sock_listen;
}

/**
 * Check if socket is active
 *
 * @return true if connection opened
 */
bool interconnect_channel::isClosed()
{
	fd_set rfd;
	FD_ZERO(&rfd);
	FD_SET(this->sock_channel, &rfd);
	timeval tv = { 0, 0 };
	::select(this->sock_channel+1, &rfd, 0, 0, &tv);
	if (!FD_ISSET(this->sock_channel, &rfd))
		return false;
	int n = 0;
	ioctl(this->sock_channel, FIONREAD, &n);
	return (n == 0);
}

void interconnect_channel::close()
{
	::close(this->sock_channel);
	this->sock_channel = -1;
	LOG(this->log_interconnect, LEVEL_INFO,
	    "closed interconnect socket\n");
}

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @brief This implements a set of satellite carrier channels
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.fr>
 */

#include "interconnect_channel_test.h"
#include "Config.h"

#include <opensand_output/Output.h>

#include <cstring>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/tcp.h>

/**
 * Constructor
 *
 * @param input         true if the channel accepts incoming data
 * @param output        true if the channel sends data
 */
interconnect_channel_test::interconnect_channel_test(bool input, bool output):
    m_input(input),
    m_output(output),
    sock_listen(-1),
    sock_channel(-1),
    send_pos(0),
    recv_size(10*MAX_SOCK_SIZE),
    pkt_remaining(0),
    recv_start(0),
    recv_end(0),
    recv_is_full(false),
    recv_is_empty(true)
{
    // Output log
	//this->log_init = Output::registerLog(LEVEL_WARNING, "Interconnect.init");
	//this->log_interconnect = Output::registerLog(LEVEL_WARNING,
	//                                            "Interconnect.Channel");

}

interconnect_channel_test::~interconnect_channel_test()
{
    if (this->sock_listen > 0)
        close(this->sock_listen); 
    if (this->sock_channel > 0)
        close(this->sock_channel); 
}

bool interconnect_channel_test::listen(uint16_t port)
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
        printf("Can't open the socket, errno %d (%s)\n",
            errno, strerror(errno));
        goto error;
    }

    if(setsockopt(this->sock_listen, SOL_SOCKET, SO_REUSEADDR,
                (char *)&one, sizeof(one)) < 0)
    {
        printf("Error in reusing addr\n");
        goto error;
    }

    if(setsockopt(this->sock_listen, IPPROTO_TCP, TCP_NODELAY,
                (char *)&one, sizeof(one)) < 0)
    {
        printf("failed to set the socket in no_delay mode: "
                "%s (%d)\n", strerror(errno), errno);
        goto error;
    }

    //if(fcntl(this->sock_listen, F_SETFL, O_NONBLOCK))
    //{
    //    LOG(this->log_init, LEVEL_ERROR,
    //            "failed to set the socket in non blocking mode: "
    //            "%s (%d)\n", strerror(errno), errno);
    //    goto error;
    //}

    // TODO: get network interface index??

    // bind socket
    if(bind(this->sock_listen, (struct sockaddr *) &this->m_socketAddr,
               sizeof(this->m_socketAddr)) < 0)
    {
       printf("failed to bind to TCP socket: %s (%d)\n",
             strerror(errno), errno);
      goto error;
    }

    printf("TCP channel created with local IP %s and local "
         "port %u\n", inet_ntoa(m_socketAddr.sin_addr),
         ntohs(m_socketAddr.sin_port));

    if(::listen(this->sock_listen, 1))
    {
        printf("failed to listen on socket: %s (%d)\n",
                strerror(errno), errno);
        goto error;
    }
    
    printf("listening on socket for incoming connections");

    return true;

error:
   printf("Can't create channel\n");
   return false;
}

bool interconnect_channel_test::connect(const string ip_addr,
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
        printf("Can't open the socket, errno %d (%s)\n",
            errno, strerror(errno));
        goto error;
    }

    if(setsockopt(this->sock_channel, SOL_SOCKET, SO_REUSEADDR,
                (char *)&one, sizeof(one)) < 0)
    {
        printf("Error in reusing addr\n");
        goto error;
    }

    if(setsockopt(this->sock_channel, IPPROTO_TCP, TCP_NODELAY,
                (char *)&one, sizeof(one)) < 0)
    {
        printf("failed to set the socket in no_delay mode: "
                "%s (%d)\n", strerror(errno), errno);
        goto error;
    }

    // TODO: remove this
    //if(fcntl(this->sock_listen, F_SETFL, O_NONBLOCK))
    //{
    //    LOG(this->log_init, LEVEL_ERROR,
    //            "failed to set the socket in non blocking mode: "
    //            "%s (%d)\n", strerror(errno), errno);
    //    goto error;
    //}

    // TODO: get network interface index??

    // bind socket
    if(bind(this->sock_channel, (struct sockaddr *) &this->m_socketAddr,
               sizeof(this->m_socketAddr)) < 0)
    {
       printf("failed to bind to TCP socket: %s (%d)\n",
             strerror(errno), errno);
      goto error;
    }

    printf("TCP channel created with local IP %s and local "
         "port %u\n", inet_ntoa(m_socketAddr.sin_addr),
         ntohs(m_socketAddr.sin_port));

    if(::connect(this->sock_channel, (struct sockaddr *) &this->m_remoteIPAddress,
                    sizeof(this->m_remoteIPAddress)) < 0)
    {
        printf("failed to listen on socket: %s (%d)\n",
                strerror(errno), errno);
        goto error;
    }
    
    printf("TCP connection established with remote IP %s and remote "
         "port %u\n", inet_ntoa(m_remoteIPAddress.sin_addr),
         ntohs(m_remoteIPAddress.sin_port));

    return true;

error:
   printf("Can't create channel\n");
   return false;
}


bool interconnect_channel_test::send(const unsigned char *data,
                                   size_t length)
{
	int slen;
    unsigned short length_len = sizeof(length);
	
    // check that the channel sends data
    if(!this->isOutputOk())
    {
        printf("this channel is not configured to send data\n");
        goto error;
    }

    // TODO: any other way to send first the size?, two calls to send?
    // add length field
    // bzero(this->send_buffer, sizeof(this->send_buffer));
    // TODO: check that send_buffer doesnt overflow!!!!
    memcpy(this->send_buffer + this->send_pos, &length, length_len);
    this->send_pos += length_len;
    memcpy(this->send_buffer + this->send_pos, data, length);
    this->send_pos += length;
    
    // check if the socket is open
    if(this->sock_channel <= 0)
    {
        printf("cannot send %zu bytes of data through channel: "
               "connection not established", length);
        goto error;
    }

    slen = ::send(this->sock_channel, this->send_buffer, this->send_pos, 0);

    if( slen < 0)
    {
        printf("Error: send errno %s (%d)\n",
                strerror(errno), errno);
        goto error;
    }
    
    if (DEBUG)
        printf("==> Interconnect_Send: len=%d\n", slen);

    if ( slen > 0)
    {
        this->send_pos -= slen;
    }
    
    return true;

error:
    return false;
}


int interconnect_channel_test::receive(NetSocketEvent *const event)
{
    unsigned short length_len = sizeof(this->pkt_remaining);
    size_t recv_len;
    size_t recv_pos = 0;
    size_t data_len;
    int ret;
    unsigned char *data;

    if (DEBUG)
        printf("try to receive a packet from interconnect channel");

    // the channel fd must be valid
    if(this->sock_channel < 0)
    {
        printf("socket not opened !\n");
        goto error;
    }

    // error if channel doesn't accept incoming data
    if(!this->isInputOk())
    {
        printf("channel does not accept data\n");
        goto error;
    }

    data = event->getData();
    recv_len = event->getSize();
    //printf("total len!! : %zu\n", recv_len);
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
               printf("no enough data received to read the "
                       "packet length.");
               goto free;
           }
           //printf("(%zu->%zu/%zu):", recv_pos, recv_pos+2*length_len, recv_len);
           //for(int i=0; i<2*length_len; i++) printf(" %02X", *(data+recv_pos+i));
           //printf("\n");
           ret = this->storeData(data+recv_pos, length_len);
           if(ret < 0)
           {
               printf("not enough space in receive buffer to "
                       "store data. Discarding packet.");
               goto discard;
           }
           memcpy(&(this->pkt_remaining), data+recv_pos, length_len);
           recv_pos += ret;
           data_len -= ret;
       }
       // check if packet can be completed
       if(data_len>=this->pkt_remaining)
       {
           ret = this->storeData(data+recv_pos, this->pkt_remaining);
           if(ret < 0)
           {
               printf("not enough space in receive buffer to "
                       "store data. Discarding packet.");
               goto discard;
           }
       }
       else
       {
           ret = this->storeData(data+recv_pos, data_len);
           if(ret < 0)
           {
               printf("not enough space in receive buffer to "
                       "store data. Discarding packet.");
               goto discard;
           }
       }
       recv_pos += ret;
       data_len -= ret;
       this->pkt_remaining -= ret;
    }
    if (DEBUG)
        printf("successfully stored %zu bytes in receive buffer.",
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
int interconnect_channel_test::storeData(const unsigned char *data,
                                     size_t len)
{
    size_t aux;

    if (len == 0)
    {
        return 0;
    }

    if(len > this->getFreeSpace())
    {
        printf("not enough space in recv for storing data, "
            "discard packet.");
        return -1;
    }
    if(this->recv_end > this->recv_start)
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
bool interconnect_channel_test::isInputOk()
{
    return (m_input);
}

/**
 * Get if the channel accepts output
 * @return true if channel accepts output
 */
bool interconnect_channel_test::isOutputOk()
{
    return (m_output);
}

/**
 * Get available space in recv buffer
 * @return Number of bytes free on recv_buffer
 */
size_t interconnect_channel_test::getFreeSpace()
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
size_t interconnect_channel_test::getUsedSpace()
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
bool interconnect_channel_test::getPacket(unsigned char **buf,
                                     size_t &data_len)
{
    size_t pkt_len;
    size_t aux;
    unsigned int length_len=sizeof(this->pkt_remaining);

    if (this->recv_is_empty)
        goto no_pkt;

    // Fetch packet length    
    if (this->getUsedSpace() < length_len)
    {
        goto error;
    }

    if (DEBUG)
        printf("used space: %zu\n", this->getUsedSpace()); 

    memcpy(&pkt_len, this->recv_buffer + this->recv_start, length_len);
    
    // Check if packet lenght exceeds used buffer space
    if (pkt_len > this->getUsedSpace()-length_len)
    {
        // If no pkt is being received, or not this one, there's a problem
        if (this->pkt_remaining == 0) //|| pkt_len != this->pkt_remaining)
        {
            goto error;
        }
        goto no_pkt;
    }
    // Allocate memory for buffer;
    (*buf) = (unsigned char *)calloc(pkt_len, sizeof(unsigned char));
    
    // Copy data to buffer
    if (this->recv_start + length_len + pkt_len > this->recv_size)
    {
        aux = this->recv_size - (this->recv_start + length_len);
        memcpy((*buf), this->recv_buffer + this->recv_start + length_len,
               aux);
        memcpy((*buf)+aux, this->recv_buffer, pkt_len - aux); 
    }
    else
    {
        memcpy((*buf), this->recv_buffer + this->recv_start + length_len,
               pkt_len);
    }
    // Refresh start position
    //printf("antes: %zu/%zu", this->recv_start, this->recv_end);
    this->recv_start = (this->recv_start + length_len + pkt_len) % 
                        this->recv_size;
    //printf("  dsp: %zu/%zu\n", this->recv_start, this->recv_end);
   
    if ((length_len + pkt_len) > 0)
        this->recv_is_full = false;

    if (this->recv_start == this->recv_end)
       this->recv_is_empty = true; 
    
    if (DEBUG)
        printf("fetched packet of %zu bytes\n", pkt_len);

    data_len = pkt_len;
    return true;
error:
    discardPacket();
no_pkt:
    return false;
}

/**
 * Discard incomplete packet, by chaging recv_end
 * position.
 */
void interconnect_channel_test::discardPacket()
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
                printf("discarded incomplete packet\n");
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
                printf("discarded incomplete packet\n");
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
void interconnect_channel_test::setChannelSock(int sock)
{
    this->sock_channel = sock;
}

/**
 * Get socket channel fd
 *
 * @return channel fd.
 */
int interconnect_channel_test::getFd()
{
    return this->sock_channel;
}

/**
 * Get listen socket fd
 *
 * @return listen fd.
 */
int interconnect_channel_test::getListenFd()
{
    return this->sock_listen;
}

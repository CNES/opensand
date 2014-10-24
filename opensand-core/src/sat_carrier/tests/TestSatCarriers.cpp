/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file TestSatCarriers.cpp
 * @brief Test of channels with a bidirectionnal flow 
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * 
 * 
 *
 *  +-------------------------------+   +-------------------------------+
 *  |       +---------------+       |   |       +---------------+       |
 *  | +-----|-----+   +-----+-----+ |   | +-----|-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |   | |     |     |   |     |     | |
 *  | |     |     |   |     |     | |   | |     |     |   |     |     | |
 *  | |     |     |   |     |     | |   | |     |     |   |     |     | |
 *  | +-----+-----+   +-----+-----+ |   | +-----+-----+   +-----+-----+ |
 *  +------TUN--------------+-------+   +-------+--------------TUN------+
 *          |               +-------------------+               |
 *      iperf s/c              OpenSAND Channel             iperf s/c
 */


#include "TestSatCarriers.h"

#include "Data.h"
#include "OpenSandCore.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_tun.h>


#define TUNTAP_FLAGS_LEN 4 // Flags [2 bytes] + Proto [2 bytes]

static bool allocTun(int &fd)
{
	struct ifreq ifr;
	int err;

	fd = open("/dev/net/tun", O_RDWR);
	if(fd < 0)
	{
		fprintf(stderr, "cannot open '/dev/net/tun': %s\n",
		        strerror(errno));
		return false;
	}

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */

	/* create TUN or TAP interface */
	snprintf(ifr.ifr_name, IFNAMSIZ, "opensand_tun");
	ifr.ifr_flags = (IFF_TUN);

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if(err < 0)
	{
		fprintf(stderr, "cannot set flags on file descriptor %s\n",
		        strerror(errno));
		close(fd);
		return false;
	}

	return true;
}

enum
{
	from_lan,
	from_udp
};

// TODO WARNING MEMORY !!!

/**
 * Constructor
 */
TestSatCarriers::TestSatCarriers(const string &name,
                                 struct sc_specific UNUSED(specific)):
	Block(name)
{
}

TestSatCarriers::~TestSatCarriers()
{
}


// TODO remove this functions once every channels will be correctly separated
bool TestSatCarriers::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}

bool TestSatCarriers::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// Lan message to be sent on channel
			if(((MessageEvent *)event)->getMessageType() == from_lan)
			{
				Data *packet = (Data *)((MessageEvent *)event)->getData();

				if(!this->out_channel_set.send(1,
				                               packet->data(),
				                               packet->length()))
				{
					fprintf(stderr, "error when sending data\n");
					return false;
				}
				delete packet;
				break;
			}
			// UDP message to be transmitted on network
			else if(((MessageEvent *)event)->getMessageType() == from_udp)
			{
				Data *packet = (Data *)((MessageEvent *)event)->getData();
				unsigned char head[TUNTAP_FLAGS_LEN] = {0, 0, 8, 0};
				packet->insert(0, head, TUNTAP_FLAGS_LEN);
				if(write(this->fd, packet->data(), packet->length()) < 0)
				{
					fprintf(stderr, "Unable to write data on tun interface: %s\n",
					        strerror(errno));
					return false;
				}
				delete packet;
				break;
			}
		}
		// do not break if none of the above

		default:
			fprintf(stderr, "unknown event received %s", event->getName().c_str());
			return false;
	}
	return true;
}


bool TestSatCarriers::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}

bool TestSatCarriers::Upward::onEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case evt_net_socket:
		{
			// event on UDP channel
			// Data to read in Sat_Carrier socket buffer
			size_t length;
			unsigned char *buf = NULL;

			unsigned int carrier_id;
			int ret;

			// for UDP we need to retrieve potentially desynchronized
			// datagrams => loop on receive function
			do
			{
				ret = this->in_channel_set.receive((NetSocketEvent *)event,
				                                    carrier_id,
				                                    &buf, length);
				if(ret < 0)
				{
					fprintf(stderr, "failed to receive data on any "
					        "input channel (code = %zu)\n", length);
					status = false;
				}
				else
				{
					if(length > 0)
					{
						Data *packet = new Data(buf, length);
						free(buf);

						if(!this->shareMessage((void **)(&packet), length, from_udp))
						{
							fprintf(stderr,
							        "failed to send packet from carrier %u to opposite layer\n",
							        carrier_id);
							return false;
						}
					}
				}
			} while(ret > 0);
		}
		break;
		case evt_file:
		{
			unsigned char *read_data;
			const unsigned char *data;
			unsigned int length;
			Data *packet;

			// read  data received on tun/tap interface
			length = ((NetSocketEvent *)event)->getSize() - TUNTAP_FLAGS_LEN;
			read_data = ((NetSocketEvent *)event)->getData();
			data = read_data + TUNTAP_FLAGS_LEN;

			packet = new Data((unsigned char *)data, length);
			free(read_data);

			if(!this->shareMessage((void **)&packet, length, from_lan))
			{
				fprintf(stderr, "failed to send burst to opposite channel\n");
				return false;
			}
		}
		break;

		default:
			fprintf(stderr, "unknown event received %s", event->getName().c_str());
			return false;
	}

	return status;
}


bool TestSatCarriers::onInit(void)
{
	int fd = -1;
	// create TUN or TAP virtual interface
	if(!allocTun(fd))
	{
		return false;
	}

	// we can share FD as one thread will write, the second will read
	((Upward *)this->upward)->setFd(fd);
	((Downward *)this->downward)->setFd(fd);

	return true;
}

bool TestSatCarriers::Upward::onInit(void)
{
	vector<sat_carrier_udp_channel *>::iterator it;
	sat_carrier_udp_channel *channel;

	// initialize all channels from the configuration file
	if(!this->in_channel_set.readInConfig(this->ip_addr,
	                                      this->interface_name))
	{
		fprintf(stderr, "Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(it = this->in_channel_set.begin(); it != this->in_channel_set.end(); it++)
	{
		channel = *it;

		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			ostringstream name;

			name << "Channel_" << channel->getChannelID();
			this->addNetSocketEvent(name.str(),
			                        channel->getChannelFd(),
			                        9000);
		}
	}

	return true;
}

bool TestSatCarriers::Downward::onInit()
{
	// initialize all channels from the configuration file
	if(!this->out_channel_set.readOutConfig(this->ip_addr,
	                                        this->interface_name))
	{
		fprintf(stderr, "Wrong channel set configuration\n");
		return false;
	}

	return true;
}

void TestSatCarriers::Downward::setFd(int fd)
{
	this->fd = fd;
}

void TestSatCarriers::Upward::setFd(int fd)
{
	// add file descriptor for TUN/TAP interface
	this->addFileEvent("tun/tap", fd, 9000 + 4);
}



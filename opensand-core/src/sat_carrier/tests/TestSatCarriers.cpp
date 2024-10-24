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
 * @file TestSatCarriers.cpp
 * @brief Test of channels with a bidirectionnal flow 
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
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

#include "OpenSandCore.h"

#include <sstream>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_tun.h>
#include <cstring>
#include <errno.h>

#include <opensand_rt/FileEvent.h>
#include <opensand_rt/MessageEvent.h>
#include <opensand_rt/NetSocketEvent.h>


constexpr const std::size_t TUNTAP_FLAGS_LEN = 4;  // Flags [2 bytes] + Proto [2 bytes]


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

enum class Origin: uint8_t
{
	from_lan,
	from_udp
};

// TODO WARNING MEMORY !!!

/**
 * Constructor
 */
Rt::DownwardChannel<TestSatCarriers>::DownwardChannel(const std::string& name, sc_specific specific):
	Channels::Downward<DownwardChannel<TestSatCarriers>>{name},
	out_channel_set{specific.tal_id},
	ip_addr{specific.ip_addr}
{
}


Rt::UpwardChannel<TestSatCarriers>::UpwardChannel(const std::string& name, sc_specific specific):
	Channels::Upward<UpwardChannel<TestSatCarriers>>{name},
	in_channel_set{specific.tal_id},
	ip_addr{specific.ip_addr}
{
}


bool Rt::DownwardChannel<TestSatCarriers>::onEvent(const Event& event)
{
	fprintf(stderr, "unknown event received %s", event.getName().c_str());
	return false;
}


bool Rt::DownwardChannel<TestSatCarriers>::onEvent(const MessageEvent& event)
{
	Ptr<Data> packet = event.getMessage<Data>();
	std::size_t length = packet->length();

	switch(to_enum<Origin>(event.getMessageType()))
	{
		case Origin::from_lan:
			{
				if(!this->out_channel_set.send(1, packet->data(), length))
				{
					fprintf(stderr, "error when sending data\n");
					return false;
				}
				break;
			}
		case Origin::from_udp:
			{
				unsigned char head[TUNTAP_FLAGS_LEN] = {0, 0, 8, 0};
				packet->insert(0, head, TUNTAP_FLAGS_LEN);
				if(write(this->fd, packet->data(), length) < 0)
				{
					fprintf(stderr, "Unable to write data on tun interface: %s\n",
					        strerror(errno));
					return false;
				}
				break;
			}
		default:
			return this->onEvent(static_cast<const Event&>(event));
	}

	return true;
}

bool Rt::UpwardChannel<TestSatCarriers>::onEvent(const Event& event)
{
	fprintf(stderr, "unknown event received %s", event.getName().c_str());
	return false;
}


bool Rt::UpwardChannel<TestSatCarriers>::onEvent(const FileEvent& event)
{
	// read  data received on tun/tap interface
	std::size_t length = event.getSize() - TUNTAP_FLAGS_LEN;
	Data read_data = event.getData();
	Ptr<Data> packet = make_ptr<Data>(read_data, TUNTAP_FLAGS_LEN, length);

	if(!this->shareMessage(std::move(packet), to_underlying(Origin::from_lan)))
	{
		fprintf(stderr, "failed to send burst to opposite channel\n");
		return false;
	}

	return true;
}


bool Rt::UpwardChannel<TestSatCarriers>::onEvent(const NetSocketEvent& event)
{
	UdpChannel::ReceiveStatus ret;

	// event on UDP channel
	// Data to read in Sat_Carrier socket buffer

	// for UDP we need to retrieve potentially desynchronized
	// datagrams => loop on receive function
	do
	{
		unsigned int carrier_id;
		spot_id_t spot_id;
		Ptr<Data> buf = make_ptr<Data>(nullptr);
		ret = this->in_channel_set.receive(event, carrier_id, spot_id, buf);

		std::size_t length = buf->length();
		if(ret == UdpChannel::ERROR)
		{
			fprintf(stderr, "failed to receive data on any "
			        "input channel (code = %zu)\n", length);
			return false;
		}
		else
		{
			if(length > 0)
			{
				if(!this->shareMessage(std::move(buf), to_underlying(Origin::from_udp)))
				{
					fprintf(stderr,
							"failed to send packet from carrier %u to opposite layer\n",
							carrier_id);
					return false;
				}
			}
		}
	} while(ret == UdpChannel::STACKED);

	return true;
}


bool TestSatCarriers::onInit()
{
	int fd = -1;
	// create TUN or TAP virtual interface
	if(!allocTun(fd))
	{
		return false;
	}

	// we can share FD as one thread will write, the second will read
	this->upward.setFd(fd);
	this->downward.setFd(fd);

	return true;
}

bool Rt::UpwardChannel<TestSatCarriers>::onInit()
{
	// initialize all channels from the configuration file
	if(!this->in_channel_set.readInConfig(this->ip_addr, Component::unknown, 0))
	{
		fprintf(stderr, "Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for (auto &&channel: this->in_channel_set)
	{
		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			std::ostringstream name;

			name << "Channel_" << channel->getChannelID();
			this->addNetSocketEvent(name.str(),
			                        channel->getChannelFd(),
			                        9000);
		}
	}

	return true;
}

bool Rt::DownwardChannel<TestSatCarriers>::onInit()
{
	// initialize all channels from the configuration file
	if (!this->out_channel_set.readOutConfig(this->ip_addr, Component::unknown, 0))
	{
		fprintf(stderr, "Wrong channel set configuration\n");
		return false;
	}

	return true;
}

void Rt::DownwardChannel<TestSatCarriers>::setFd(int fd)
{
	this->fd = fd;
}

void Rt::UpwardChannel<TestSatCarriers>::setFd(int fd)
{
	// add file descriptor for TUN/TAP interface
	this->addFileEvent("tun/tap", fd, 9000 + 4);
}



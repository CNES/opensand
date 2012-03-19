/**
 * @file sat_carrier_channel.h
 * @brief This implements a bloc satellite carrier channel
 * @author AQL (ame)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef SATCAR_CHANNEL_H
#define SATCAR_CHANNEL_H

#include <vector>
#include <linux/if_packet.h>
#include <net/if.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "platine_conf/conf.h"

// margouilla includes
#include "platine_margouilla/mgl_socket.h"

/**
 * @class sat_carrier_channel
 * @brief This implements a bloc satellite carrier channel
 */
class sat_carrier_channel
{
 public:

	sat_carrier_channel(unsigned int channelID, bool input, bool output);

	virtual ~sat_carrier_channel() = 0;

	unsigned int getChannelID();

	virtual int getChannelFd() = 0;

	bool isInputOk();

	bool isOutputOk();

	virtual int send(unsigned char *buf, unsigned int len) = 0;
	virtual int receive(unsigned char *buf, unsigned int *data_len,
	                    unsigned int max_len, long timeout) = 0;

	static int getIfIndex(const char *name);

 protected:

	/// the ID of the channel
	int m_channelID;

	/// if channel accept input
	bool m_input;

	/// if channel accept output
	bool m_output;
};

#endif

/**
 * @file sat_carrier_channel_set.h
 * @brief This implements a set of satellite carrier channels
 * @author AQL (Antoine)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef SAT_CARRIER_CHANNEL_SET_H
#define SAT_CARRIER_CHANNEL_SET_H

#include <vector>
#include <net/if.h>

#include "sat_carrier_channel.h"
#include "sat_carrier_eth_channel.h"
#include "sat_carrier_udp_channel.h"
#include "platine_conf/conf.h"

/**
 * @class sat_carrier_channel_set
 * @brief This implements a set of satellite carrier channels
 */
class sat_carrier_channel_set: public std::vector < sat_carrier_channel * >
{
 public:

	sat_carrier_channel_set();
	~sat_carrier_channel_set();

	int readConfig();

	int send(unsigned int i_carrier, unsigned char *ip_buf,
	         unsigned int i_len);

	int receive(int fd, unsigned int *op_carrier, unsigned char *op_buf,
	            unsigned int *op_len, unsigned int op_max_len, long timeout_ms);

	int getChannelFdByChannelId(unsigned int i_channelID);

	unsigned int getNbChannel();

 private:
	// type of the socket (udp or ethernet)
	std::string socket_type;
};

#endif

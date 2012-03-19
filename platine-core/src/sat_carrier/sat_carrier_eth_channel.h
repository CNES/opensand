/**
 * @file sat_carrier_eth_channel.h
 * @brief This implements a ethernet satellite carrier channel
 * @author AQL (Antoine)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef SAT_CARRIER_ETH_CHANNEL_H
#define SAT_CARRIER_ETH_CHANNEL_H

// system includes
#include <net/ethernet.h>
#include <netinet/in.h>

// platine includes
#include "sat_carrier_channel.h"

// Protocol field of Ethernet header to identify satellite carrier frames
#define SAT_ETH_PROTO (5678)


/*
 * @class sat_carrier_eth_channel
 * @brief Ethernet satellite carrier channel
 */
class sat_carrier_eth_channel: public sat_carrier_channel
{
 public:

	sat_carrier_eth_channel(unsigned int channelID,
	                        bool input, bool output,
	                        const char *localInterfaceName,
	                        const char *remoteMacAddress);

	~sat_carrier_eth_channel();

	int getChannelFd();

	unsigned char *getRemoteMacAddress();

	unsigned char *getLocalMacAddress();

	int send(unsigned char *buf, unsigned int len);

	int receive(unsigned char *buf, unsigned int *data_len,
	            unsigned int max_len, long timeout);

	static int getMacAddress(const char *interfaceName,
	                         unsigned char *macAddress);

 protected:

	/// the local MAC address of the channel
	unsigned char m_localMacAddress[ETH_ALEN];

	/// the remote MAC address of the channel
	unsigned char m_remoteMacAddress[ETH_ALEN];

	/// address of the channel
	sockaddr_ll m_socketAddr;

	/// the common network socket shared by all the Ethernet channels
	static int common_socket;
	/// reference counter for the common network socket
	static unsigned int socket_use_counter;

	/// internal buffer to build and send ethernet frames
	unsigned char send_buffer[2000];

	/// buffer (common to all the ethernet channels)
	/// to receive ethernet frames
	static unsigned char recv_buffer[2000];

	/// length of data stored in the common receive buffer
	static int recv_buffer_len;

	/// file descriptor associated with the data stored
	/// in the common receive buffer
	static int fd_for_recv_buffer;

	/// channel ID of the satellite channel that put the data
	/// in the receive buffer
	static int recv_buffer_channel_id;
};

#endif

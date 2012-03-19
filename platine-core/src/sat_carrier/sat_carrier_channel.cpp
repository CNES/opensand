/**
 * @file sat_carrier_channel.cpp
 * @brief This implements a bloc sat carrier channel
 * @author AQL (Antoine)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#define DBG_PACKAGE PKG_SAT_CARRIER
#include "platine_conf/uti_debug.h"

#include "sat_carrier_channel.h"

/**
 * Constructor
 * @param channelID the ID of the new channel
 * @param input true if the channel accept input
 * @param output true if channel accept output
 */
sat_carrier_channel::sat_carrier_channel(unsigned int channelID,
                                         bool input,
                                         bool output)
{

	m_channelID = channelID;
	m_input = input;
	m_output = output;
}

/**
 * Destructor
 */
sat_carrier_channel::~sat_carrier_channel()
{
}

/**
 * Get the ID of the channel
 * @return the channel ID
 */
unsigned int sat_carrier_channel::getChannelID()
{
	return (m_channelID);
}

/**
 * Get if the channel accept input
 * @return true if channel accept input
 */
bool sat_carrier_channel::isInputOk()
{
	return (m_input);
}

/**
 * Get if the channel accept output
 * @return true if channel accept output
 */
bool sat_carrier_channel::isOutputOk()
{
	return (m_output);
}

/**
 * Get the index of a network interface
 * @param name the name of the interface
 * @return the index of the interface if successful, -1 otherwise
 */
int sat_carrier_channel::getIfIndex(const char *name)
{
	const char FUNCNAME[] = "[sat_carrier_channel::getIfIndex]";
	int sock;
	ifreq ifr;
	int index = -1;

	// open the network interface socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(sock < 0)
	{
		UTI_ERROR("%s cannot create an INET socket: %s (%d)\n", FUNCNAME,
		strerror(errno), errno);
		goto exit;
	}

	// get the network interface index
	bzero(&ifr, sizeof(ifreq));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
	if(ioctl(sock, SIOGIFINDEX, &ifr) < 0)
	{
		UTI_ERROR("%s cannot get the network interface index: %s (%d)\n",
			   FUNCNAME, strerror(errno), errno);
		goto close;
	}

	index = ifr.ifr_ifindex;

close:
	close(sock);
exit:
	return index;
}


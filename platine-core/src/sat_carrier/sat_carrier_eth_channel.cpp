/**
 * @file sat_carrier_eth_channel.cpp
 * @brief This implements a ethernet satellite carrier channel
 * @author AQL (Antoine)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#define DBG_PACKAGE PKG_SAT_CARRIER
#include "platine_conf/uti_debug.h"

#include "sat_carrier_eth_channel.h"

int sat_carrier_eth_channel::common_socket = -1;
unsigned int sat_carrier_eth_channel::socket_use_counter = 0;

unsigned char sat_carrier_eth_channel::recv_buffer[2000];
int sat_carrier_eth_channel::recv_buffer_len = 0;
int sat_carrier_eth_channel::fd_for_recv_buffer = -1;
int sat_carrier_eth_channel::recv_buffer_channel_id = -1;

/**
 * Constructor
 * @param channelID the Id of the new channel
 * @param input true if the channel accept incoming data
 * @param output true if channel send data
 * @param localInterfaceName the name of the local network interface to use
 * @param remoteMacAddress the MAC address of the remote network interface
 * @see sat_carrier_channel::sat_carrier_channel()
 */
sat_carrier_eth_channel::sat_carrier_eth_channel(unsigned int channelID,
                                                 bool input, bool output,
                                                 const char *localInterfaceName,
                                                 const char *remoteMacAddress):
	sat_carrier_channel(channelID, input, output)
{
	int option_sock;
	socklen_t l_len;
	int ifIndex;

	// if it's the first channel initialize the socket
	if(this->common_socket < 0)
	{
		// open the socket
		this->common_socket = socket(AF_PACKET, SOCK_RAW, htons(SAT_ETH_PROTO));
		if(this->common_socket < 0)
		{
			UTI_ERROR("Can't open the receive socket, errno %d (%s)\n",
			          errno, strerror(errno));
			goto error;
		}

		option_sock = 0;
		l_len = sizeof(option_sock);

		// Checking socket buffer len
		getsockopt(this->common_socket, SOL_SOCKET, SO_SNDBUF,
					  &option_sock, &l_len);
		UTI_DEBUG("Socket buffer length = %d\n", option_sock);
	}

	// increment the use counter for the common socket
	this->socket_use_counter++;

	sscanf(remoteMacAddress, "%x:%x:%x:%x:%x:%x",
	       (unsigned int *) &(m_remoteMacAddress[0]),
	       (unsigned int *) &(m_remoteMacAddress[1]),
	       (unsigned int *) &(m_remoteMacAddress[2]),
	       (unsigned int *) &(m_remoteMacAddress[3]),
	       (unsigned int *) &(m_remoteMacAddress[4]),
	       (unsigned int *) &(m_remoteMacAddress[5]));

	// get the local MAC address of the interface
	if(sat_carrier_eth_channel::getMacAddress(localInterfaceName,
	                                          m_localMacAddress) < 0)
	{
		UTI_ERROR("Can't get Mac Address for %s\n", localInterfaceName);
		goto error;
	}

	// get the index of the network interface
	ifIndex = sat_carrier_eth_channel::getIfIndex(localInterfaceName);

	if(ifIndex < 0)
	{
		UTI_ERROR("cannot get the index for %s\n", localInterfaceName);
		goto error;
	}

	// initialiaze the channel
	bzero(&this->m_socketAddr, sizeof(this->m_socketAddr));
	m_socketAddr.sll_family = PF_PACKET;
	m_socketAddr.sll_protocol = htons(SAT_ETH_PROTO);
	m_socketAddr.sll_pkttype = PACKET_MULTICAST;
	m_socketAddr.sll_hatype = 0;
	m_socketAddr.sll_ifindex = ifIndex;
	m_socketAddr.sll_halen = ETH_ALEN;

	memcpy(m_socketAddr.sll_addr, this->getRemoteMacAddress(), ETH_ALEN);

	return;

 error:
	UTI_ERROR("Can't create channel\n");
}

/**
 * Destructor
 */
sat_carrier_eth_channel::~sat_carrier_eth_channel()
{
	this->socket_use_counter--;
	if(this->socket_use_counter == 0)
		close(this->common_socket);
}

/**
 * Return the network socket common to all the ethernet channels
 * @return the network socket common to all the ethernet channels
 */
int sat_carrier_eth_channel::getChannelFd()
{
	return this->common_socket;
}

/**
 * Return the remote Mac Address of the channel
 * @return the remote Mac Address
 */
unsigned char *sat_carrier_eth_channel::getRemoteMacAddress()
{
	return (m_remoteMacAddress);
}

/**
 * Return the local Mac Address of the channel
 * @return the local Mac Address
 */
unsigned char *sat_carrier_eth_channel::getLocalMacAddress()
{
	return (m_localMacAddress);
}

/**
 * Blocking receive function.
 * @param buf      pointer to a char buffer
 * @param data_len length of the received data
 * @param max_len  length of the buffer
 * @param timeout  maximum amount of time to wait for data (in ms)
 * @return         the length of data receive in case of success, -1 otherwise
 */
int sat_carrier_eth_channel::receive(unsigned char *buf, unsigned int *data_len,
                                     unsigned int max_len, long timeout)
{
	const char FUNCNAME[] = "[sat_carrier_eth_channel::receive]";
	struct ether_header *hdr_eth;

	// Broadcast address
	char mac_broadcast[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	int num;

	UTI_DEBUG_L3("%s try to receive a packet from satellite channel %d\n",
	             FUNCNAME, this->getChannelID());

	// the channel file descriptor must be valid
	if(this->getChannelFd() < 0)
	{
		UTI_ERROR("%s socket not open !\n", FUNCNAME);
		goto error;
	}

	// ignore if channel doesn't accept incoming data
	if(!this->isInputOk())
	{
		UTI_DEBUG_L3("%s channel %d does not accept data\n",
		             FUNCNAME, this->getChannelID());
		goto ignore;
	}

	// if the channel file descriptor matches the file descriptor associated
	// with the receive buffer and the channel is not the one that put the
	// packet in the buffer, there is no need to read an ethernet frame from
	// the socket, the packet is already stored in the received buffer;
	// otherwise, file descriptors are different and the packet in the buffer
	// must be ignored
	if(this->getChannelFd() == this->fd_for_recv_buffer &&
	   this->recv_buffer_channel_id > 0 &&
	   (unsigned int) this->recv_buffer_channel_id != this->getChannelID())
	{
		UTI_DEBUG_L3("%s there is data waiting in the receive buffer "
		             "(length = %d)\n", FUNCNAME, this->recv_buffer_len);
		hdr_eth = (struct ether_header *) this->recv_buffer;
		goto skip_read;
	}
	else
	{
		UTI_DEBUG_L3("%s there is no data waiting in the receive buffer\n",
		             FUNCNAME);
		this->fd_for_recv_buffer = -1;
		this->recv_buffer_channel_id = -1;
	}

	// wait for data to read on the selected socket
	num = mgl_socket_select_fd(timeout, this->common_socket);
	if(num < 0)
	{
		UTI_ERROR("%s cannot wait for data to receive\n", FUNCNAME);
		goto error;
	}
	else if(num == 0)
	{
		UTI_ERROR("%s no data to receive before timeout\n", FUNCNAME);
		goto error;
	}

	// retrieve the ethernet frame
	this->recv_buffer_len = recv(this->common_socket, this->recv_buffer,
	                             sizeof(this->recv_buffer), 0);
	if(this->recv_buffer_len == -1)
	{
		UTI_ERROR("%s reception of ethernet frame failed\n", FUNCNAME);
		goto error;
	}
	else if(this->recv_buffer_len <= ETH_HLEN)
	{
		UTI_ERROR("%s received data (%d bytes) too small for an ethernet "
		          "frame\n", FUNCNAME, this->recv_buffer_len);
		goto error;
	}

	// get the ethernet header
	hdr_eth = (struct ether_header *) this->recv_buffer;

	// filter bad ethernet protocol
	if(ntohs(hdr_eth->ether_type) != SAT_ETH_PROTO)
	{
		UTI_DEBUG("%s bad protocol received and ignored\n", FUNCNAME);
		goto ignore;
	}

	// filter broadcast
	if(memcmp(hdr_eth->ether_dhost, mac_broadcast, ETH_ALEN) == 0)
	{
		UTI_DEBUG("%s broadcast received and ignored\n", FUNCNAME);
		goto ignore;
	}

skip_read:

	// does the received MAC address matches with the channel one?
	//  - if no, the data is NOT for the current channel, but for another one
	//    in the channel set => associate the data in the buffer with the
	//    channel file descriptor and return that no data has been received for
	//    that channel, the 'channel set' object will try the other channels
	//    that share the same file descriptor
	//  - if yes, the data is for the current channel => return the received
	//    packet and mark the buffer as empty
	if(memcmp(hdr_eth->ether_dhost, this->getRemoteMacAddress(), ETH_ALEN) != 0)
	{
		u_int8_t *d = hdr_eth->ether_dhost;

#define mac_addr(addr) \
	*(addr), *((addr)+1), *((addr)+2), \
	*((addr)+3), *((addr)+4), *((addr)+5)

		UTI_DEBUG_L3("%s eth frame dest addr (%x:%x:%x:%x:%x:%x) does not match "
		             "channel addr (%x:%x:%x:%x:%x:%x), store the eth frame in "
		             "buffer\n", FUNCNAME, mac_addr(d),
		             mac_addr(this->getRemoteMacAddress()));

		if(this->fd_for_recv_buffer == -1)
		{
			this->fd_for_recv_buffer = this->getChannelFd();
			this->recv_buffer_channel_id = this->getChannelID();
		}

		goto ignore;
	}

	UTI_DEBUG("%s channel %d accepts data\n", FUNCNAME,
	          this->getChannelID());

	*data_len = this->recv_buffer_len - ETH_HLEN;

	if(*data_len > max_len)
	{
		UTI_ERROR("%s received packet (%d bytes) too large for buffer "
		          "(%d bytes)\n", FUNCNAME, *data_len, max_len);
		goto error;
	}

	memcpy(buf, this->recv_buffer + ETH_HLEN, *data_len);

	this->recv_buffer_len = 0;
	this->fd_for_recv_buffer = -1;
	this->recv_buffer_channel_id = -1;

ignore:
	return 0;
error:
	return -1;
}

/**
* Send a variable length buffer on the specified satellite carrier.
* @param buf pointer to a char buffer
* @param len length of the buffer
* @return -1 if failed, the size of data send if succeed
*/
int sat_carrier_eth_channel::send(unsigned char *buf, unsigned int len)
{
	int ret;
	int lg;
	struct ether_header *hdr_eth;

	if(!m_output)
	{
		UTI_ERROR("Channel %d is not configure to send data\n", m_channelID);
		return (-1);
	}

	if(this->common_socket < 0)
	{
		UTI_ERROR("Socket not open !\n");
		goto error;
	}

	hdr_eth = (struct ether_header *) this->send_buffer;

	memcpy(hdr_eth->ether_shost, this->getLocalMacAddress(), ETH_ALEN);
	memcpy(hdr_eth->ether_dhost, this->getRemoteMacAddress(), ETH_ALEN);
	hdr_eth->ether_type = htons(SAT_ETH_PROTO);
	lg = ETH_HLEN + len;

	memcpy(send_buffer + ETH_HLEN, buf, len);

	errno = 0;
	ret = sendto(this->common_socket, send_buffer, lg, 0,
	             (struct sockaddr *) &m_socketAddr, sizeof(m_socketAddr));
	if(ret < lg)
	{
		UTI_ERROR("Error:  sendto(..,0,..) errno %s (%d)\n",
		          strerror(errno), errno);
		goto error;
	}

	UTI_DEBUG("==> SAT_Channel_Send [%d]: len=%d\n", m_channelID, len);

	return len;

 error:
	return (-1);
}

/**
 * Get the MAC address of a network interface
 * @param interfaceName the if network interface name (eth0)
 * @param macAddress char[6] where the MAC address will be put in
 * @return -1 if failed, 0 if succeed
 */
int sat_carrier_eth_channel::getMacAddress(const char *interfaceName,
                                           unsigned char *macAddress)
{
	int ifSocket = -1;
	struct ifreq ifr;

	// Open if socket
	ifSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(ifSocket < 0)
	{
		UTI_ERROR("Can't get information about network interface, "
		          "errno %d (%s)", errno, strerror(errno));
		goto error;
	}

	bzero(&ifr, sizeof(ifreq));
	strncpy(ifr.ifr_name, interfaceName, sizeof(ifr.ifr_name) - 1);

	if(ioctl(ifSocket, SIOCGIFHWADDR, &ifr) < 0)
	{
		UTI_ERROR("IOCTL SIOCGIFHWADDR, errno %d (%s)", errno, strerror(errno));
		goto error;
	}

	memcpy(macAddress, ifr.ifr_hwaddr.sa_data, 6);

	close(ifSocket);

	return (0);

error:
	return (-1);
}



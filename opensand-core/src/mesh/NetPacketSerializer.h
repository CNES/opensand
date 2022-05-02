#include "NetPacket.h"
#include <array>
#include <cstdint>
#include <memory>

#define NET_PACKET_MAX_NAME_SIZE 32
#define NET_PACKET_MAX_DATA_SIZE 8000

struct __attribute__((__packed__)) net_packet_buffer_t
{
	spot_id_t spot;
	uint8_t qos;
	uint8_t src_tal_id;
	uint8_t dst_tal_id;
	uint16_t type;
	std::array<char, NET_PACKET_MAX_NAME_SIZE> name;
	uint32_t name_length;
	std::size_t header_length;
	uint32_t length;
	std::array<uint8_t, NET_PACKET_MAX_DATA_SIZE> data;

	net_packet_buffer_t(const NetPacket &pkt);
	std::unique_ptr<NetPacket> deserialize() const;
};

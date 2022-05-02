#include "NetPacketSerializer.h"
#include "NetPacket.h"

net_packet_buffer_t::net_packet_buffer_t(const NetPacket &pkt):
    spot{pkt.getSpot()},
    qos{pkt.getQos()},
    src_tal_id{pkt.getSrcTalId()},
    dst_tal_id{pkt.getDstTalId()},
    type{pkt.getType()},
    header_length{pkt.getHeaderLength()}
{
	auto pkt_name = pkt.getName();
	name_length = pkt_name.size();
	std::copy(pkt_name.begin(), pkt_name.end(), name.begin()); // Try std::move?
	auto pkt_data = pkt.getData();
	length = pkt_data.size();
	std::copy(pkt_data.begin(), pkt_data.end(), data.begin()); // Try std::move?
}

std::unique_ptr<NetPacket> net_packet_buffer_t::deserialize() const
{
	std::string name_str{name.data(), name_length};
	Data data_str{data.data(), length};
	auto pkt = std::unique_ptr<NetPacket>{
	    new NetPacket{data_str,
	                  length,
	                  name_str,
	                  type,
	                  qos,
	                  src_tal_id,
	                  dst_tal_id,
	                  header_length}};
    pkt->setSpot(spot);
	return pkt;
}

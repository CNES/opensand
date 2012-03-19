/**
 * @file AtmCtx.cpp
 * @brief ATM encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "AtmCtx.h"

#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


AtmCtx::AtmCtx(): EncapCtx(), contexts()
{
}

AtmCtx::~AtmCtx()
{
	std::map < AtmIdentifier *, Data *, ltAtmIdentifier >::iterator it;

	for(it = this->contexts.begin(); it != this->contexts.end(); it++)
	{
		if((*it).first != NULL)
			delete (*it).first;
		if((*it).second != NULL)
			delete (*it).second;
	}
}

NetBurst *AtmCtx::encapsulate(NetPacket *packet,
                              int &context_id,
                              long &time)
{
	const char *FUNCNAME = "[AtmCtx::encapsulate]";
	Aal5Packet *aal5_packet;
	NetBurst *atm_cells;
	AtmCell *atm_cell;
	int nb_atm_cells, i;
	uint8_t vpi;
	uint16_t vci;

	time = 0; // no need for an encapsulation timer

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	// packet must be an AAL5 packet
	if(packet->type() != NET_PROTO_AAL5)
	{
		UTI_ERROR("%s encapsulation packet is not an AAL5 packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	packet->addTrace(HERE());
	UTI_DEBUG_L3("%s talID of packet to encapsulate: %ld \n", FUNCNAME, packet->talId());

	// cast from a generic packet to an AAL5 packet
	aal5_packet = dynamic_cast<Aal5Packet *>(packet);
	if(aal5_packet == NULL)
	{
		UTI_ERROR("%s bad cast from NetPacket to Aal5Packet!\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s AAL5 packet is valid, create ATM cells", FUNCNAME);

	// create an empty burst of ATM cells
	atm_cells = new NetBurst();
	if(atm_cells == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of ATM cells\n",
		          FUNCNAME);
		goto drop;
	}

	nb_atm_cells = aal5_packet->nbAtmCells();
	for(i = 0; i < nb_atm_cells; i++)
	{
		// VPI (8 bits) = MAC id = satellite spot id
		// VCI (16 bits) = TAL id (13 bits) + QoS (3 bits)
		vpi = packet->macId();
		vci = ((packet->talId() & 0x1fff) << 3) + (packet->qos() & 0x07);

		if((packet->macId() & 0x00ff) != packet->macId())
		{
			UTI_ERROR("Be careful, you have set a MAC ID (satellite spot id) greater"
			           " than 0x00ff, this can not stand in 8 bits of VPI field of"
			           " ATM cells!!!\n");
		}

		if((packet->talId() & 0x1fff) != packet->talId())
		{
			UTI_ERROR("Be careful, you have set a TAL ID greater than 0x1fff,"
			           " this can not stand in the first 13 bits of VCI field of"
			           " ATM cells!!!\n");
		}
		if((packet->qos() & 0x0007) != packet->qos())
		{
			UTI_ERROR("Be careful, you have set a QoS priority greater than 7,"
			           " this can not stand in the last 3 bits of VCI field of"
			           "  ATM cells!!!\n");
		}

		atm_cell = AtmCell::create(i, vpi, vci, 0x40, 0,
		                          (i == (nb_atm_cells - 1)),
		                          aal5_packet->atmCell(i));
		if(atm_cell == NULL)
		{
			UTI_ERROR("%s cannot allocate memory for one ATM cell, drop it\n",
			          FUNCNAME);
			continue;
		}
		atm_cell->addTrace(HERE());

		UTI_DEBUG("%s one ATM cell created with QoS %d\n",
		          FUNCNAME, atm_cell->qos());

		// add the ATM cell to the list
		atm_cells->push_back(atm_cell);
	}

	return atm_cells;

drop:
	return NULL;
}

NetBurst *AtmCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[AtmCtx::desencapsulate]";
	NetBurst *aal5_packets;
	AtmCell *atm_cell;
	uint8_t vpi;
	uint16_t vci;
	AtmIdentifier *atm_id;
	std::map < AtmIdentifier *, Data *,
	           ltAtmIdentifier >::iterator context_it;
	Data *context;

	// packet must be a valid encapsulation packet
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	packet->addTrace(HERE());
	// packet must be an ATM cell
	if(packet->type() != NET_PROTO_ATM)
	{
		UTI_ERROR("%s encapsulation packet is not an ATM cell, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG_L3("%s talID of received packet: %ld \n",
	             FUNCNAME, packet->talId());

	// cast from a generic packet to an ATM cell
	atm_cell = dynamic_cast<AtmCell *>(packet);
	if(atm_cell == NULL)
	{
		UTI_ERROR("%s bad cast from NetPacket to AtmCell!\n", FUNCNAME);
		goto drop;
	}

	// get the VPI and VCI numbers for the ATM cell to desencapsulate
	vpi = atm_cell->vpi();
	vci = atm_cell->vci();
	UTI_DEBUG("%s ATM packet belongs to the encapsulation context "
	          "identified by VPI = %d and VCI = %d\n", FUNCNAME, vpi, vci);

	// find the desencapsulation context for the ATM cell
	atm_id = new AtmIdentifier(vpi, vci);
	context_it = this->contexts.find(atm_id);

	if(context_it == this->contexts.end())
	{
		UTI_DEBUG("%s desencapsulation context does not exist yet\n", FUNCNAME);

		Data *new_context = new Data();
		std::pair < std::map < AtmIdentifier *, Data *,
		            ltAtmIdentifier >::iterator, bool > infos;
		infos = this->contexts.insert(make_pair(atm_id, new_context));

		if(!infos.second)
		{
			UTI_ERROR("%s cannot create a new desencapsulation context, "
			          "drop the packet\n", FUNCNAME);
			delete new_context;
			delete atm_id;
			goto drop;
		}

		UTI_INFO("%s new desencapsulation context created (VPI = %d, "
		         "VCI = %d)\n", FUNCNAME, vpi, vci);
		context = (*(infos.first)).second;
	}
	else
	{
		UTI_DEBUG("%s desencapsulation context already exists\n", FUNCNAME);
		context = (*context_it).second;
		delete atm_id;
	}

	UTI_DEBUG("%s desencapsulation context contains %d bytes of data\n",
	          FUNCNAME, context->length());

	// create an empty burst of AAL5 packets
	aal5_packets = new NetBurst();
	if(aal5_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of AAL5 packets\n",
		          FUNCNAME);
		goto drop;
	}

	// desencapsulate the ATM payload data

	if(!atm_cell->isLastCell())
	{
		UTI_DEBUG("%s ATM cell is not the last one of AAL5 packet, store "
		          "payload data in the desencapsulation context for "
		          "next ATM cell\n", FUNCNAME);

		context->append(atm_cell->payload());
	}
	else
	{
		NetPacket *aal5_packet;

		UTI_DEBUG("%s ATM cell is the last one of AAL5 packet, "
		          "extract the AAL5 packet from ATM payloads\n", FUNCNAME);

		context->append(atm_cell->payload());

		// create an AAL5 packet with ATM payloads
		aal5_packet = new Aal5Packet(*context);
		if(aal5_packet == NULL)
		{
			UTI_ERROR("%s cannot create an AAL5 packet, drop all of the "
			          "ATM cells in the desencapsulation context\n", FUNCNAME);
			goto clear;
		}
		aal5_packet->addTrace(HERE());
		// check AAL5 packet validity
		if(!aal5_packet->isValid())
		{
			UTI_ERROR("%s AAL5 packet is not valid, drop all of the "
			          "ATM cells in the desencapsulation context\n", FUNCNAME);
			delete aal5_packet;
			goto clear;
		}

		// set some parameters
		aal5_packet->setMacId(atm_cell->macId());
		aal5_packet->setTalId(atm_cell->talId());
		UTI_DEBUG_L3("%s talID of AAL5 packet: %ld \n",
		             FUNCNAME, aal5_packet->talId());
		aal5_packet->setQos(atm_cell->qos());

		// add the AAL5 packet to the list
		aal5_packets->push_back(aal5_packet);
		UTI_DEBUG("%s AAL5 packet added to the burst\n", FUNCNAME);

		// clear data stored in context
		context->clear();
	}

	UTI_DEBUG("%s ATM cell is now desencapsulated "
	          "(context data = %d bytes)\n",
	          FUNCNAME, context->length());

	return aal5_packets;

clear:
	// clear data stored in context
	context->clear();
	// delete the burst of AAL5 packets
	delete aal5_packets;
drop:
	return NULL;
}

std::string AtmCtx::type()
{
	return std::string("ATM");
}

NetBurst *AtmCtx::flush(int context_it)
{
	// nothing to do for ATM
	UTI_DEBUG("[AtmCtx::flush] do nothing\n");
	return NULL;
}

NetBurst *AtmCtx::flushAll()
{
	// nothing to do for ATM
	UTI_DEBUG("[AtmCtx::flushAll] do nothing\n");
	return NULL;
}


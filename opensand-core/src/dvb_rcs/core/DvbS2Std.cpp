/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file DvbS2Std.cpp
 * @brief DVB-S2 Transmission Standard
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include "DvbS2Std.h"

#include <cassert>
#include <algorithm>

using std::list;


DvbS2Std::DvbS2Std(const EncapPlugin::EncapPacketHandler *const pkt_hdl):
	PhysicStd("DVB-S2", pkt_hdl),
	real_modcod(28), // TODO fmt_simu->getmaxFwdModcod()
	received_modcod(this->real_modcod)
{
	/* TODO read this value from file
	 * for example get the higher MODCOD value on init
	 * thus every MODCOD will be decoded */
	// TODO handle elsewhere ?

}

DvbS2Std::~DvbS2Std()
{
}


#if 0
bool DvbS2Std::createOptionModcod(component_t comp, long tal_id,
                                  unsigned int modcod, long spot_id)
{
	int ul_encap_packet_length = MpegPacket::length();
	unsigned char *dvb_payload;  // pointer to room for encap packet
				     // in the BBframe payload
	int max_packets;
	int bbframe_size;
	map<unsigned int, T_DVB_BBFRAME *> *bbframe_table;
	map<unsigned int, T_DVB_BBFRAME *>::iterator it;
	NetBurst *burst_table;
	bool do_advertise_modcod;
	int max_per_burst;
	int first_id;

	// retrieve some variables
	switch(comp)
	{
		case satellite:
			bbframe_table = ((BlocDVBRcsSat*)component_bloc)->spots[id]->m_bbframe;
			burst_table = ((BlocDVBRcsSat*)component_bloc)->spots[id]->m_encapBurstMPEGUnderBuild;
			do_advertise_modcod = !((BlocDVBRcsSat*)component_bloc)->fmt_simu.isCurrentModcodAdvertised(tal_id);
			first_id = ((BlocDVBRcsSat*)component_bloc)->getFirstModcodID();
			break;
		case gateway:
			bbframe_table = ((BlocDVBRcsNcc*) component_bloc)->m_bbframe;
			burst_table = ((BlocDVBRcsNcc*)component_bloc)->m_encapBurstMPEGUnderBuild;
			do_advertise_modcod = !((BlocDVBRcsNcc*)component_bloc)->fmt_simu.isCurrentModcodAdvertised(tal_id);
			first_id = ((BlocDVBRcsNcc*)component_bloc)->getFirstModcodID();
			break;
		default:
			UTI_ERROR("the type of the component is not adapted here\n");
			assert(0);
			goto error;
	}

	// do not create MODCOD option if the MODCOD
	if(!do_advertise_modcod)
	{
		UTI_DEBUG("MODCOD ID already advertised for ST %ld\n", tal_id);
		goto skip;
	}

	// get the BB frame that corresponds to the MODCOD
	it = bbframe_table->find(modcod);
	if(update == true && it != bbframe_table->end())
	{
		UTI_DEBUG_L3("option in construction\n");
		dvb_payload = (unsigned char *)it->second + sizeof(T_DVB_BBFRAME)+
			      it->second->real_modcod_nbr*sizeof(T_DVB_REAL_MODCOD);
		it->second->real_modcod_nbr ++;
		g_memory_pool_dvb_rcs.add_function(string(__FUNCTION__), (char *) it->second);

		//Updates the size of the data
		max_per_burst = (MSG_BBFRAME_SIZE_MAX- sizeof(T_DVB_BBFRAME)
			      - (it->second->real_modcod_nbr * sizeof(T_DVB_REAL_MODCOD)))
			      / ul_encap_packet_length;
		if(comp == satellite)
		{
			((BlocDVBRcsSat*)component_bloc)->setMaxPacketsBurst(max_per_burst);
			bbframe_size = DvbS2Std::getPayload(((BlocDVBRcsSat*)component_bloc)->modcod_definitions.getCodingRate(modcod));
		}
		else if(comp == gateway)
		{
			((BlocDVBRcsNcc*)component_bloc)->setMaxPacketsBurst(max_per_burst);
			bbframe_size = DvbS2Std::getPayload(((BlocDVBRcsNcc*)component_bloc)->modcod_definitions.getCodingRate(modcod));
		}
		else
		{
			assert(0);
		}
		max_packets = (bbframe_size - sizeof(T_DVB_BBFRAME)-(it->second->real_modcod_nbr
			      *sizeof(T_DVB_REAL_MODCOD)))/ ul_encap_packet_length;
		UTI_DEBUG("%s max packets : %d \n", FUNCNAME, max_packets);
		burst_table[modcod - first_id].setMaxPackets(max_packets);

		//Option to give the new real modcod to the specified ST (pid)
		T_DVB_REAL_MODCOD *newRealModcod;
		newRealModcod =(T_DVB_REAL_MODCOD*)malloc(sizeof(T_DVB_REAL_MODCOD));
		if(newRealModcod != NULL)
		{
			newRealModcod->terminal_id = tal_id;
			if(comp == satellite)
			{
				((BlocDVBRcsSat*)component_bloc)->setUpdateST(false, nb_row);
				newRealModcod->real_modcod = ((BlocDVBRcsSat*)component_bloc)->getModcodID(nb_row);
			}
			else if(comp == gateway)
			{
				((BlocDVBRcsNcc*)component_bloc)->setUpdateST(false, nb_row);
				newRealModcod->real_modcod = ((BlocDVBRcsNcc*)component_bloc)->getModcodID(tal_id);
			}
			// fills the BBframe payload with the new option
			memcpy(dvb_payload, newRealModcod, sizeof(T_DVB_REAL_MODCOD));
			free(newRealModcod);
		}
		else
		{
			UTI_ERROR("%s Failed to allocate the new option\n", FUNCNAME);
		}
	}

skip:
	UTI_DEBUG_L3("creation of MODCOD option for ST %ld finished\n", tal_id);
	return true;

error:
	UTI_ERROR("failed to create MODCOD option for ST %ld\n", tal_id);
	return false;
}
#endif


// TODO factorize with DVB-RCS function ?
int DvbS2Std::onRcvFrame(unsigned char *frame,
                         long length,
                         long type,
                         int mac_id,
                         NetBurst **burst)
{
	T_DVB_BBFRAME *bbframe_burst; // BBFrame burst received from lower layer
	long i;                       // counter for packets
	int real_mod;                 // real modcod of the receiver

	// Offset from beginning of frame to beginning of data
	size_t offset;
	size_t previous_length = 0;

	*burst = NULL;

	// sanity check
	if(frame == NULL)
	{
		UTI_ERROR("invalid frame received\n");
		goto error;
	}

	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}

	bbframe_burst = (T_DVB_BBFRAME *) frame;

	// sanity check: this function only handle BB frames
	if(type != MSG_TYPE_BBFRAME)
	{
		UTI_ERROR("the message received is not a BB frame\n");
		goto error;
	}
	if(bbframe_burst->pkt_type != this->packet_handler->getEtherType())
	{
		UTI_ERROR("Bad packet type (%d) in BB frame burst (expecting %d)\n",
		          bbframe_burst->pkt_type,
		          this->packet_handler->getEtherType());
		goto error;
	}
	UTI_DEBUG("BB frame received (%d %s packet(s)\n",
	           bbframe_burst->data_length,
	           this->packet_handler->getName().c_str());

	// retrieve the current real MODCOD of the receiver
	// (do this before any MODCOD update occurs)
	real_mod = this->real_modcod;

	// check if there is an update of the real MODCOD among all the real
	// MODCOD options located just after the header of the BB frame
	for(i = 0; i < bbframe_burst->real_modcod_nbr; i++)
	{
		T_DVB_REAL_MODCOD *real_modcod_option;

		// retrieve one real MODCOD option
		real_modcod_option = (T_DVB_REAL_MODCOD *)
			(frame + sizeof(T_DVB_BBFRAME) + i * sizeof(T_DVB_REAL_MODCOD));

		// is the option for us ?
		if(real_modcod_option->terminal_id == mac_id)
		{
			UTI_DEBUG("update real MODCOD to %d\n",
			          real_modcod_option->real_modcod);
			// check if the value is not outside the values of the file
			this->real_modcod = real_modcod_option->real_modcod;
		}
	}

	// used for terminal statistics
	// TODO add the stat
	this->received_modcod = bbframe_burst->used_modcod;

	// is the ST able to decode the received BB frame ?
	if(this->received_modcod > real_mod)
	{
		// the BB frame is not robust enough to be decoded, drop it
		UTI_ERROR("received BB frame is encoded with MODCOD %d and "
		          "the real MODCOD of the BB frame (%d) is not "
		          "robust enough, so emulate a lost BB frame\n",
		          this->received_modcod, real_mod);
		goto drop;
	}

	if(bbframe_burst->data_length <= 0)
	{
		UTI_DEBUG("skip BB frame with no encapsulation packet\n");
		goto skip;
	}

	// now we are sure that the BB frame is robust enough to be decoded,
	// so create an empty burst of encapsulation packets to store the
	// encapsulation packets we are about to extract from the BB frame
	*burst = new NetBurst();
	if(*burst == NULL)
	{
		UTI_ERROR("failed to create a burst of packets\n");
		goto error;
	}

	// add packets received from lower layer
	// to the newly created burst
	offset = sizeof(T_DVB_BBFRAME) +
	         bbframe_burst->real_modcod_nbr * sizeof(T_DVB_REAL_MODCOD);
	for(i = 0; i < bbframe_burst->data_length; i++)
	{
		NetPacket *encap_packet;
		size_t current_length;

		current_length = this->packet_handler->getLength(frame + offset +
		                                                 previous_length);
		// Use default values for QoS, source/destination tal_id
		encap_packet = this->packet_handler->build(frame + offset + previous_length,
		                                           current_length,
		                                           0x00, BROADCAST_TAL_ID,
		                                           BROADCAST_TAL_ID);
		previous_length += current_length;
		if(encap_packet == NULL)
		{
			UTI_ERROR("cannot create one %s packet\n",
			          this->packet_handler->getName().c_str());
			goto release_burst;
		}

		// add the packet to the burst of packets
		(*burst)->add(encap_packet);
		UTI_DEBUG("%s packet (%d bytes) added to burst\n",
		          this->packet_handler->getName().c_str(),
		          encap_packet->getTotalLength());
	}

drop:
skip:
	// release buffer (data is now saved in NetPacket objects)
	free(frame);
	return 0;

release_burst:
	delete burst;
error:
	free(frame);
	return -1;
}



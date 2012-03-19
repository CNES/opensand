/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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

#include "DvbS2Std.h"

#include <cassert>
#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <platine_conf/uti_debug.h>
#include <algorithm>


DvbS2Std::DvbS2Std(EncapPlugin::EncapPacketHandler *pkt_hdl):
	PhysicStd("DVB-S2", pkt_hdl),
	modcod_definitions()
{
	/* TODO read this value from file 
	 * for example get the higher MODCOD value on init
	 * thus every MODCOD will be decoded */
	this->realModcod = 11;
	this->receivedModcod = this->realModcod;
	this->dra_scheme_definitions = new DraSchemeDefinitionTable;
}

DvbS2Std::~DvbS2Std()
{
	delete dra_scheme_definitions;
	for(std::list<BBFrame *>::iterator it = incomplete_bb_frames_ordered.begin();
	    it != incomplete_bb_frames_ordered.end(); ++it)
	{
		delete *it;
	}
}


// beware: this fonction is static
// TODO: rewrite this function to avoid coding rate as string and check the returned sizes
unsigned int DvbS2Std::getPayload(std::string coding_rate)
{
	int payload;

	if(!coding_rate.compare("1/3"))
		payload = 2676;
	else if(!coding_rate.compare("1/2"))
		payload = 4026;
	else if(!coding_rate.compare("2/3"))
		payload = 5380;
	else if(!coding_rate.compare("3/4"))
		payload = 6051;
	else if(!coding_rate.compare("5/6"))
		payload = 6730;
	else if(!coding_rate.compare("8/9"))
		payload = 7274;
	else
		payload = 8100;

	return payload;
}


#if 0 /* TODO: manage options and do not use component_bloc,
               some functions may have changed... */
bool DvbS2Std::createOptionModcod(t_component comp, long tal_id,
                                  unsigned int modcod, long spot_id)
{
	int ul_encap_packet_length = MpegPacket::length();
	unsigned char *dvb_payload;  // pointer to room for encap packet
				     // in the BBframe payload
	int max_packets;
	int bbframe_size;
	std::map<unsigned int, T_DVB_BBFRAME *> *bbframe_table;
	std::map<unsigned int, T_DVB_BBFRAME *>::iterator it;
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
			do_advertise_modcod = !((BlocDVBRcsSat*)component_bloc)->satellite_terminals.isCurrentModcodAdvertised(tal_id);
			first_id = ((BlocDVBRcsSat*)component_bloc)->getFirstModcodID();
			break;
		case gateway:
			bbframe_table = ((BlocDVBRcsNcc*) component_bloc)->m_bbframe;
			burst_table = ((BlocDVBRcsNcc*)component_bloc)->m_encapBurstMPEGUnderBuild;
			do_advertise_modcod = !((BlocDVBRcsNcc*)component_bloc)->satellite_terminals.isCurrentModcodAdvertised(tal_id);
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
			      it->second->list_realModcod_size*sizeof(T_DVB_REAL_MODCOD);
		it->second->list_realModcod_size ++;
		g_memory_pool_dvb_rcs.add_function(std::string(__FUNCTION__), (char *) it->second);

		//Updates the size of the data
		max_per_burst = (MSG_BBFRAME_SIZE_MAX- sizeof(T_DVB_BBFRAME)
			      - (it->second->list_realModcod_size * sizeof(T_DVB_REAL_MODCOD)))
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
		max_packets = (bbframe_size - sizeof(T_DVB_BBFRAME)-(it->second->list_realModcod_size
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

int DvbS2Std::scheduleEncapPackets(dvb_fifo *fifo,
                                   long current_time,
                                   std::list<DvbFrame *> *complete_dvb_frames)
{
	int ret;
	unsigned int sent_packets = 0;
	MacFifoElement *elem;
	long max_to_send;
	float duration_credit;

	// the current BBFrame
	BBFrame *current_bbframe;

	// retrieve the number of packets waiting for retransmission
	max_to_send = fifo->getCount();
	if(max_to_send <= 0)
	{
		// if there is nothing to send, return with success
		// erase remaining credit as FIFO is empty (TODO is it ok ??)
		this->remainingCredit = 0;
		goto skip;
	}

	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}

	// there are really packets to send
	UTI_DEBUG("send at most %ld encapsulation packets\n", max_to_send);

	// how much time do we have to send as many BB frames as possible ?
	// TODO remove test once frameDuration will be initialized in constructor
	if(this->frameDuration != 0)
	{
		duration_credit = ((float) this->frameDuration) + this->remainingCredit;
		UTI_DEBUG("duration credit is %.2f ms (%.2f ms were remaining)\n",
		          duration_credit, this->remainingCredit);
		this->remainingCredit = 0;
	}
	else
	{
		UTI_ERROR("frame duration not initialized\n");
		goto error;
	}

	// first add the pending complete BBFrame in the complete BBframes list
	if(this->pending_bbframe)
	{
		if(this->addCompleteBBFrame(complete_dvb_frames,
		                            this->pending_bbframe,
		                            duration_credit) < 0)
		{
			UTI_ERROR("cannot add pending BBFrame in the list of complete BBFrames\n");
		}
	}
	this->pending_bbframe = NULL;
	
	// now build BB frames with packets extracted from the MAC FIFO
	while((elem = (MacFifoElement *)fifo->get()) != NULL)
	{
		NetPacket *encap_packet;
		long tal_id;
		NetPacket *data;
		NetPacket *remaining_data;

		// first examine the packet to be sent without removing it from the queue
		if(elem->getType() != 1)
		{
			UTI_ERROR("MAC FIFO element does not contain NetPacket\n");
			goto error_fifo_elem;
		}
		// simulate the satellite delay
		if(elem->getTickOut() > current_time)
		{
			UTI_DEBUG("packet is not scheduled for the moment, "
			          "break\n");
			// this is the first MAC FIFO element that is not ready yet,
			// there is no more work to do, break now
			break;
		}

		encap_packet = elem->getPacket();
		// retrieve the encapsulation packet
		if(encap_packet == NULL)
		{
			UTI_ERROR("invalid packet #%u in MAC FIFO element\n",
			          sent_packets + 1);
			goto error_fifo_elem;
		}

		// retrieve the ST ID associated to the packet
		tal_id = encap_packet->getDstTalId();

		if(!this->getIncompleteBBFrame(tal_id, &current_bbframe))
		{
			// cannot initialize incomplete BB Frame
			delete encap_packet;
			goto error_fifo_elem;
		}
		else if(!current_bbframe)
		{
			// cannot get modcod for the ST remove the fifo element
			fifo->remove();
			delete encap_packet;
			delete elem;
			continue;
		}

		UTI_DEBUG_L3("Got the BBFrame for packet #%u, "
		             "there is now %u complete BBFrames and %u incomplete\n",
		             sent_packets + 1, complete_dvb_frames->size(),
		             this->incomplete_bb_frames.size());

		// get the part of the packet to store in the BBFrame
		ret = this->packet_handler->getChunk(encap_packet,
		                                     current_bbframe->getFreeSpace(),
		                                     &data, &remaining_data);
		// use case 4 (see @ref getChunk)
		if(!ret)
		{
			UTI_ERROR("error while processing packet #%u\n",
			          sent_packets + 1);
			fifo->remove();
			delete elem;
		}
		// use cases 1 (see @ref getChunk)
		else if(data && !remaining_data)
		{
			if(!current_bbframe->addPacket(data))
			{
				UTI_ERROR("failed to add encapsulation packet #%u "
				          "in BB frame with MODCOD ID %u (packet "
				          "length %u, free space %u",
				          sent_packets + 1, current_bbframe->getModcodId(),
				          data->getTotalLength(),
				          current_bbframe->getFreeSpace());
				goto error_fifo_elem;
			}
			// delete the NetPacket once it has been copied in the BBFrame
			delete data;
			sent_packets++;
			// remove the element from the MAC FIFO and destroy it
			fifo->remove();
			delete elem;
		}
		// use case 2 (see @ref getChunk)
		else if(data && remaining_data)
		{
			if(!current_bbframe->addPacket(data))
			{
				UTI_ERROR("failed to add encapsulation packet #%u "
				          "in BB frame with MODCOD ID %u (packet "
				          "length %u, free space %u",
				          sent_packets + 1, current_bbframe->getModcodId(),
				          data->getTotalLength(),
				          current_bbframe->getFreeSpace());
				goto error_fifo_elem;
			}
			// delete the NetPacket once it has been copied in the BBFrame
			delete data;

			// replace the fifo first element with the remaining data
			elem->setPacket(remaining_data);

			UTI_DEBUG("packet fragmented, there is still %u bytes of data\n",
			          remaining_data->getTotalLength());
		}
		// use case 3 (see @ref getChunk)
		else if(!data && remaining_data)
		{
			// replace the fifo first element with the remaining data
			elem->setPacket(remaining_data);

			// keep the NetPacket in the fifo
			UTI_DEBUG("not enough free space in BBFrame (%u bytes) "
			          "for %s packet (%u bytes)\n",
			          current_bbframe->getFreeSpace(),
			          this->packet_handler->getName().c_str(),
			          encap_packet->getTotalLength());
		}
		else
		{
			UTI_ERROR("bad getChunk function implementation, "
			          "assert or skip packet #%u\n", sent_packets + 1);
			assert(0);
			fifo->remove();
			delete elem;
		}

		// the BBFrame has been completed or the next packet is too long
		// add the BBFrame in the list of complete BBFrames and decrease
		// duration credit
		if(current_bbframe->getFreeSpace() <= 0 ||
		   remaining_data != NULL)
		{
			ret = this->addCompleteBBFrame(complete_dvb_frames,
			                               current_bbframe,
			                               duration_credit);
			if(ret == -1)
			{
				goto error;
			}
			else if(ret == -2)
			{
				this->pending_bbframe = current_bbframe;
				break;
			}
			else
			{
				unsigned int modcod = current_bbframe->getModcodId();

				this->incomplete_bb_frames_ordered.remove(current_bbframe);
				this->incomplete_bb_frames.erase(modcod);
			}
		}
	}

	// try to fill the BBFrames list with the remaining incomplete BBFrames
	for(std::list<BBFrame *>::iterator it = this->incomplete_bb_frames_ordered.begin();
	    it != this->incomplete_bb_frames_ordered.end();
	    it = this->incomplete_bb_frames_ordered.erase(it))
	{
		if(duration_credit <= 0)
		{
			break;
		}

		ret = this->addCompleteBBFrame(complete_dvb_frames, *it,
		                               duration_credit);
		if(ret == -1)
		{
			goto error;
		}
		else if(ret == 0)
		{
			unsigned int modcod = (*it)->getModcodId();

			this->incomplete_bb_frames.erase(modcod);
		}
	}

	// keep the remaining credit
	this->remainingCredit = duration_credit;

	if(sent_packets != 0)
	{
		unsigned int cpt_frame = complete_dvb_frames->size();

		UTI_DEBUG("%u %s been scheduled and %u BB %s completed\n",
		          sent_packets, (sent_packets > 1) ? "packets have" : "packet has",
		          cpt_frame, (cpt_frame > 1) ? "frames were" : "frame was");
	}

skip:
	return 0;
error_fifo_elem:
	fifo->remove();
	delete elem;
error:
	return -1;
}


int DvbS2Std::loadModcodDefinitionFile(std::string filename)
{
	// load all the MODCOD definitions from file
	if(!this->modcod_definitions.load(filename))
	{
		UTI_ERROR("failed to load all the MODCOD definitions "
		          "from file '%s'\n", filename.c_str());
		return 0;
	}

	return 1;
}

int DvbS2Std::loadModcodSimulationFile(std::string filename)
{
	// associate the simulation file with the list of STs
	if(!this->satellite_terminals.setModcodSimuFile(filename))
	{
		UTI_ERROR("failed to associate MODCOD simulation file %s"
		          "with the list of STs\n", filename.c_str());
		this->modcod_definitions.clear();
		return 0;
	}

	return 1;
}


int DvbS2Std::loadDraSchemeDefinitionFile(std::string filename)
{
	// load all the DRA definitions from file
	if(!this->dra_scheme_definitions->load(filename))
	{
		UTI_ERROR("failed to load all the DRA scheme definitions "
		          "from file '%s'\n", filename.c_str());
		return 0;
	}

	return 1;
}

int DvbS2Std::loadDraSchemeSimulationFile(std::string filename)
{
	// associate the simulation file with the list of STs
	if(!this->satellite_terminals.setDraSchemeSimuFile(filename))
	{
		UTI_ERROR("failed to associate DRA scheme simulation file %s"
		          "with the list of STs\n", filename.c_str());
		this->dra_scheme_definitions->clear();
		return 0;
	}

	return 1;
}

DraSchemeDefinitionTable *DvbS2Std::getDraSchemeDefinitions()
{
	return this->dra_scheme_definitions;
}


bool DvbS2Std::createIncompleteBBFrame(BBFrame **bbframe,
                                       unsigned int modcod_id)
{
	// if there is no incomplete BB frame create a new one
	std::string rate;
	unsigned int bbframe_size;

	*bbframe = new BBFrame();
	if(bbframe == NULL)
	{
		UTI_ERROR("failed to create an incomplete BB frame\n");
		goto error;
	}

	if(!this->packet_handler)
	{
		UTI_ERROR("packet handler is NULL\n");
		goto error;
	}

	// set the MODCOD ID of the BB frame
	(*bbframe)->setModcodId(modcod_id);

	// set the type of encapsulation packets the BB frame will contain
	(*bbframe)->setEncapPacketEtherType(this->packet_handler->getEtherType());

	// get the coding rate of the MODCOD and the corresponding BB frame size
	rate = this->modcod_definitions.getCodingRate(modcod_id);
	bbframe_size = this->getPayload(rate);
	UTI_DEBUG_L3("size of the BBFRAME for MODCOD %d = %d\n",
	             modcod_id, bbframe_size);

	// set the size of the BB frame
	(*bbframe)->setMaxSize(bbframe_size);

	return true;

error:
	return false;
}


bool DvbS2Std::retrieveCurrentModcod(long tal_id,
                                     unsigned int &modcod_id)
{
	bool do_advertise_modcod;

	// retrieve the current MODCOD for the ST and whether
	// it changed or not
	if(!this->satellite_terminals.do_exist(tal_id))
	{
		UTI_ERROR("encapsulation packet is for ST with ID %ld "
		          "that is not registered\n", tal_id);
		goto error;
	}
	do_advertise_modcod = !this->satellite_terminals.isCurrentModcodAdvertised(tal_id);
	if(!do_advertise_modcod)
	{
		modcod_id = this->satellite_terminals.getCurrentModcodId(tal_id);
	}
	else
	{
		modcod_id = this->satellite_terminals.getPreviousModcodId(tal_id);
	}
	UTI_DEBUG_L3("MODCOD for ST ID %ld = %u (changed = %s)\n",
	             tal_id, modcod_id,
	             do_advertise_modcod ? "yes" : "no");

#if 0 /* TODO: manage options */
		if(do_advertise_modcod)
		{
			this->createOptionModcod(comp, nb_row, *modcod_id, id);
		}
#endif

	return true;

error:
	return false;
}

bool DvbS2Std::getBBFrameDuration(unsigned int modcod_id,
                                  float &bbframe_duration)
{
	float spectral_efficiency;

	if(!this->modcod_definitions.do_exist(modcod_id))
	{
		UTI_ERROR("failed to found the definition of MODCOD ID %u\n",
		          modcod_id);
		goto error;
	}

	spectral_efficiency = this->modcod_definitions.getSpectralEfficiency(modcod_id);

	// duration is calculated in ms not in s
	// TODO remove test once bandwidth will be initialized in constructor
	if(this->bandwidth != 0)
	{
		bbframe_duration = (MSG_BBFRAME_SIZE_MAX * 8) /
		                   (spectral_efficiency * this->bandwidth * 1000);
	}
	else
	{
		UTI_ERROR("bandwidth not initialized\n");
		goto error;
	}


	UTI_DEBUG("duration of the BBFRAME = %f ms\n", bbframe_duration);

	return true;

error:
	return false;
}

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
		          bbframe_burst->pkt_type, this->packet_handler->getEtherType());
		goto error;
	}
	UTI_DEBUG("BB frame received (%d %s packet(s)\n",
	           bbframe_burst->dataLength, this->packet_handler->getName().c_str());

	// retrieve the current real MODCOD of the receiver
	// (do this before any MODCOD update occurs)
	real_mod = this->realModcod;

	// check if there is an update of the real MODCOD among all the real
	// MODCOD options located just after the header of the BB frame
	for(i = 0; i < bbframe_burst->list_realModcod_size; i++)
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
#if 0 // TODO: enable this once real MODCOD options are managed
			// check if the value is not outside the values of the file
			this->realModcod = real_modcod_option->real_modcod);
#endif
		}
	}

	// used for terminal statistics
	this->receivedModcod = bbframe_burst->usedModcod;

	// is the ST able to decode the received BB frame ?
	if(bbframe_burst->usedModcod > real_mod)
	{
		// the BB frame is not robust enough to be decoded, drop it
		UTI_ERROR("received BB frame is encoded with MODCOD %d and "
		          "the real MODCOD of the BB frame (%d) is not "
		          "robust enough, so emulate a lost BB frame\n",
		          bbframe_burst->usedModcod, real_mod);
		goto drop;
	}

	if(bbframe_burst->dataLength <= 0)
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
	         bbframe_burst->list_realModcod_size * sizeof(T_DVB_REAL_MODCOD);
	for(i = 0; i < bbframe_burst->dataLength; i++)
	{
		NetPacket *encap_packet;
		size_t current_length;

		current_length = this->packet_handler->getLength(frame + offset +
		                                                 previous_length);
		// Use default values for QoS, source/destination tal_id
		encap_packet = this->packet_handler->build(frame + offset + previous_length,
		                                           current_length,
		                                           0x00, BROADCAST_TAL_ID, BROADCAST_TAL_ID);
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
	g_memory_pool_dvb_rcs.release((char *) frame);
	return 0;

release_burst:
	delete burst;
error:
	g_memory_pool_dvb_rcs.release((char *) frame);
	return -1;
}

bool DvbS2Std::getIncompleteBBFrame(unsigned int tal_id,
                                    BBFrame **bbframe)
{
	std::map<unsigned int, BBFrame *>::iterator iter;
	unsigned int modcod_id;

	*bbframe = NULL;

	// retrieve the current MODCOD for the ST
	if(!this->retrieveCurrentModcod(tal_id, modcod_id))
	{
		// cannot get modcod for the ST skip this element
		goto skip;
	}

	// find if the BBFrame exists
	iter = this->incomplete_bb_frames.find(modcod_id);
	if(iter != this->incomplete_bb_frames.end() && (*iter).second != NULL)
	{
		UTI_DEBUG("Found a BBFrame for MODCOD %u\n", modcod_id);
		*bbframe = (*iter).second;
	}
	// no BBFrame for this MOCDCOD create a new one
	else
	{
		UTI_DEBUG("Create a new BBFrame for MODCOD %u\n", modcod_id);
		// if there is no incomplete BB frame create a new one
		if(!this->createIncompleteBBFrame(bbframe, modcod_id))
		{
			goto error;
		}
		// add the BBFrame in the map and list
		this->incomplete_bb_frames[modcod_id] = *bbframe;
		this->incomplete_bb_frames_ordered.push_back(*bbframe);

	}

skip:
	return true;
error:
	return false;
}


int DvbS2Std::addCompleteBBFrame(std::list<DvbFrame *> *complete_bb_frames,
                                 BBFrame *bbframe,
                                 float &duration_credit)
{
	unsigned int modcod_id = bbframe->getModcodId();
	float bbframe_duration;

	// how much time do we need to send the BB frame ?
	if(!this->getBBFrameDuration(modcod_id, bbframe_duration))
	{   
		UTI_ERROR("failed to get BB frame duration "
		          "(MODCOD ID = %u)\n", modcod_id);
		return -1;
	}   
   
	// not enough space for this BBFrame
	if(duration_credit < bbframe_duration)
	{
		UTI_DEBUG("not enough duration credit (%f) for the BBFrame of "
		          "duration %f\n", duration_credit, bbframe_duration);
		return -2;
	}

	// we can send the BBFrame
	complete_bb_frames->push_back(bbframe);

	// reduce the time credit by the time of the BB frame
	duration_credit -= bbframe_duration;

	return 0;
}




/**
 * @file DvbS2Std.cpp
 * @brief DVB-S2 Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#include "DvbS2Std.h"
#include "MpegPacket.h"
#include "GsePacket.h"

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "platine_conf/uti_debug.h"


DvbS2Std::DvbS2Std():
	PhysicStd("DVB-S2"),
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
	unsigned int cpt_frame = 0;
	unsigned int sent_packets = 0;
	MacFifoElement *elem;
	long max_to_send;
	float duration_credit;
	// reinitialize the current BBFrame parameters to avoid using old values
	this->bbframe_duration = 0;
	this->incomplete_bb_frame = NULL;
	this->modcod_id = 0;

	// retrieve the number of packets waiting for retransmission
	max_to_send = fifo->getCount();
	if(max_to_send <= 0)
	{
		// if there is nothing to send, return with success
		// erase remaining credit as FIFO is empty (TODO is it ok ??)
		this->remainingCredit = 0;
		goto skip;
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

	// now build BB frames with packets extracted from the MAC FIFO
	while((elem = (MacFifoElement *)fifo->get()) != NULL)
	{
		NetPacket *encap_packet;
		int ret;
		long tal_id;

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
		tal_id = encap_packet->talId();

		ret = this->initializeIncompleteBBFrame(tal_id);
		if(ret == -1)
		{
			// cannot initialize incomplete BB Frame
			delete encap_packet;
			goto error_fifo_elem;
		}
		else if(ret == -2)
		{
			// cannot get modcod for the ST remove the fifo element
			fifo->remove();
			delete encap_packet;
			delete elem;
			continue;
		}

		// is there enough free space in the BB frame that corresponds
		// to the MODCOD ID for the encapsulation packet ?
		if(encap_packet->totalLength() >
		   this->incomplete_bb_frame->getFreeSpace())
		{
			int process_res;

			UTI_DEBUG("BB frame with MODCOD ID %u does not contain "
			          "enough free space (%u bytes) "
			          "for the packet to encapsulate (%u bytes)\n",
			          this->modcod_id, this->incomplete_bb_frame->getFreeSpace(),
			          encap_packet->totalLength());

			if(this->encapPacketType == PKT_TYPE_MPEG)
			{
				process_res = this->processMpegPacket(complete_dvb_frames,
				                                      encap_packet,
				                                      &duration_credit,
				                                      &cpt_frame);
			}
			else if(this->encapPacketType == PKT_TYPE_GSE)
			{
				process_res = this->processGsePacket(complete_dvb_frames,
				                                     &encap_packet,
				                                     &duration_credit,
				                                     &cpt_frame,
				                                     sent_packets,
				                                     elem);
			}
			else
			{
				UTI_ERROR("Bad packet type (%d) in DVB FIFO\n",
				          this->incomplete_bb_frame->getEncapPacketType());
				delete encap_packet;
				goto error_fifo_elem;
			}
			// handle the processXXXPacket() return code
			if(process_res == -1)
			{
				// error when processing packet
				delete encap_packet;
				goto error_fifo_elem;
			}
			else if(process_res == -2)
			{
				goto skip;
			}
		}

		// add the encapsulation packet to the BB frame
		if(!this->incomplete_bb_frame->addPacket(encap_packet))
		{
			UTI_ERROR("failed to add encapsulation packet #%u "
			          "in BB frame with MODCOD ID %u",
			          sent_packets + 1, this->modcod_id);
			delete encap_packet;
			goto error_fifo_elem;
		}
		sent_packets++;

		// remove the element from the MAC FIFO and destroy it
		delete encap_packet;
		fifo->remove();
		delete elem;
	}

	// add the incomplete BB frame to the list of complete BB frame
	// if it is not empty
	if(this->incomplete_bb_frame != NULL)
	{
		if(this->incomplete_bb_frame->getNumPackets() > 0)
		{
			complete_dvb_frames->push_back(this->incomplete_bb_frame);

			// increment the counter of complete frames
			cpt_frame++;
		}
		else
		{
			delete this->incomplete_bb_frame;
			this->incomplete_bb_frame = NULL;
		}
	}

skip:
	if(sent_packets != 0)
	{
		UTI_DEBUG("%u %s been scheduled and %u BB %s completed\n",
		          sent_packets, (sent_packets > 1) ? "packets have" : "packet has",
		          cpt_frame, (cpt_frame > 1) ? "frames were" : "frame was");
	}
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


bool DvbS2Std::createIncompleteBBFrame()
{
	// if there is no incomplete BB frame create a new one
	std::string rate;
	unsigned int bbframe_size;
	BBFrame *bbframe;

	if(this->encapPacketType != PKT_TYPE_GSE &&
	   this->encapPacketType != PKT_TYPE_MPEG)
	{
		UTI_ERROR("invalid packet type (%d) in DvbS2 class\n",
		          this->encapPacketType);
		goto error;
	}

	bbframe = new BBFrame();
	if(bbframe == NULL)
	{
		UTI_ERROR("failed to create an incomplete BB frame\n");
		goto error;
	}

	// set the MODCOD ID of the BB frame
	bbframe->setModcodId(this->modcod_id);

	// set the type of encapsulation packets the BB frame will contain
	bbframe->setEncapPacketType(this->encapPacketType);

	// get the coding rate of the MODCOD and the corresponding BB frame size
	rate = this->modcod_definitions.getCodingRate(this->modcod_id);
	bbframe_size = this->getPayload(rate);
	UTI_DEBUG_L3("size of the BBFRAME for MODCOD %d = %d\n",
	             this->modcod_id, bbframe_size);

	// set the size of the BB frame
	bbframe->setMaxSize(bbframe_size);

	this->incomplete_bb_frame = bbframe;

	return true;

error:
	return false;
}


bool DvbS2Std::retrieveCurrentModcod(long tal_id)
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
		this->modcod_id = this->satellite_terminals.getCurrentModcodId(tal_id);
	}
	else
	{
		this->modcod_id = this->satellite_terminals.getPreviousModcodId(tal_id);
	}
	UTI_DEBUG_L3("MODCOD for ST ID %ld = %u (changed = %s)\n",
	             tal_id, this->modcod_id,
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

bool DvbS2Std::getBBFRAMEDuration()
{
	float spectral_efficiency;

	if(!this->modcod_definitions.do_exist(this->modcod_id))
	{
		UTI_ERROR("failed to found the definition of MODCOD ID %u\n",
		          this->modcod_id);
		goto error;
	}

	spectral_efficiency = this->modcod_definitions.getSpectralEfficiency(this->modcod_id);

	// duration is calculated in ms not in s
	// TODO remove test once bandwidth will be initialized in constructor
	if(this->bandwidth != 0)
	{
		this->bbframe_duration = (MSG_BBFRAME_SIZE_MAX * 8) /
		                         (spectral_efficiency * this->bandwidth * 1000);
	}
	else
	{
		UTI_ERROR("bandwidth not initialized\n");
		goto error;
	}


	UTI_DEBUG("duration of the BBFRAME = %f ms\n", this->bbframe_duration);

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
	long i;                       // counter for MPEG/GSE packets
	int real_mod;                 // real modcod of the receiver

	// Store total length of previous GSE packets in the frame
	uint16_t previous_gse_length = 0;
	// Store length of the GSE packet
	uint16_t current_gse_length;
	// Offset from beginning of frame to beginning of data
	unsigned int offset;

	// sanity check
	if(frame == NULL)
	{
		UTI_ERROR("invalid frame received\n");
		goto error;
	}

	bbframe_burst = (T_DVB_BBFRAME *) frame;

	// sanity check: this function only handle BB frames
	if(type != MSG_TYPE_BBFRAME)
	{
		UTI_ERROR("the message received is not a BB frame\n");
		goto error;
	}
	switch(bbframe_burst->pkt_type)
	{
		case(PKT_TYPE_MPEG):
			UTI_DEBUG("BB frame received (%d MPEG packet(s)\n",
			          bbframe_burst->dataLength);
			break;
		case(PKT_TYPE_GSE):
			UTI_DEBUG("BB frame received (%d GSE packet(s)\n",
			          bbframe_burst->dataLength);
			break;
		default:
			UTI_ERROR("Bad packet type (%d) in BB frame burst",
			          bbframe_burst->pkt_type);
			goto error;
	}
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

	// add MPEG/GSE packets received from lower layer
	// to the newly created burst
	offset = sizeof(T_DVB_BBFRAME) +
	         bbframe_burst->list_realModcod_size * sizeof(T_DVB_REAL_MODCOD);
	for(i = 0; i < bbframe_burst->dataLength; i++)
	{
		NetPacket *encap_packet;

		switch(bbframe_burst->pkt_type)
		{
			case(PKT_TYPE_MPEG):
				encap_packet = new MpegPacket(
				                   frame + offset +
				                   i * MpegPacket::length(), MpegPacket::length());
				if(encap_packet == NULL)
				{
					UTI_ERROR("cannot create one MPEG packet\n");
					goto release_burst;
				}
				UTI_DEBUG("MPEG packet (%d bytes) added to burst\n",
				          encap_packet->totalLength());
				break;

			case(PKT_TYPE_GSE):
				current_gse_length =
				    GsePacket::length(frame, previous_gse_length + offset);
				encap_packet = new GsePacket(
				                   frame + offset +
				                   previous_gse_length,
				                   current_gse_length);
				if(encap_packet == NULL)
				{
					UTI_ERROR("cannot create one GSE packet\n");
					goto release_burst;
				}
				previous_gse_length += current_gse_length;
				UTI_DEBUG("GSE packet (%d bytes) added to burst\n",
				          encap_packet->totalLength());
				break;

			default:
				UTI_ERROR("Bad packet type (%d) in BB frame burst\n",
				          bbframe_burst->pkt_type);
				goto release_burst;
		}

		if(this->tal_id != -1 && encap_packet->talId() != this->tal_id)
		{
			UTI_DEBUG("packet with id %ld ignored (%ld expected), "
			          "this should not append in transparent mode\n",
			          encap_packet->talId(), this->tal_id);
			delete encap_packet;
			continue;
		}
		// add the packet to the burst of packets
		(*burst)->add(encap_packet);
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

int DvbS2Std::initializeIncompleteBBFrame(unsigned int tal_id)
{
	// retrieve the current MODCOD for the ST
	if(!this->retrieveCurrentModcod(tal_id))
	{
		// cannot get modcod for the ST skip this element
		goto skip;
	}

	// how much time do we need to send the BB frame ?
	if(!this->getBBFRAMEDuration())
	{
		UTI_ERROR("failed to get BB frame duration "
				  "(MODCOD ID = %u)\n", this->modcod_id);
		goto error;
	}

	// if there is no incomplete BB frame create a new one
	if(this->incomplete_bb_frame == NULL)
	{
		if(!this->createIncompleteBBFrame())
		{
			goto error;
		}
	}

	return 0;

error:
	return -1;
skip:
	return -2;
}

int DvbS2Std::processMpegPacket(std::list<DvbFrame *> *complete_bb_frames,
                                NetPacket *encap_packet,
                                float *duration_credit,
                                unsigned int *cpt_frame)
{
	// not enough free space in the BB frame: the
	// encapsulation is MPEG2-TS which is of constant
	// length (188 bytes) so we can not fragment the
	// packet and we must complete the BB frame with
	// padding. So:
	//  - add padding to the BB frame (not done yet TODO)
	//  - put the BB frame in the list of complete frames
	//  - use a new BB frame
	//  - put the encapsulation packet in this new BB frame
	//    (this is done oustide this function)

	int ret;

	UTI_DEBUG("pad the BB frame if necessary and send it\n");

	ret = this->addCompleteBBFrame(complete_bb_frames,
	                               duration_credit,
	                               cpt_frame);
	if(ret == -1)
	{
		goto error;
	}
	else if(ret == -2)
	{
		goto skip;
	}

	// is there enough free space in the new BB frame ?
	if(encap_packet->totalLength() > this->incomplete_bb_frame->getFreeSpace())
	{
		UTI_ERROR("BB frame with MODCOD ID %u got no "
		          "enough free space, this should never "
		          "happen\n", this->modcod_id);
		goto error;
	}

	return 0;

error:
	return -1;
skip:
	return -2;
}

int DvbS2Std::processGsePacket(std::list<DvbFrame *> *complete_bb_frames,
                               NetPacket **encap_packet,
                               float *duration_credit,
                               unsigned int *cpt_frame,
                               unsigned int sent_packets,
                               MacFifoElement *elem)
{
	// not enough free space in the BB frame: the
	// encapsulation is GSE, so we can refragment the
	// packet. Thus:
	//  - refragment the packet
	//  - add packet to the BB frame
	//  - put the BB frame in the list of complete frames
	//  - use a new BB frame
	//  - put the second fragment in this new BB frame,
	//    refragment it and do this as any time as necessary

	while((*encap_packet)->totalLength() >
		  this->incomplete_bb_frame->getFreeSpace())
	{
		int ret;
		gse_vfrag_t *first_frag;
		gse_vfrag_t *second_frag;
		GsePacket *encap_packet_frag;
		gse_status_t status;
		uint8_t qos;
		unsigned long mac_id;
		long tal_id;

		// get the packet information to correctly create fragments
		qos = (*encap_packet)->qos();
		mac_id = (*encap_packet)->macId();
		tal_id = (*encap_packet)->talId();

		UTI_DEBUG("refragment the GSE packet\n");

		UTI_DEBUG_L3("Create a virtual fragment with GSE packet to refragment it\n");
		status = gse_create_vfrag_with_data(&first_frag,
											(*encap_packet)->totalLength(),
											GSE_MAX_REFRAG_HEAD_OFFSET, 0,
											(unsigned char *)(*encap_packet)->data().c_str(),
											(*encap_packet)->totalLength());
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("Failed to create a virtual fragment for the GSE packet "
					  "refragmentation (%s)\n", gse_get_status(status));
			goto error;
		}

		UTI_DEBUG_L3("Refragement the GSE packet to fit the BB frame "
					 "(length = %d)\n", this->incomplete_bb_frame->getFreeSpace());
		status = gse_refrag_packet(first_frag, &second_frag, 0, 0, qos,
								   this->incomplete_bb_frame->getFreeSpace());
		if(status == GSE_STATUS_LENGTH_TOO_SMALL)
		{
			// there is not enough space to create a GSE fragment,
			// pad the BB frame and send it. The packet will be handled either
			// next loop iteration or outside the loop

			// TODO: be sure not to constantly loop here if an empty BB frame
			// is smaller than the minimum GSE packet size
			UTI_DEBUG("Unable to refragment GSE packet (%s), pad the BB frame, "
					  "and send it\n", gse_get_status(status));
			// first_frag contains the whole encap packet
			// but it could not be fragmented : it is useless so let's free it
			status = gse_free_vfrag(&first_frag);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("Failed to free virtual fragment (%s)\n",
						  gse_get_status(status));
				goto error;
			}
			if(second_frag != NULL)
			{
				gse_free_vfrag(&second_frag);
			}

			ret = this->addCompleteBBFrame(complete_bb_frames,
			                               duration_credit,
			                               cpt_frame);
			if(ret == -1)
			{
				goto error;
			}
			else if(ret == -2)
			{
				goto skip;
			}
		}
		else if(status == GSE_STATUS_OK)
		{
			// add the first fragment to the BB frame
			encap_packet_frag = new GsePacket(gse_get_vfrag_start(first_frag),
											  gse_get_vfrag_length(first_frag));
			if(encap_packet_frag == NULL)
			{
				UTI_ERROR("failed to create the first fragment\n");
				goto error;
			}
			// set the packet information
			(*encap_packet)->setQos(qos);
			(*encap_packet)->setMacId(mac_id);
			(*encap_packet)->setTalId(tal_id);

			status = gse_free_vfrag(&first_frag);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("Failed to free the first virtual fragment (%s)\n",
						  gse_get_status(status));
				goto error;
			}
			if(!this->incomplete_bb_frame->addPacket(encap_packet_frag))
			{
				UTI_ERROR("failed to add a packet #%u fragment "
						  "in BB frame with MODCOD ID %u",
						  sent_packets + 1, this->modcod_id);
				delete encap_packet_frag;
				goto error;
			}
			// do not increase the sent_packets number as it is juste fragments

			ret = this->addCompleteBBFrame(complete_bb_frames,
			                               duration_credit,
			                               cpt_frame);
			delete encap_packet_frag;
			delete *encap_packet;
			if(ret == -1)
			{
				goto error;
			}
			else
			{
				// in case of success or skip, replace encap_packet by the
				// second fragment, it will either be refragmented,
				// encapsulated completely or kept in the FIFO in skip case.
				// be careful to keep the ret value for skip case

				// only the second fragment is not encapsulated yet
				// create a new encap_packet containing the second fragment
				*encap_packet = new GsePacket(gse_get_vfrag_start(second_frag),
				                              gse_get_vfrag_length(second_frag));
				if(*encap_packet == NULL)
				{
					UTI_ERROR("failed to create the second fragment\n");
					goto error;
				}
				// set the packet information
				(*encap_packet)->setQos(qos);
				(*encap_packet)->setMacId(mac_id);
				(*encap_packet)->setTalId(tal_id);
				// store the packet in the FIFO in case of skip
				elem->setPacket(*encap_packet);
	
				status = gse_free_vfrag(&second_frag);
				if(status != GSE_STATUS_OK)
				{
					UTI_ERROR("Failed to free the second virtual fragment (%s)\n",
							  gse_get_status(status));
					goto error;
				}

				// skip if there is not enough credit for a new BB Frame
				if(ret == -2)
				{
					goto skip;
				}
			}
		}
		else
		{
			UTI_ERROR("Failed to refragment GSE packet (%s)\n",
					  gse_get_status(status));
			goto error;
		}
	}

	return 0;

error:
	return -1;
skip:
	return -2;
}


int DvbS2Std::addCompleteBBFrame(std::list<DvbFrame *> *complete_bb_frames,
                                 float *duration_credit,
                                 unsigned int *cpt_frame)
{
	complete_bb_frames->push_back(this->incomplete_bb_frame);

	// reduce the time credit by the time of the BB frame
	(*duration_credit) -= this->bbframe_duration;

	// increment the counter of complete frames
	(*cpt_frame)++;

	// do we have enough credit to create another BB frame ?
	if(this->bbframe_duration > *duration_credit)
	{
		// can not send the BB frame, stop algorithm here
		// store the remaining credit
		this->remainingCredit = *duration_credit;
		UTI_DEBUG("too few credit (%.2f ms) to create another "
		          "BB frame which needs %.2f ms for MODCOD %u. "
		          "Add the remaining credit to the next frame duration\n",
		          *duration_credit, this->bbframe_duration, this->modcod_id);
		goto skip;
	}

	// create a new incomplete BB frame
	if(!this->createIncompleteBBFrame())
	{
		goto error;
	}

	return 0;

error:
	return -1;
skip:
	return -2;
}

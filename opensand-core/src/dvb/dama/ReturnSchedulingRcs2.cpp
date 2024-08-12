/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */


/**
 * @file     ReturnSchedulingRcs2.cpp
 * @brief    The scheduling functions for MAC FIFOs with DVB-RCS2 return link
 * @author   Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author   Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "ReturnSchedulingRcs2.h"
#include "DvbFifo.h"
#include "FifoElement.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <limits>


constexpr const uint32_t max_allocation = std::numeric_limits<vol_kb_t>::max();


ReturnSchedulingRcs2::ReturnSchedulingRcs2(SimpleEncapPlugin*  packet_handler,
                                           std::shared_ptr<fifos_t> fifos):
	Scheduling(packet_handler, fifos, nullptr),
	max_burst_length_b(0)
{
}


vol_b_t ReturnSchedulingRcs2::getMaxBurstLength() const
{
	return this->max_burst_length_b;
}


void ReturnSchedulingRcs2::setMaxBurstLength(vol_b_t length_b)
{
	this->max_burst_length_b = length_b;
	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "DVB-RCS frame max burst length: %u bits (%u bytes)\n",
	    this->max_burst_length_b, this->max_burst_length_b >> 3);
}


bool ReturnSchedulingRcs2::schedule(const time_sf_t current_superframe_sf,
                                    std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                    uint32_t &remaining_allocation)
{
	if(remaining_allocation > max_allocation)
	{
		LOG(this->log_scheduling, LEVEL_NOTICE,
		    "Remaining allocation (%u) is too long and will be "
		    "truncated\n", remaining_allocation);
	}
	// check max burst length
	if(this->max_burst_length_b <= 0)
	{
		LOG(this->log_scheduling, LEVEL_NOTICE,
		    "The max burst length does not allow to send data\n");
		return true;
	}
	// extract and send encap packets from MAC FIFOs, in function of
	// UL allocation
	if(!this->macSchedule(current_superframe_sf,
	                      complete_dvb_frames,
	                      (vol_b_t &)remaining_allocation))
	{
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "SF#%u: MAC scheduling failed\n",
		    current_superframe_sf);
		return false;
	}

	return true;
}


bool ReturnSchedulingRcs2::schedulePacket(const time_sf_t current_superframe_sf,
                                          unsigned int &sent_packets,
                                          unsigned int &complete_frames_count,
                                          vol_b_t &frame_length_b,
                                          vol_b_t &remaining_allocation_b,
                                          std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                          Rt::Ptr<DvbRcsFrame> &incomplete_dvb_frame,
                                          Rt::Ptr<NetPacket> encap_packet)
{
	while (encap_packet)
	{
		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: Extracted packet: %d kbits (%d bytes)",
		    current_superframe_sf,
		    (encap_packet->getTotalLength() << 3) / 1000,
		    encap_packet->getTotalLength());

		Rt::Ptr<NetPacket> data = Rt::make_ptr<NetPacket>(nullptr);
		int ret = this->packet_handler->encapNextPacket(std::move(encap_packet),
		                                                incomplete_dvb_frame->getFreeSpace(),
		                                                incomplete_dvb_frame->getPacketsCount() == 0,
		                                                data, this->remaining_data);
		if(!ret)
		{
			LOG(this->log_scheduling, LEVEL_ERROR,
			    "SF#%u: error while processing packet "
			    "#%u\n", current_superframe_sf,
			    sent_packets + 1);
			// continue anyways with other packets in the fifo
			return true;
		}

		LOG(this->log_scheduling, LEVEL_DEBUG,
		    "SF#%u: %s encapsulated packet length = %d kbits (%d bytes)",
		    current_superframe_sf,
		    this->remaining_data ? "Partial" : "Complete",
		    data ? (data->getTotalLength() << 3) / 1000 : 0,
		    data ? data->getTotalLength() : 0);

		// Check the frame allows data
		bool missing_alloc = this->remaining_data != nullptr;
	
		if (data)
		{
			// Add data to the frame
			if(!incomplete_dvb_frame->addPacket(*data))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: failed to add encapsulation "
				    "packet #%u->in DVB-RCS2 frame with MODCOD ID %u "
				    "(packet length %zu, free space %zu)",
				    current_superframe_sf,
				    sent_packets + 1,
				    incomplete_dvb_frame->getModcodId(),
				    data->getTotalLength(),
				    incomplete_dvb_frame->getFreeSpace());
				return false;
			}

			// Delete the NetPacket once it has been copied in the DVB-RCS2 Frame
			frame_length_b += data->getTotalLength() << 3;
			sent_packets++;

			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: DVB Frame filling (%d packets): used %d kbits (%d bytes), free %d kbits (%d bytes)",
				current_superframe_sf,
				incomplete_dvb_frame->getNumPackets(),
				frame_length_b / 1000,
				frame_length_b >> 3,
				(incomplete_dvb_frame->getFreeSpace() << 3) / 1000,
				incomplete_dvb_frame->getFreeSpace());

			// Check the frame is completed
			if(incomplete_dvb_frame->getFreeSpace() <= 0)
			{
				missing_alloc = true;
			}
			
			// Check there is enough remaining allocation
			if(remaining_allocation_b <= frame_length_b)
			{
				missing_alloc = true;
			}
		}
		else
		{
			missing_alloc = true;
		}

		if (missing_alloc)
		{
			// is there any packets in the current DVB-RCS frame ?
			if(incomplete_dvb_frame->getNumPackets() <= 0)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "DVB-RCS2 frame #%u got no packets, "
				    "this should never append (free space %u bytes)\n",
				    complete_frames_count + 1,
				    incomplete_dvb_frame->getFreeSpace());

				frame_length_b = 0;
				return false;
			}
			
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: DVB frame #%u "
			    "is full, change for next one\n",
			    current_superframe_sf,
			    complete_frames_count + 1);

			// Store DVB-RCS2 frame with completed frames
			complete_dvb_frames.push_back(dvb_frame_downcast(std::move(incomplete_dvb_frame)));
			complete_frames_count++;
			remaining_allocation_b -= std::min(frame_length_b, remaining_allocation_b);
			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: %d DVB frames completed, remaining allocation %d kbits (%d bytes)",
				current_superframe_sf,
				complete_frames_count,
				remaining_allocation_b / 1000,
				remaining_allocation_b >> 3);

			// Check the remaining allocation
			if(!remaining_allocation_b)
			{
				incomplete_dvb_frame.reset();
				return true;
			}

			// Create a new incomplete DVB-RCS2 frame
			if(!this->allocateDvbRcsFrame(incomplete_dvb_frame))
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: failed to create a new DVB frame\n",
				    current_superframe_sf);
				return false;
			}
			frame_length_b = 0;
			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: DVB Frame filling (%d packets): used %d kbits (%d bytes), free %d kbits (%d bytes)",
				current_superframe_sf,
				incomplete_dvb_frame->getNumPackets(),
				frame_length_b / 1000,
				frame_length_b >> 3,
				(incomplete_dvb_frame->getFreeSpace() << 3) / 1000,
				incomplete_dvb_frame->getFreeSpace());
		}

		// Replace the fifo current element with the remaining data
		encap_packet = std::move(this->remaining_data);
	}

	return true;
}


bool ReturnSchedulingRcs2::macSchedule(const time_sf_t current_superframe_sf,
                                       std::list<Rt::Ptr<DvbFrame>> &complete_dvb_frames,
                                       vol_b_t &remaining_allocation_b)
{
	Rt::Ptr<DvbRcsFrame> incomplete_dvb_frame = Rt::make_ptr<DvbRcsFrame>(nullptr);

	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: attempt to extract encap packets from MAC"
	    " FIFOs (remaining allocation = %d kbits)\n",
	    current_superframe_sf,
	    remaining_allocation_b / 1000);

	// create an incomplete DVB-RCS frame
	if(!this->allocateDvbRcsFrame(incomplete_dvb_frame))
	{
		return false;
	}

	vol_b_t frame_length_b = 0;
	//frame_length_b = incomplete_dvb_frame->getHeaderLength() << 3;

	// extract encap packets from MAC FIFOs while some UL capacity is available
	// (MAC fifos priorities are in MAC IDs order)
	// fifo are classified by priority value (map are ordered)
	unsigned int complete_frames_count = 0;
	unsigned int sent_packets = 0;

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "SF#%u: %d DVB frames completed, remaining allocation %d kbits (%d bytes)",
	    current_superframe_sf,
	    complete_frames_count,
	    remaining_allocation_b / 1000,
	    remaining_allocation_b >> 3);
	LOG(this->log_scheduling, LEVEL_DEBUG,
		"SF#%u: DVB Frame filling (%d packets): used %d kbits (%d bytes), free %d kbits (%d bytes)",
		current_superframe_sf,
		incomplete_dvb_frame->getNumPackets(),
		frame_length_b / 1000,
		frame_length_b >> 3,
		(incomplete_dvb_frame->getFreeSpace() << 3) / 1000,
		incomplete_dvb_frame->getFreeSpace());

	if (!this->schedulePacket(current_superframe_sf,
	                          sent_packets,
	                          complete_frames_count,
	                          frame_length_b,
	                          remaining_allocation_b,
	                          complete_dvb_frames,
	                          incomplete_dvb_frame,
	                          std::move(this->remaining_data)))
	{
		return false;
	}

	for (auto&& fifo_it: *(this->dvb_fifos))
	{
		if (!remaining_allocation_b)
		{
			break;
		}

		DvbFifo &fifo = *(fifo_it.second);
		if (fifo.getAccessType() == ForwardOrReturnAccessType{ReturnAccessType::saloha})
		{
			// not the good fifo
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: ignore MAC FIFO %s  "
			    "not the right access type (%d)\n",
			    current_superframe_sf,
			    fifo.getName(),
			    fifo.getAccessType());
			continue;
		}

		for (auto &&elem: fifo)
		{
			// FIFO with awaiting data
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: extract packet from "
			    "MAC FIFO %s: "
			    "%u awaiting packets (remaining "
			    "allocation = %d kbits)\n",
			    current_superframe_sf,
			    fifo.getName(),
			    fifo.getCurrentSize(),
			    remaining_allocation_b / 1000);

			Rt::Ptr<NetPacket> encap_packet = elem->releaseElem<NetPacket>();
			if(!encap_packet)
			{
				LOG(this->log_scheduling, LEVEL_ERROR,
				    "SF#%u: error while getting packet (null) "
				    "#%u\n", current_superframe_sf,
				    sent_packets + 1);
			}
			else
			{
				// extract next encap packet context from MAC fifo
				if (!this->schedulePacket(current_superframe_sf,
				                          sent_packets,
				                          complete_frames_count,
				                          frame_length_b,
				                          remaining_allocation_b,
				                          complete_dvb_frames,
				                          incomplete_dvb_frame,
				                          std::move(encap_packet)))
				{
					return false;
				}

				if (!remaining_allocation_b)
				{
					break;
				}
			}
		}
	}

	// Add the incomplete DVB-RCS2 frame to the list of complete DVB-RCS2 frame
	// if it is not empty
	if(incomplete_dvb_frame != nullptr)
	{
		if(0 < incomplete_dvb_frame->getNumPackets())
		{
			LOG(this->log_scheduling, LEVEL_DEBUG,
			    "SF#%u: DVB frame #%u "
			    "is full, change for next one\n",
			    current_superframe_sf,
			    complete_frames_count + 1);

			// Store DVB-RCS2 frame with completed frames
			complete_dvb_frames.push_back(dvb_frame_downcast(std::move(incomplete_dvb_frame)));
			complete_frames_count++;
			remaining_allocation_b = (vol_b_t)std::max((int)(remaining_allocation_b - frame_length_b), 0);
			LOG(this->log_scheduling, LEVEL_DEBUG,
				"SF#%u: %d DVB frames completed, remaining allocation %d kbits (%d bytes)",
				current_superframe_sf,
				complete_frames_count,
				remaining_allocation_b / 1000,
				remaining_allocation_b >> 3);
		}
	}

	// Print status
	LOG(this->log_scheduling, LEVEL_INFO,
	    "SF#%u: %d packets extracted from MAC FIFOs, "
	    "%u DVB frame(s) were built (remaining "
	    "allocation = %d kbits)\n", current_superframe_sf,
	    sent_packets, complete_frames_count,
	    remaining_allocation_b / 1000);

	return true;
}

bool ReturnSchedulingRcs2::allocateDvbRcsFrame(Rt::Ptr<DvbRcsFrame> &incomplete_dvb_frame)
{
	vol_bytes_t length_bytes;

	try
	{
		incomplete_dvb_frame = Rt::make_ptr<DvbRcsFrame>();
	}
	catch (const std::bad_alloc&)
	{ 
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS2 frame\n");
		return false;
	}

	// Get the max burst length
	length_bytes = this->max_burst_length_b >> 3;
	if(length_bytes <= 0)
	{
		incomplete_dvb_frame.reset();
		LOG(this->log_scheduling, LEVEL_ERROR,
		    "failed to create DVB-RCS2 frame: invalid burst length\n");
		return false;
	}

	length_bytes += incomplete_dvb_frame->getHeaderLength();
	//length_bytes += sizeof(T_DVB_PHY);
	if(MSG_DVB_RCS_SIZE_MAX < length_bytes)
	{
		length_bytes = MSG_DVB_RCS_SIZE_MAX;
	}

	// set the max size of the DVB-RCS2 frame, also set the type
	// of encapsulation packets the DVB-RCS2 frame will contain
	incomplete_dvb_frame->setMaxSize(length_bytes);

	LOG(this->log_scheduling, LEVEL_DEBUG,
	    "new DVB-RCS2 frame with max length %u bytes (<= %u bytes), "
	    "payload length %u bytes, header length %u bytes\n",
	    incomplete_dvb_frame->getMaxSize(),
	    MSG_DVB_RCS_SIZE_MAX,
	    incomplete_dvb_frame->getFreeSpace(),
	    incomplete_dvb_frame->getHeaderLength());
	return true;
}

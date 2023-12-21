/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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

/*
 * @file DvbFifo.cpp
 * @brief  FIFO queue containing MAC packets
 * @author Julien Bernard / Viveris Technologies
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include <unistd.h>
#include <stdlib.h>
#include <cstring>

#include <opensand_output/Output.h>

#include "DvbFifo.h"
#include "FifoElement.h"


ForwardOrReturnAccessType::ForwardOrReturnAccessType():
	direction{ForwardOrReturnAccessType::Direction::Unknown}
{
}


ForwardOrReturnAccessType::ForwardOrReturnAccessType(ReturnAccessType access_type):
	direction{ForwardOrReturnAccessType::Direction::Return},
	return_access_type{access_type}
{
}


ForwardOrReturnAccessType::ForwardOrReturnAccessType(ForwardAccessType access_type):
	direction{ForwardOrReturnAccessType::Direction::Forward},
	forward_access_type{access_type}
{
}


bool ForwardOrReturnAccessType::IsForwardAccess() const
{
	return direction == ForwardOrReturnAccessType::Direction::Forward;
}


bool ForwardOrReturnAccessType::IsReturnAccess() const
{
	return direction == ForwardOrReturnAccessType::Direction::Return;
}


bool ForwardOrReturnAccessType::operator == (const ForwardOrReturnAccessType& other) const
{
	switch (direction)
	{
		case Direction::Forward:
			return other.direction == Direction::Forward && this->forward_access_type == other.forward_access_type;
		case Direction::Return:
			return other.direction == Direction::Return && this->return_access_type == other.return_access_type;

		default:
			return false;
	}
}


bool ForwardOrReturnAccessType::operator != (const ForwardOrReturnAccessType& other) const
{
	switch (direction)
	{
		case Direction::Forward:
			return other.direction != Direction::Forward || this->forward_access_type != other.forward_access_type;

		case Direction::Return:
			return other.direction != Direction::Return || this->return_access_type != other.return_access_type;

		default:
			return true;
	}
}


DvbFifo::DvbFifo(unsigned int fifo_priority, std::string fifo_name,
                 std::string type_name,
                 vol_pkt_t max_size_pkt):
	DelayFifo(max_size_pkt),
	fifo_priority(fifo_priority),
	fifo_name(fifo_name),
	access_type(),
	vcm_id(),
	new_size_pkt(0),
	cur_length_bytes(0),
	new_length_bytes(0),
	carrier_id(0),
	cni(0)
{
	// Output log
	this->log_dvb_fifo = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Fifo");

	memset(&this->stat_context, '\0', sizeof(mac_fifo_stat_context_t));

	if(type_name == "DAMA_RBDC")
	{
		this->access_type = ForwardOrReturnAccessType{ReturnAccessType::dama_rbdc};
	}
	else if(type_name == "DAMA_VBDC")
	{
		this->access_type = ForwardOrReturnAccessType{ReturnAccessType::dama_vbdc};
	}
	else if(type_name == "SALOHA")
	{
		this->access_type = ForwardOrReturnAccessType{ReturnAccessType::saloha};
	}
	else if(type_name == "DAMA_CRA")
	{
		this->access_type = ForwardOrReturnAccessType{ReturnAccessType::dama_cra};
	}
	else if(type_name == "ACM")
	{
		this->access_type = ForwardOrReturnAccessType{ForwardAccessType::acm};
	}
	else if(type_name.find("VCM") == 0)
	{
		this->access_type = ForwardOrReturnAccessType{ForwardAccessType::vcm};
	}
	else
	{
		LOG(this->log_dvb_fifo, LEVEL_INFO,
		    "unknown CR/Access type of FIFO: %s\n", type_name.c_str());
	}
	if(this->access_type == ForwardAccessType::vcm)
	{
		sscanf(type_name.c_str(), "VCM%d", &this->vcm_id);
	}
}

DvbFifo::DvbFifo(uint8_t carrier_id,
                 vol_pkt_t max_size_pkt,
                 std::string fifo_name):
	DelayFifo(max_size_pkt),
	fifo_priority(0),
	fifo_name(fifo_name),
	access_type(),
	new_size_pkt(0),
	cur_length_bytes(0),
	new_length_bytes(0),
	carrier_id(carrier_id)
{
	// Output log
	this->log_dvb_fifo = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Fifo");

	memset(&this->stat_context, '\0', sizeof(mac_fifo_stat_context_t));
}


std::string DvbFifo::getName() const
{
	return this->fifo_name;
}

ForwardOrReturnAccessType DvbFifo::getAccessType() const
{
	return this->access_type;
}

unsigned int DvbFifo::getVcmId() const
{
	return this->vcm_id;
}

// FIFO priority for ST
unsigned int DvbFifo::getPriority() const
{
	return this->fifo_priority;

}

// FIFO Carrier ID for SAT and GW
uint8_t DvbFifo::getCarrierId() const
{
	return this->carrier_id;
}

vol_pkt_t DvbFifo::getNewSize() const
{
	Rt::Lock lock(this->fifo_mutex);
	return this->new_size_pkt;
}

vol_bytes_t DvbFifo::getNewDataLength() const
{
	Rt::Lock lock(this->fifo_mutex);
	return this->new_length_bytes;
}

void DvbFifo::resetNew(const ForwardOrReturnAccessType cr_type)
{
	if(this->access_type == cr_type)
	{
		Rt::Lock lock(this->fifo_mutex);
		this->new_size_pkt = 0;
		this->new_length_bytes = 0;
	}
}

vol_bytes_t DvbFifo::getCurrentDataLength() const
{
	Rt::Lock lock(this->fifo_mutex);
	return this->cur_length_bytes;
}

void DvbFifo::setCni(uint8_t cni)
{
	this->cni = cni;
}

uint8_t DvbFifo::getCni(void) const
{
	return this->cni;
}

bool DvbFifo::push(Rt::Ptr<NetContainer> elem, time_ms_t duration)
{
	vol_bytes_t length = elem->getTotalLength();

	Rt::Lock lock(this->fifo_mutex);
	if (!this->DelayFifo::push(std::move(elem), duration))
	{
		this->stat_context.drop_pkt_nbr++;
		this->stat_context.drop_bytes += length;
		return false;
	}

	// update counter
	this->new_size_pkt++;
	this->stat_context.current_pkt_nbr = this->queue.size();
	this->stat_context.in_pkt_nbr++;
	this->new_length_bytes += length;
	this->cur_length_bytes += length;
	this->stat_context.current_length_bytes += length;
	this->stat_context.in_length_bytes += length;

	LOG(this->log_dvb_fifo, LEVEL_INFO,
	    "Added %u bytes, new size is %u bytes\n",
	    length, this->cur_length_bytes);

	return true;
}


std::unique_ptr<FifoElement> DvbFifo::pop()
{
	Rt::Lock lock(this->fifo_mutex);

	auto elem = this->queue.begin();
	if (elem != this->queue.end())
	{
		std::unique_ptr<FifoElement> result = std::move(elem->second);
		vol_bytes_t length = result->getTotalLength();
	
		// remove the packet
		this->queue.erase(elem);
		this->cur_length_bytes -= length;
	
		// update counters
		this->stat_context.current_pkt_nbr = this->queue.size();
		this->stat_context.out_pkt_nbr++;
	
		this->stat_context.current_length_bytes -= length;
		this->stat_context.out_length_bytes += length;
	
		LOG(this->log_dvb_fifo, LEVEL_INFO,
		    "Removed %u bytes, new size is %u bytes\n",
		    length, this->cur_length_bytes);

		return result;
	}

	return {nullptr};
}


void DvbFifo::flush()
{
	Rt::Lock lock(this->fifo_mutex);
	this->queue.clear();
	this->new_size_pkt = 0;
	this->new_length_bytes = 0;
	this->cur_length_bytes = 0;
	this->resetStats();
}


void DvbFifo::getStatsCxt(mac_fifo_stat_context_t &stat_info)
{
	Rt::Lock lock(this->fifo_mutex);
	stat_info.current_pkt_nbr = this->stat_context.current_pkt_nbr;
	stat_info.current_length_bytes = this->stat_context.current_length_bytes;
	stat_info.in_pkt_nbr = this->stat_context.in_pkt_nbr;
	stat_info.out_pkt_nbr = this->stat_context.out_pkt_nbr;
	stat_info.in_length_bytes = this->stat_context.in_length_bytes;
	stat_info.out_length_bytes = this->stat_context.out_length_bytes;
	stat_info.drop_pkt_nbr = this->stat_context.drop_pkt_nbr;
	stat_info.drop_bytes = this->stat_context.drop_bytes;

	// reset counters
	this->resetStats();
}

void DvbFifo::resetStats()
{
	this->stat_context.in_pkt_nbr = 0;
	this->stat_context.out_pkt_nbr = 0;
	this->stat_context.in_length_bytes = 0;
	this->stat_context.out_length_bytes = 0;
	this->stat_context.drop_pkt_nbr = 0;
	this->stat_context.drop_bytes = 0;
}

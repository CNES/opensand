/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 */


#include "DvbFifo.h"

#include <opensand_output/Output.h>

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>


DvbFifo::DvbFifo(unsigned int fifo_priority, string fifo_name,
                 string type_name,
                 vol_pkt_t max_size_pkt):
	queue(),
	fifo_priority(fifo_priority),
	fifo_name(fifo_name),
	access_type(),
	vcm_id(),
	new_size_pkt(0),
	max_size_pkt(max_size_pkt),
	carrier_id(0),
	fifo_mutex(fifo_name),
	cni(0)
{
	// Output log
	this->log_dvb_fifo = Output::registerLog(LEVEL_WARNING, "Dvb.Fifo");

	memset(&this->stat_context, '\0', sizeof(mac_fifo_stat_context_t));

	if(type_name == "DAMA_RBDC")
	{
		this->access_type = access_dama_rbdc;
	}
	else if(type_name == "DAMA_VBDC")
	{
		this->access_type = access_dama_vbdc;
	}
	else if(type_name == "SALOHA")
	{
		this->access_type = access_saloha;
	}
	else if(type_name == "DAMA_CRA")
	{
		this->access_type = access_dama_cra;
	}
	else if(type_name == "ACM")
	{
		this->access_type = access_acm;
	}
	else if(type_name.find("VCM") == 0)
	{
		this->access_type = access_vcm;
	}
	else
	{
		LOG(this->log_dvb_fifo, LEVEL_ERROR,
		    "unknown CR/Access type of FIFO: %s\n", type_name.c_str());
	}
	if(this->access_type == access_vcm)
	{
		sscanf(type_name.c_str(), "VCM%d", &this->vcm_id);
	}
}

DvbFifo::DvbFifo(uint8_t carrier_id,
                 vol_pkt_t max_size_pkt,
                 string fifo_name):
	queue(),
	fifo_priority(0),
	fifo_name(fifo_name),
	access_type(0),
	new_size_pkt(0),
	max_size_pkt(max_size_pkt),
	carrier_id(carrier_id),
	fifo_mutex(fifo_name)
{
	memset(&this->stat_context, '\0', sizeof(mac_fifo_stat_context_t));
}


/**
 * Destructor
 */
DvbFifo::~DvbFifo()
{
	this->flush();
}

string DvbFifo::getName() const
{
	return this->fifo_name;
}

int DvbFifo::getAccessType() const
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
	RtLock lock(this->fifo_mutex);
	return this->new_size_pkt;
}

vol_bytes_t DvbFifo::getNewDataLength() const
{
	RtLock lock(this->fifo_mutex);
	return this->new_length_bytes;
}

void DvbFifo::resetNew(ret_access_type_t cr_type)
{
	if(this->access_type == cr_type)
	{
		RtLock lock(this->fifo_mutex);
		this->new_size_pkt = 0;
		this->new_length_bytes = 0;
	}
}

vol_pkt_t DvbFifo::getCurrentSize() const
{
	RtLock lock(this->fifo_mutex);
	return this->queue.size();
}

vol_bytes_t DvbFifo::getCurrentDataLength() const
{
	RtLock lock(this->fifo_mutex);
	return this->cur_length_bytes;
}

vol_pkt_t DvbFifo::getMaxSize() const
{
	RtLock lock(this->fifo_mutex);
	return this->max_size_pkt;
}

clock_t DvbFifo::getTickOut() const
{
	RtLock lock(this->fifo_mutex);
	if(queue.size() > 0)
	{
		return this->queue.front()->getTickOut();
	}
	return 0;
}

void DvbFifo::setCni(uint8_t cni)
{
	this->cni = cni;
}

uint8_t DvbFifo::getCni(void) const
{
	return this->cni;
}

vector<MacFifoElement *> DvbFifo::getQueue(void)
{
	return this->queue;
}

bool DvbFifo::push(MacFifoElement *elem)
{
	RtLock lock(this->fifo_mutex);
	vol_bytes_t length;
	length = elem->getTotalLength();

	if(this->queue.size() >= this->max_size_pkt)
	{
		this->stat_context.drop_pkt_nbr++;
		this->stat_context.drop_bytes += length;
		return false;
	}

	// insert in top of fifo
	this->queue.push_back(elem);
	// update counter
	this->new_size_pkt++;
	this->stat_context.current_pkt_nbr = this->queue.size();
	this->stat_context.in_pkt_nbr++;
	this->new_length_bytes += length;
	this->cur_length_bytes += length;
	this->stat_context.current_length_bytes += length;
	this->stat_context.in_length_bytes += length;

	return true;
}

bool DvbFifo::pushFront(MacFifoElement *elem)
{
	RtLock lock(this->fifo_mutex);

	// insert in head of fifo
	if(this->queue.size() < this->max_size_pkt)
	{
		vol_bytes_t length = elem->getTotalLength();

		this->queue.insert(this->queue.begin(), elem);
		this->cur_length_bytes += length;
		// update counter but not new ones as it is a fragment of an old element
		this->stat_context.current_pkt_nbr = this->queue.size();
		this->stat_context.current_length_bytes += length;
		// remove the remainng part of element from out counter
		this->stat_context.out_length_bytes -= length;
		return true;
	}

	return false;

}

bool DvbFifo::pushBack(MacFifoElement *elem)
{
	RtLock lock(this->fifo_mutex);

	// insert in head of fifo
	if(this->queue.size() < this->max_size_pkt)
	{
		vol_bytes_t length = elem->getTotalLength();

		this->queue.insert(this->queue.end(), elem);
		this->cur_length_bytes += length;
		// update counter but not new ones as it is a fragment of an old element
		this->stat_context.current_pkt_nbr = this->queue.size();
		this->stat_context.current_length_bytes += length;
		// remove the remainng part of element from out counter
		this->stat_context.out_length_bytes -= length;
		return true;
	}

	return false;

}
MacFifoElement *DvbFifo::pop()
{
	RtLock lock(this->fifo_mutex);
	MacFifoElement *elem;
	vol_bytes_t length;

	if(this->queue.size() <= 0)
	{
		return NULL;
	}

	elem = this->queue.front();
	length = elem->getTotalLength();

	// remove the packet
	this->queue.erase(this->queue.begin());
	this->cur_length_bytes -= length;

	// update counters
	this->stat_context.current_pkt_nbr = this->queue.size();
	this->stat_context.out_pkt_nbr++;

	this->stat_context.current_length_bytes -= length;
	this->stat_context.out_length_bytes += length;

	return elem;
}

void DvbFifo::flush()
{
	RtLock lock(this->fifo_mutex);
	vector<MacFifoElement *>::iterator it;
	for(it = this->queue.begin(); it != this->queue.end(); ++it)
	{
//		NetContainer *elem = (*it)->getElem();
//		delete elem;
		delete *it;
	}

	this->queue.clear();
	this->new_size_pkt = 0;
	this->new_length_bytes = 0;
	this->cur_length_bytes = 0;
	this->resetStats();
}


void DvbFifo::getStatsCxt(mac_fifo_stat_context_t &stat_info)
{
	RtLock lock(this->fifo_mutex);
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





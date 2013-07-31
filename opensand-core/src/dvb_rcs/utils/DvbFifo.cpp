/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#include "DvbFifo.h"

#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>


/**
 * Constructor
 */
DvbFifo::DvbFifo():
	queue(),
	fifo_priority(0),
	fifo_name("default"),
	pvc(0),
	new_size_pkt(0),
	carrier_id(0)
{
	this->resetStats();
}


DvbFifo::DvbFifo(unsigned int fifo_priority, string fifo_name,
                 string cr_type_name, unsigned int pvc,
                 vol_pkt_t max_size_pkt):
	queue(),
	fifo_priority(fifo_priority),
	fifo_name(fifo_name),
	pvc(pvc),
	new_size_pkt(0),
	max_size_pkt(max_size_pkt),
	carrier_id(0)
{
	// fifo_priority is a value (e.g: from 0 to 5) specified in the configuration file 
	// of FIFO queues (dvb_rcs_tal section)
	if(cr_type_name == "RBDC")
	{
		this->cr_type = cr_rbdc;
	}
	else if(cr_type_name == "VBDC")
	{
		this->cr_type = cr_vbdc;
	}
	else if(cr_type_name == "NONE")
	{
		this->cr_type = cr_none;
	}
	else
	{
		UTI_ERROR("unknown CR type of FIFO: %s\n",
		          cr_type_name.c_str());
	}
	this->resetStats();
}

/**
 * Destructor
 */
DvbFifo::~DvbFifo()
{
}

string DvbFifo::getName() const
{
	return this->fifo_name;
}

unsigned int DvbFifo::getPvc() const
{
	return this->pvc;
}

cr_type_t DvbFifo::getCrType() const
{
	return this->cr_type;
}

// FIFO priority for ST
unsigned int DvbFifo::getPriority() const
{
	return this->fifo_priority;

}

// FIFO Carrier ID for SAT and GW
unsigned int DvbFifo::getCarrierId() const
{
	return this->carrier_id;
}

vol_pkt_t DvbFifo::getNewSize() const
{
	return this->new_size_pkt;
}

vol_kb_t DvbFifo::getNewDataLength() const
{
	return this->new_length_kb;
}

void DvbFifo::resetNew(cr_type_t cr_type)
{
	if(this->cr_type == cr_type)
	{
		this->new_size_pkt = 0;
	}
}

vol_pkt_t DvbFifo::getCurrentSize() const
{
	return this->queue.size();
}

vol_pkt_t DvbFifo::getMaxSize() const
{
	return this->max_size_pkt;
}

clock_t DvbFifo::getTickOut() const
{
	if(queue.size() > 0)
	{
		return this->queue.front()->getTickOut();
	}
	return 0;
}


bool DvbFifo::push(MacFifoElement *elem)
{
	// insert in top of fifo
	
	if(this->queue.size() >= this->max_size_pkt)
	{
		return false;
	}

	this->queue.push_back(elem);
	// update counter
	this->new_size_pkt++;
	this->stat_context.current_pkt_nbr = this->queue.size();
	this->stat_context.in_pkt_nbr++;
	if(elem->getType() == 1)
	{
		// TODO accessor in MacFifoElem directly
		vol_kb_t length = elem->getPacket()->getTotalLength();
		this->new_length_kb += length;
		this->stat_context.current_length_kb += length;
		this->stat_context.in_length_kb += length;
	}
	return true;
}

bool DvbFifo::pushFront(MacFifoElement *elem)
{
	assert(elem->getType() == 1);

	// insert in head of fifo
	if(this->queue.size() < this->max_size_pkt)
	{
		vol_kb_t length = elem->getPacket()->getTotalLength();

		this->queue.insert(this->queue.begin(), elem);
		// update counter but not new ones as it is a fragment of an old element
		this->stat_context.current_pkt_nbr = this->queue.size();
		this->stat_context.current_length_kb += length;
		// remove the remainng part of element from out counter
		this->stat_context.out_length_kb -= length;
		return true;
	}

	return false;

}

MacFifoElement *DvbFifo::pop()
{
	MacFifoElement *elem;
	
	if(this->queue.size() <= 0)
	{
		return NULL;
	}

	elem = this->queue.front();

	// remove the packet
	this->queue.erase(this->queue.begin());

	// update counters
	this->stat_context.current_pkt_nbr = this->queue.size();
	this->stat_context.out_pkt_nbr++;
	if(elem->getType() == 1)
	{
		// TODO accessor in MacFifoElem directly
		vol_kb_t length = elem->getPacket()->getTotalLength();
		this->stat_context.current_length_kb -= length;
		this->stat_context.out_length_kb += length;
	}

	return elem;
}

void DvbFifo::flush()
{
	for(std::vector<MacFifoElement *>::iterator it = this->queue.begin();
	    it != this->queue.end(); ++it)
	{
		// TODO destroy element content ?
		delete (*it);
	}
	this->queue.clear();
	this->new_size_pkt = 0;
	this->new_length_kb = 0;
	this->resetStats();
}


void DvbFifo::getStatsCxt(mac_fifo_stat_context_t &stat_info)
{
	stat_info.current_pkt_nbr = this->stat_context.current_pkt_nbr;
	stat_info.current_length_kb = this->stat_context.current_length_kb;
	stat_info.in_pkt_nbr = this->stat_context.in_pkt_nbr;
	stat_info.out_pkt_nbr = this->stat_context.out_pkt_nbr;
	stat_info.in_length_kb = this->stat_context.in_length_kb;
	stat_info.out_length_kb = this->stat_context.out_length_kb;

	// reset counters
	this->resetStats();
}

void DvbFifo::resetStats()
{
	this->stat_context.current_pkt_nbr = 0;
	this->stat_context.current_length_kb = 0;
	this->stat_context.in_pkt_nbr = 0;
	this->stat_context.out_pkt_nbr = 0;
	this->stat_context.in_length_kb = 0;
	this->stat_context.out_length_kb = 0;
}

void DvbFifo::init(unsigned int carrier_id, vol_pkt_t max_size, string fifo_name)
{
	this->carrier_id = carrier_id;
	this->max_size_pkt = max_size;
	this->fifo_name = fifo_name;
}




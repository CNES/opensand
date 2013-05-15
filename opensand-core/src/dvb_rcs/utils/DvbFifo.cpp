/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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


/**
 * Constructor
 */
DvbFifo::DvbFifo():
	queue(),
	new_size_pkt(0)
{
	this->stat_context.current_pkt_nbr = 0;
	this->stat_context.in_pkt_nbr = 0;
	this->stat_context.out_pkt_nbr = 0;
}


/**
 * Constructor
 * @param id           fifo identifier
 * @param mac_priority the MAC priority of fifo
 */
DvbFifo::DvbFifo(unsigned int id, string mac_prio_name,
                 string cr_type_name, unsigned int pvc,
                 vol_pkt_t size):
	queue(),
	pvc(pvc),
	new_size_pkt(0)
{
	if(mac_prio_name == "NM")
	{
		this->mac_priority = fifo_nm;
	}
	else if(mac_prio_name == "EF")
	{
		this->mac_priority = fifo_ef;
	}
	else if(mac_prio_name == "SIG")
	{
		this->mac_priority = fifo_sig;
	}
	else if(mac_prio_name == "AF")
	{
		this->mac_priority = fifo_af;
	}
	else if(mac_prio_name == "BE")
	{
		this->mac_priority = fifo_be;
	}
	else
	{
		UTI_ERROR("unknown kind of fifo: %s\n",
				  mac_prio_name.c_str());
	}

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

	this->init(id, size);
}

/**
 * Destructor
 */
DvbFifo::~DvbFifo()
{
}

//TODO remove ID and use mac_prio only ?

mac_prio_t DvbFifo::getMacPriority() const
{
	return this->mac_priority;
}

unsigned int DvbFifo::getPvc() const
{
	return this->pvc;
}

cr_type_t DvbFifo::getCrType() const
{
	return this->cr_type;
}

unsigned int DvbFifo::getId() const
{
	return this->id;
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

void DvbFifo::init(unsigned int id, vol_pkt_t max_size_pkt)
{
	this->id = id;
	this->max_size_pkt = max_size_pkt;
	this->resetStats();
}

bool DvbFifo::push(MacFifoElement *elem)
{
	// insert in top of fifo
	if(this->queue.size() < this->max_size_pkt)
	{
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

	return false;
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
		delete (*it)->getPacket();
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



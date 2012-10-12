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

#include <unistd.h>

#include "DvbFifo.h"

#define DBG_PACKAGE PKG_DVB_RCS
#include "opensand_conf/uti_debug.h"

#define RED_RIGHT_SHIFT (7)
#define TICKS_INTERVAL  (1*sysconf(_SC_CLK_TCK))
#define NOTICE_INTERVAL (60*sysconf(_SC_CLK_TCK))



/**
 * Constructor
 */
DvbFifo::DvbFifo(): mgl_fifo(),
	filled(0),
	avg_queue_size(0),
	current_size(0),
	previous_tick(0),
	this_tick(0),
	notice_tick(0)
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
	id(id),
	pvc(pvc)
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

	this->init(size);
}

/* TODO remove
DvbFifo::DvbFifo(unsigned int id, mac_prio_t mac_priority):
	id(id),
	mac_priority(mac_priority)
{
}
*/

/**
 * Constructor
 * @param id           fifo identifier
 * @param mac_priority the MAC priority of fifo
 * @param pvc          the PVC of fifo
 */
/*
DvbFifo::DvbFifo(unsigned int id, mac_prio_t mac_priority, unsigned int pvc):
	id(id),
	mac_priority(mac_priority),
	pvc(pvc),
	filled(0),
	avg_queue_size(0),
	current_size(0),
	previous_tick(0),
	this_tick(0),
	notice_tick(0)
{
}*/

/**
 * Destructor
 */
DvbFifo::~DvbFifo()
{
}

//TODO remove ID and use mac_prio only ?

/**
 * Return the mac_priority of the fifo
 * @return the mac_priority of the fifo
 */
mac_prio_t DvbFifo::getMacPriority()
{
	return this->mac_priority;
}

/**
 * Set the mac_priority of the fifo
 * @param mac_priority is the mac_priority of the fifo
 */
/*
void DvbFifo::setMacPriority(mac_prio_t mac_priority)
{
	this->mac_priority = mac_priority;
}
*/

/**
 * Set the PVC associated to the fifo
 * @param pvc is the PVC of the fifo
 */
/*
void DvbFifo::setPvc(unsigned int pvc)
{
	this->pvc = pvc;
}
*/

/**
 * Get the PVC associated to the fifo
 * return the PVC of the fifo
 */
unsigned int DvbFifo::getPvc()
{
	return (this->pvc);
}


/**
 * Set the CR type associated to the fifo
 * @param cr_type is the CR type associated to the fifo
 */
/*
void DvbFifo::setCrType(cr_type_t cr_type)
{
	this->cr_type = cr_type;
}
*/

/**
 * Get the CR type associated to the fifo
 * return the CR type associated to the fifo (int)
 */
cr_type_t DvbFifo::getCrType()
{
	return (this->cr_type);
}

/**
 * get the id of the fifo
 * @return the id of the fifo
 */
unsigned int DvbFifo::getId()
{
	return (this->id);
}

/**
 * set the id of the fifo
 * @param id is the id of the fifo
 */
void DvbFifo::setId(unsigned int id)
{
	this->id = id;
}

/**
 * Set size then call mgl_fifo::init
 *
 * @param i_size is the fifo maximum size
 * @return 0 on success
 */
int DvbFifo::init(vol_pkt_t i_size)
{
	this->size = i_size;
	this->avg_queue_size = 0;
	this->current_size = 0;
	this->previous_tick = times(NULL);
	this->this_tick = times(NULL);
	this->notice_tick = times(NULL);

	return mgl_fifo::init(i_size);
}

/**
 * Add an element at the end of the list
 * -- Increments filled then call  mgl_fifo::append
 *
 * @param ptr_data is the pointer on data buffer
 * @return -1 if insertion failed because fifo is full, or return
 *         fifo current size otherwise
 */
long DvbFifo::append(void *ptr_data)
{
	int ret;
	// insert buffer
	ret = mgl_fifo::append(ptr_data);
	if(ret != -1)
	{
		// update counter
		this->filled++;
		this->current_size++;
		this->stat_context.current_pkt_nbr = this->current_size;
		this->stat_context.in_pkt_nbr++;
	}
	return ret;
}


/**
 * Remove an element at the head of the list
 * -- capacity then call mgl_fifo::remove
 *
 * @return NULL pointer if extraction failed because fifo is empty
 *         pointer on extracted packet otherwise
 */
void *DvbFifo::remove()
{

	//update counters
	this->capacity--;
	this->current_size--;
	this->stat_context.current_pkt_nbr = this->current_size;
	this->stat_context.out_pkt_nbr++;

	// extract the pk
	return (mgl_fifo::remove());
}

/**
 * Set the capacity to emit for this superframe
 * @param capacity is the MAC capacity associated to the fifo - if any
 */
/*void DvbFifo::setCapacity(long capacity)
{
	this->capacity = capacity;
}*/

/**
 * Get the capacity to emit for this superframe
 * @return the MAC capacity associated to the fifo - if any
 */
vol_pkt_t DvbFifo::getCapacity()
{
	return this->capacity;
}

/**
 * Get the number of cells that fed the queue since last call, reset filled
 * @return the number of cells that fed the queue since last call
 */
vol_pkt_t DvbFifo::getFilled()
{
	long ret;
	ret = this->filled;
	this->filled = 0;
	return ret;
}


/**
 * Get the number of cells that fed the queue since last filled exlicity reset
 *  BUT DO NOT RESET filled
 * @return the number of cells that fed the queue since last call
 */
vol_pkt_t DvbFifo::getFilledWithNoReset()
{
	return this->filled;
}


/**
 * Reset filled, only if the FIFO  has the requested CR type
 * @param cr_type is the CR type for which reset must be done
 */
void DvbFifo::resetFilled(cr_type_t cr_type)
{
	if(this->cr_type == cr_type)
	{
		this->filled = 0;
	}
}

/**
 * Informs if we are _soft_ allowed or not to fill the queue
 * @return boolean indicateing if threshold is exceeding or not
 */
bool DvbFifo::allowed()
{
	// TODO reuse OpenSandCore types in Margouilla
	return ((vol_pkt_t)mgl_fifo::getCount() < this->threshold);
}

/**
 * @return the queue maximum size
 */
vol_pkt_t DvbFifo::getMaxSize()
{
	return this->size;
}

/**
 * Flush the dvb fifo and reset counters
 */
void DvbFifo::flush()
{
	long i;
	long i_max = this->current_size;

	for(i = 0; i < i_max; i++)
	{
		remove();
	}
	this->filled = 0;
}


/**
 * Returns statistics of the fifo in a context
 * @ return statistics context
 */
void DvbFifo::getStatsCxt(mac_fifo_stat_context_t &stat_info)
{
	stat_info.current_pkt_nbr = this->stat_context.current_pkt_nbr;
	stat_info.in_pkt_nbr = this->stat_context.in_pkt_nbr;
	stat_info.out_pkt_nbr = this->stat_context.out_pkt_nbr;

	// reset counters
	this->stat_context.out_pkt_nbr = 0;
	this->stat_context.in_pkt_nbr = 0;
}

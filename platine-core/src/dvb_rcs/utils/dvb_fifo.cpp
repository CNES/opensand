/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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

/*
 * @file dvb_fifo.cpp
 * @brief  FIFO queue containing MAC packets
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <unistd.h>

#include "dvb_fifo.h"

#define DBG_PACKAGE PKG_DVB_RCS
#include "platine_conf/uti_debug.h"
#define DBG_PREFIX "[dvb_fifo]"

#define RED_RIGHT_SHIFT (7)
#define TICKS_INTERVAL  (1*sysconf(_SC_CLK_TCK))
#define NOTICE_INTERVAL (60*sysconf(_SC_CLK_TCK))


/**
 * Constructor
 */
dvb_fifo::dvb_fifo(): mgl_fifo()
{
	m_kind = -1;
	m_pvc = -1;
	m_crType = -1;
	m_capacity = -1;
	m_filled = 0;
	m_size = -1;
	m_threshold = -1;
	avg_queue_size = 0;
	current_size = 0;
	previous_tick = 0;
	this_tick = 0;
	notice_tick = 0;
	m_statContext.currentPkNb = 0;
	m_statContext.inPkNb = 0;
	m_statContext.outPkNb = 0;
}


/**
 * Constructor
 * @param id fifo identifier
 * @param kind the kind of fifo (DVB_FIFO_NRT, DVB_FIFO_RT etc...)
 */
dvb_fifo::dvb_fifo(int id, int kind)
{
	m_kind = kind;
	m_id = id;
}

/**
 * Constructor
 * @param id :  fifo identifier
 * @param kind : the kind of fifo (DVB_FIFO_NRT, DVB_FIFO_RT etc...)
 * @param pvc : the PVC of fifo
 */
dvb_fifo::dvb_fifo(int id, int kind, int pvc)
{
	m_kind = kind;
	m_id = id;
	m_pvc = pvc;
	m_capacity = -1;
	m_filled = 0;
	m_size = -1;
	m_threshold = -1;
	avg_queue_size = 0;
	current_size = 0;
	previous_tick = 0;
	this_tick = 0;
	notice_tick = 0;
}

/**
 * Destructor
 */
dvb_fifo::~dvb_fifo()
{
}

/**
 * Return the kind of the fifo
 * @return the kind of the fifo DVB_FIFO_NRT, DVB_FIFO_RT
 */
int dvb_fifo::getKind()
{
	return (m_kind);
}

/**
 * Set the kind of the fifo
 * @param kind is the kind of the fifo DVB_FIFO_NRT, DVB_FIFO_RT
 */
void dvb_fifo::setKind(int kind)
{
	m_kind = kind;
}

/**
 * Set the PVC associated to the fifo
 * @param pvc is the PVC of the fifo (int)
 */
void dvb_fifo::setPvc(int pvc)
{
	m_pvc = pvc;
}

/**
 * Get the PVC associated to the fifo
 * return the PVC of the fifo (int)
 */
int dvb_fifo::getPvc()
{
	return (m_pvc);
}


/**
 * Set the CR type associated to the fifo
 * @param crType is the CR type associated to the fifo (int)
 */
void dvb_fifo::setCrType(int crType)
{
	m_crType = crType;
}

/**
 * Get the CR type associated to the fifo
 * return the CR type associated to the fifo (int)
 */
int dvb_fifo::getCrType()
{
	return (m_crType);
}

/**
 * get the id of the fifo
 * @return the id of the fifo
 */
int dvb_fifo::getId()
{
	return (m_id);
}

/**
 * set the id of the fifo
 * @param id is the id of the fifo
 */
void dvb_fifo::setId(int id)
{
	m_id = id;
}

/**
 * Set m_size then call mgl_fifo::init
 * @param i_size is the fifo maximum size
 */
int dvb_fifo::init(long i_size)
{
	m_size = i_size;
	avg_queue_size = 0;
	current_size = 0;
	previous_tick = times(NULL);
	this_tick = times(NULL);
	notice_tick = times(NULL);

	return mgl_fifo::init(i_size);
}

/**
 * Add an element at the end of the list
 * -- Increments m_filled then call  mgl_fifo::append
 *
 * @param ptr_data is the pointer on data buffer
 * @return -1 if insertion failed because fifo is full, or return
 *         fifo current size otherwise
 */
long dvb_fifo::append(void *ptr_data)
{
	int ret;
	// insert buffer
	ret = mgl_fifo::append(ptr_data);
	if(ret != -1)
	{
		// update counter
		m_filled++;
		current_size++;
		m_statContext.currentPkNb = current_size;
		m_statContext.inPkNb++;
	}
	return ret;
}


/**
 * Remove an element at the head of the list
 * -- m_capacity then call mgl_fifo::remove
 *
 * @return NULL pointer if extraction failed because fifo is empty
 *         pointer on extracted packet otherwise
 */
void *dvb_fifo::remove()
{

	//update counters
	m_capacity--;
	current_size--;
	m_statContext.currentPkNb = current_size;
	m_statContext.outPkNb++;

	// extract the pk
	return (mgl_fifo::remove());
}

/**
 * Set the capacity to emit for this superframe
 * @param capacity is the MAC capacity associated to the fifo - if any
 */
void dvb_fifo::setCapacity(long capacity)
{
	m_capacity = capacity;
}

/**
 * Get the capacity to emit for this superframe
 * @return the MAC capacity associated to the fifo - if any
 */
long dvb_fifo::getCapacity()
{
	return m_capacity;
}

/**
 * Get the number of cells that fed the queue since last call, reset m_filled
 * @return the number of cells that fed the queue since last call
 */
long dvb_fifo::getFilled()
{
	long ret;
	ret = m_filled;
	m_filled = 0;
	return ret;
}


/**
 * Get the number of cells that fed the queue since last m_filled exlicity reset
 *  BUT DO NOT RESET m_filled
 * @return the number of cells that fed the queue since last call
 */
long dvb_fifo::getFilledWithNoReset()
{
	return m_filled;
}


/**
 * Reset m_filled, only if the FIFO  has the requested CR type
 * @param crType is the CR type for which reset must be done
 */
void dvb_fifo::resetFilled(int crType)
{
	if(m_crType == crType)
	{
		m_filled = 0;
	}
}

/**
 * Informs if we are _soft_ allowed or not to fill the queue
 * @return boolean indicateing if threshold is exceeding or not
 */
bool dvb_fifo::allowed()
{
	return (mgl_fifo::getCount() < m_threshold);
}

/**
 * Set the computed threshold
 * @param thr is the threshold
 */
void dvb_fifo::setThreshold(long thr)
{
	m_threshold = thr;
}

/**
 * @return the queue maximum size
 */
long dvb_fifo::getMaxSize()
{
	return m_size;
}

/**
 * Flush the dvb fifo and reset counters
 */
int dvb_fifo::flush()
{
	long i;
	long i_max = current_size;

	for(i = 0; i < i_max; i++)
	{
		remove();
	}
	m_filled = 0;
	return 0;
}


/**
 * Returns statistics of the fifo in a context
 * @ return statistics context
 */
void dvb_fifo::getStatsCxt(MacFifoStatContext & statInfo)
{
	statInfo.currentPkNb = m_statContext.currentPkNb;
	statInfo.inPkNb = m_statContext.inPkNb;
	statInfo.outPkNb = m_statContext.outPkNb;

	// reset counters
	m_statContext.outPkNb = 0;
	m_statContext.inPkNb = 0;
}

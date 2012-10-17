/*
 *
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

/**
 * @file DvbFifo.h
 * @brief FIFO queue containing MAC packets
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef DVD_FIFO_H
#define DVD_FIFO_H

#include "OpenSandCore.h"
#include "MacFifoElement.h"

//#include <linux/param.h>
#include <vector>
#include <sys/times.h>


///> The priority of FIFO that indicates the MAC QoS which is sometimes equivalent
///>  to Diffserv IP QoS
//TODO use the Diffserv values as in configuration ?
typedef enum
{
	fifo_nm = 0, /* Network Management */
	fifo_ef = 1, /* Expedited Forwarding */
	fifo_sig = 2, /* Signalisation */
	fifo_af = 3, /* Assured Forwarding */
	fifo_be = 4, /* Best Effort */
} mac_prio_t;

// TODO move in CapacityRequest.h
///> The type of capacity request associated to each FIFO among RBDC, VBDC or None
typedef enum
{
	cr_none = 0, /* No CR, only use Constant Allocation */
	cr_rbdc = 1, /* Rate Based */
	cr_vbdc = 2, /* Volume Based */
} cr_type_t;



/// DVB fifo statistics context
typedef struct
{
	vol_pkt_t current_pkt_nbr;  // current number of elements
	vol_kb_t current_length_kb; // current length of data in fifo
	vol_pkt_t in_pkt_nbr;       // number of elements inserted during period
	vol_pkt_t out_pkt_nbr;      // number of elements extracted during period
	vol_kb_t in_length_kb;      // current length of data inserted during period
	vol_kb_t out_length_kb;     // current length of data extraction during period
} mac_fifo_stat_context_t;


/**
 * @class DvbFifo
 * @brief Defines a DVB fifo
 *
 * Manages a DVB fifo, for queuing, statistics, ...
 */
class DvbFifo
{
 public:

	DvbFifo();

	/**
	 * @brief Create the DvbFifo
	 *
	 * @param id            the fifo id
	 * @param mac_prio_name the MAC priority name for this fifo
	 * @param cr_type_name  the CR type name for this fifo
	 * @param pvc           the PVC associated to this fifo
	 * @param max_size_pkt  the fifo maximum size
	 */
	DvbFifo(unsigned int id, string mac_prio_name,
	        string cr_type_name, unsigned int pvc,
	        vol_pkt_t max_size_pkt);
	virtual ~DvbFifo();

	/**
	 * @brief Get the mac_priority of the fifo
	 *
	 * @return the mac_priority of the fifo
	 */
	mac_prio_t getMacPriority() const;

	/**
	 * @brief Get the PVC associated to the fifo
	 *
	 * return the PVC of the fifo
	 */
	unsigned int getPvc() const;

	/**
	 * @brief Get the CR type associated to the fifo
	 *
	 * return the CR type associated to the fifo
	 */
	cr_type_t getCrType() const;

	/**
	 * @brief Get the id of the fifo
	 *
	 * @return the id of the fifo
	 */
	unsigned int getId() const;

	/**
	 * @brief Get the fifo current size
	 *
	 * @return the queue current size
	 */
	vol_pkt_t getCurrentSize() const;

	/**
	 * @brief Get the fifo maximum size
	 *
	 * @return the queue maximum size
	 */
	vol_pkt_t getMaxSize() const;

	/**
	 * @brief Get the number of packets or cells that fed the queue since
	 *        last reset
	 *
	 * @return the number of packets/cells that fed the queue since last call
	 */
	vol_pkt_t getNewSize() const;

	/**
	 * @brief Get the length of data in the fifo (in kbits)
	 *
	 * @return the size of data in the fifo (in kbits)
	 */
	vol_kb_t getNewDataLength() const;

	/**
	 * @brief Get the head element tick out
	 *
	 * @return the head element tick out
	 */
	clock_t getTickOut() const;

	/**
	 * @brief Reset filled, only if the FIFO has the requested CR type
	 *
	 * @param cr_type is the CR type for which reset must be done
	 */
	void resetNew(const cr_type_t cr_type);

	/**
	 * @brief Initialize the FIFO
	 *
	 * @param id            the fifo id
	 * @param max_size_pkt  the fifo maximum size
	 */
	void init(unsigned int id, vol_pkt_t max_size_pkt);

	/**
	 * @brief Add an element at the end of the list
	 *        (Increments new_size_pkt)
	 *
	 * @param elem is the pointer on MacFifoElement
	 * @return true on success, false otherwise
	 */
	bool push(MacFifoElement *elem);

	/**
	 * @brief Add an element at the head of the list
	 *        (Decrements new_length_kb)
	 * @warning This function should be use only to replace a fragment of
	 *          previously removed data in the fifo
	 *
	 * @param elem is the pointer on MacFifoElement
	 * @return true on success, false otherwise
	 */
	bool pushFront(MacFifoElement *elem);

	/**
	 * @brief Remove an element at the head of the list
	 *
	 * @return NULL pointer if extraction failed because fifo is empty
	 *         pointer on extracted MacFifoElement otherwise
	 */
	MacFifoElement *pop();

	/**
	 * @brief Flush the dvb fifo and reset counters
	 */
	void flush();

	/**
	 * @brief Returns statistics of the fifo in a context
	 *        and reset counters
	 *
	 * @return statistics context
	 */
	void getStatsCxt(mac_fifo_stat_context_t &stat_info);

 protected:

	/**
	 * @brief Reset the fifo counters
	 */
	void resetStats();

	std::vector<MacFifoElement *> queue; ///< the FIFO itself

	unsigned int id;     ///< the fifo ID
	mac_prio_t mac_priority;  ///< the QoS MAC priority of the fifo: EF, AF, BE, ...
	unsigned int pvc;    ///< the MAC PVC (Pemanent Virtual Channel)
	                     ///< associated to the FIFO
	                     ///< No used in starred or mono-spot
	                     ///< In meshed satellite, a PVC should be associated to a
	                     ///< spot and allocation would depend of it as it depends
	                     ///< of spot
	cr_type_t cr_type;   ///< the associated Capacity Request
	vol_pkt_t new_size_pkt;  ///< the number of cells or packets that filled the fifo
	                         ///< since previous check
	vol_pkt_t new_length_kb; ///< the size of data that filled the fifo
	                         ///< since previous check
	vol_pkt_t max_size_pkt;  ///< the maximum size for that FIFO
	mac_fifo_stat_context_t stat_context; ///< statistics context used by MAC layer
};

#endif

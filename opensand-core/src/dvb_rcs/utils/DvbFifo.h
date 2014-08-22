/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @author Julien Bernard / Viveris Technologies
 */

#ifndef DVD_FIFO_H
#define DVD_FIFO_H

#include "OpenSandCore.h"
#include "MacFifoElement.h"
#include "Sac.h"

#include <opensand_rt/RtMutex.h>
#include <opensand_output/OutputLog.h>

#include <vector>
#include <map>
#include <sys/times.h>

using std::vector;
using std::map;

///> The priority of FIFO that indicates the MAC QoS which is sometimes equivalent
///>  to Diffserv IP QoS

/// DVB fifo statistics context
typedef struct
{
	vol_pkt_t current_pkt_nbr;        ///< current number of elements
	vol_bytes_t current_length_bytes; ///< current length of data in fifo
	vol_pkt_t in_pkt_nbr;             ///< number of elements inserted during period
	vol_pkt_t out_pkt_nbr;            ///< number of elements extracted during period
	vol_bytes_t in_length_bytes;      ///< current length of data inserted during period
	vol_bytes_t out_length_bytes;     ///< current length of data extraction during period
	vol_pkt_t drop_pkt_nbr;           ///< number of elements dropped
	vol_bytes_t drop_bytes;           ///< current length of data dropped
} mac_fifo_stat_context_t;

/// Access type for fifo (mapping between mac_fifo and carrier)
typedef enum
{
	access_vcm0 = 0,
	access_vcm1 = 1,
	access_vcm2 = 2,
	access_vcm3 = 3,
	access_acm = 4,
} fwd_access_type_t;


/**
 * @class DvbFifo
 * @brief Defines a DVB fifo
 *
 * Manages a DVB fifo, for queuing, statistics, ...
 */
class DvbFifo
{
 public:

	/**
	 * @brief Create the DvbFifo
	 *
	 * @param fifo_priority the fifo priority
	 * @param fifo_name     the name of the fifo queue (NM, EF, ...) or SAT
	 * @param type_name     the CR type name for this fifo if it is for a ST 
	 *                      (return link, the carrier access type name for
	 *                      this fifo if it is for the GW (forward link)
	 * @param max_size_pkt  the fifo maximum size
	 */
	DvbFifo(unsigned int fifo_priority, string mac_fifo_name,
	        string type_name,
	        vol_pkt_t max_size_pkt);

	/**
	 * @brief Create the Spot DvbFifo
	 *
	 * @param carrier_id    the carrier id for the fifo
	 * @param max_size_pkt  the fifo maximul size
	 * @param fifo_name     the name of the fifo
	 */
	DvbFifo(uint8_t carrier_id,
	        vol_pkt_t max_size_pkt,
	        string fifo_name);


	virtual ~DvbFifo();

	/**
	 * @brief Get the fifo_name of the fifo
	 *
	 * @return the fifo_name of the fifo
	 */
	string getName() const;

	/**
	 * @brief Get the CR type associated to the fifo
	 *
	 * return the CR type associated to the fifo
	 */
	cr_type_t getCrType() const;

	/**
	 * @brief Get the access type associated to the fifo
	 *
	 * return the access type associated to the fifo
	 */
	fwd_access_type_t getAccessType() const;

	/**
	 * @brief Get the fifo_priority of the fifo (value from ST FIFO configuration)
	 *
	 * @return the fifo_priority of the fifo
	 */
	unsigned int getPriority() const;

	/** 
	* @brief Get the carrier_id of the fifo (for SAT and GW configuration)
	*
	* @return the carrier_id of the fifo
	*/
	uint8_t getCarrierId() const;

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
	 * @brief Get the number of packets that fed the queue since
	 *        last reset
	 *
	 * @return the number of packets that fed the queue since last call
	 */
	vol_pkt_t getNewSize() const;

	/**
	 * @brief Get the length of data in the fifo (in kbits)
	 *
	 * @return the size of data in the fifo (in kbits)
	 */
	vol_bytes_t getNewDataLength() const;

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
	 * @brief Add an element at the end of the list
	 *        (Increments new_size_pkt)
	 *
	 * @param elem is the pointer on MacFifoElement
	 * @return true on success, false otherwise
	 */
	bool push(MacFifoElement *elem);

	/**
	 * @brief Add an element at the head of the list
	 *        (Decrements new_length_bytes)
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

	vector<MacFifoElement *> queue; ///< the FIFO itself

	unsigned int fifo_priority;     ///< the MAC priority of the fifo
	string fifo_name;               ///< the MAC fifo name: for ST (EF, AF, BE, ...) or SAT
	cr_type_t cr_type;              ///< the associated Capacity Request
	fwd_access_type_t access_type;  ///< the associated Access Type
	vol_pkt_t new_size_pkt;         ///< the number of packets that filled the fifo
	                                ///< since previous check
	vol_bytes_t new_length_bytes;   ///< the size of data that filled the fifo
	                                ///< since previous check
	vol_pkt_t max_size_pkt;         ///< the maximum size for that FIFO
	uint8_t carrier_id;             ///< the carrier id of the fifo (for SAT and GW purposes)
	mac_fifo_stat_context_t stat_context; ///< statistics context used by MAC layer

	mutable RtMutex fifo_mutex; ///< The mutex to protect FIFO from concurrent access

	// Output log
	OutputLog *log_dvb_fifo;
};

typedef map<qos_t, DvbFifo *> fifos_t;

#endif

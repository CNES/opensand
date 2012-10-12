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

#include "opensand_margouilla/mgl_fifo.h"
#include "OpenSandCore.h"

#include <linux/param.h>
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
	vol_pkt_t current_pkt_nbr; // current pk number
	vol_pkt_t in_pkt_nbr;      // number of pk inserted during period
	vol_pkt_t out_pkt_nbr;     // number of pk extracted during period
} mac_fifo_stat_context_t;


/**
 * @class DvbFifo
 * @brief Defines a DVB fifo
 *
 * Manages a DVB fifo, for queuing, statistics, ...
 */
class DvbFifo: public mgl_fifo
{
 public:

	DvbFifo();
	DvbFifo(unsigned int id, string mac_prio_name,
	        string cr_type_name, unsigned int pvc,
	        vol_pkt_t size);
	virtual ~DvbFifo();

	mac_prio_t getMacPriority();

	unsigned int getPvc();

	cr_type_t getCrType();

	unsigned int getId();
	void setId(unsigned int id);

	vol_pkt_t getFilled();
	vol_pkt_t getFilledWithNoReset();
	void resetFilled(cr_type_t cr_type);

	void setCapacity(vol_pkt_t capacity);
	vol_pkt_t getCapacity();

	bool allowed();
	vol_pkt_t getMaxSize();

	int init(vol_pkt_t i_size);
	long append(void *ptr_data);
	void *remove();
	void flush();

	void getStatsCxt(mac_fifo_stat_context_t &stat_info);

 protected:

	unsigned int id;     ///< the fifo ID
	mac_prio_t mac_priority;  ///< the QoS MAC priority of the fifo: EF, AF, BE, ...
	unsigned int pvc;    ///< the MAC PVC (Pemanent Virtual Channel)
	                     ///< associated to the FIFO
	                     ///< No used in starred or mono-spot
	                     ///< In meshed satellite, a PVC should be associated to a
	                     ///< spot and allocation would depend of it as it depends
	                     ///< of spot
	cr_type_t cr_type;   ///< the associated Capacity Request
	vol_pkt_t filled;    ///< the number of cells or packets that filled the fifo
	                     ///< since previous check
	vol_pkt_t capacity;  ///< the allocated number of cells for the current
	                     ///< superframe
	vol_pkt_t size;      ///< the maximum size for that FIFO
	vol_pkt_t threshold; ///< the computed threshold that should block upper
	                     ///< transmission
	mac_fifo_stat_context_t stat_context; ///< statistics context used by MAC layer

	// RED like statistics collect base on occupation rate
	// it is computed like this:
	//    avg_queue_size = (1 - alpha) * avg_queue_size
	//                      +   (alpha) * current_size
	// with alpha being usual CISCO RED parameter
	// alpha = 2^{-x} we have
	// If we were n times idle between two interval of time we precomute n times
	//    avg_queue_size -= avg_queue_size >> x
	// then 1 times
	//    avg_queue_size += current_size >> x
	//
	vol_pkt_t avg_queue_size; // NB: avg_queue_size is computed but not used yet
	vol_pkt_t current_size;
	// TODO check clock_t
	clock_t previous_tick;
	clock_t this_tick;
	clock_t notice_tick;
};

#endif

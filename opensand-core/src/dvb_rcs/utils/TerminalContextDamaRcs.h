/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file TerminalContextDamaRcs.h
 * @brief The terminal allocations
 * @author Julien Bernard / Viveris Technologies
 *
 */

#ifndef _TERMINAL_CONTEXT_DAMA_RCS_H_
#define _TERMINAL_CONTEXT_DAMA_RCS_H_

#include "TerminalContextDama.h"

#include "OpenSandCore.h"
#include "UnitConverter.h"

/**
 * @class TerminalContextDamaRcs
 */
class TerminalContextDamaRcs : public TerminalContextDama
{
 public:

	TerminalContextDamaRcs(tal_id_t tal_id,
	                       rate_kbps_t cra_kbps,
	                       rate_kbps_t max_rbdc_kbps,
	                       time_sf_t rbdc_timeout_sf,
	                       vol_kb_t min_vbdc_kb,
	                       const UnitConverter *converter);

	virtual ~TerminalContextDamaRcs();

	/**
	 * @brief  Update the CRA value.
	 *         (update the kbps and pktpf values)
	 *
	 * @param cra_kbps  The CRA value (kbits/s)
	 */
	void setCra(rate_kbps_t cra_kbps);

	/**
	 * @brief  Get the CRA value.
	 *
	 * @return cra_kbps  The CRA value (kbits/s)
	 */
	rate_kbps_t getCra();

	/**
	 * @brief  Update the RBDC max value.
	 *         (update the kbps and pktpf values)
	 *
	 * @param max_rbdc_kbps  The RBDC max value (kbits/s)
	 */
	void setMaxRbdc(rate_kbps_t max_rbdc_kbps);

	/**
	 * @brief  Get the RBDC max value.
	 *
	 * @return max_rbdc_kbps  The RBDC max value (kbits/s)
	 */
	rate_kbps_t getMaxRbdc();

	/**
	 * @brief  Update the RBDC timeout value.
	 *
	 * @param rbdc_timeout_sf  The timeout value (supeframe number)
	 */
	void setRbdcTimeout(time_sf_t rbdc_timeout_sf);

	/**
	 * @brief  Set the RBDC request value.
	 *         The timer, timeout credit and initial request are initialised
	 *
	 * @param  rbdc_request_pktpf  The capacity request
	 */
	void setRequiredRbdc(rate_pktpf_t rbdc_request_pktpf);

	/**
	 * @brief Get the ST RBDC request
	 *
	 * @return The RBDC request value (packets per superframe).
	 */
	rate_pktpf_t getRequiredRbdc() const;

	/**
	 * @brief  Set the RBDC allocation after DAMA computation.
	 *         The unit should be the unit for Ttp assignment_count
	 *
	 * @param  rbdc_alloc  The RBDC allocation
	 */
	void setRbdcAllocation(uint16_t alloc);

	/**
	 * @brief  Add a credit to the request credit.
	 *
	 * @param  credit_pktpf  the credit to add
	 */
	void addRbdcCredit(rate_pktpf_t credit_pktpf);

	/**
	 * @brief  Get the currecnt RBDC credit
	 *
	 * @return  the RBDC credit in packets per superframe
	 */
	rate_pktpf_t getRbdcCredit();

	/**
	 * @brief  Set the VBDC request value.
	 *         The VBDC request are cumulated
	 *
	 * @param  vbdc_request_pktpf  The capacity request
	 */
	void setRequiredVbdc(vol_pkt_t vbdc_request_pkt);

	/**
	 * @brief  Set the VBDC allocation after DAMA computation.
	 *         The unit should be the unit for Ttp assignment_count
	 *
	 * @param  vbdc_alloc        The VBDC allocation (packets)
	 * @param  allocation cycle  The number of frames per superframes
	 */
	void setVbdcAllocation(vol_pkt_t vbdc_alloc_pkt,
	                       unsigned int allocation_cycle);

	/**
	 * @brief Get the ST VBDC request
	 *
	 * @param  allocation cycle  The number of frames per superframes
	 * @return The VBDC request value (packets).
	 */
	vol_pkt_t getRequiredVbdc(unsigned int allocation_cycle) const;

	/**
	 * @brief Set the FCA allocation after DAMA computation
	 *
	 * @param fca_alloc_pktpf  The FCA allocation (packets per superframe).
	 */
	void setFcaAllocation(rate_pktpf_t fca_alloc_pktpf);

	/**
	 * @brief Get the FCA allocation after DAMA computation
	 *
	 * @return fca_alloc_pktpf  The FCA allocation (packets per superframe).
	 */
	rate_pktpf_t getFcaAllocation();

	/**
	 * @brief Get the total rate allocation
	 *
	 * @return the total rate allocation (in packets per superframe)
	 */
	rate_pktpf_t getTotalRateAllocation();

	/**
	 * @brief Get the total volume allocation
	 *
	 * @return the total volume allocation (in kb)
	 */
	vol_pkt_t getTotalVolumeAllocation();

	/**
	 * @brief Functor to sort terminals by descending remaining credit
	 *
	 * @param e1  first terminal
	 * @param e2  second terminal
	 * @return true if remaining credit of e1 is greater than e2
	 */
	static bool sortByRemainingCredit(const TerminalContextDamaRcs *e1,
	                                  const TerminalContextDamaRcs *e2);

	/**
	 * @brief Functor to sort terminals by descending VBDC Request
	 *
	 * @param e1  first terminal
	 * @param e2  second terminal
	 * @return true if VBDC request of e1 is greater than e2
	 */
	static bool sortByVbdcReq(const TerminalContextDamaRcs *e1,
	                          const TerminalContextDamaRcs *e2);

	// inherited from TerminalContextDama
	void onStartOfFrame();


 protected:

	/** the RBDC credit: the decimal part of RBDC that may remain
	 *  after DAMA computation */
	double rbdc_credit_pktpf;

	/** The timer for RBDC requests: initialized to rbdc_timeout_sf each request
	 *  and decreased on each SOF */
	time_sf_t timer_sf;

	/** the RBDC request */
	rate_pktpf_t rbdc_request_pktpf;

	/** The RBDC allocation */
	rate_pktpf_t rbdc_alloc_pktpf;

	/** the VBDC request */
	vol_pkt_t vbdc_request_pkt;

	/** The VBDC allocation */
	vol_pkt_t vbdc_alloc_pkt;

	/** The FCA allocation */
	rate_pktpf_t fca_alloc_pktpf;

	/** CRA for the terminal converted to used unit (paquets per superframe) */
	rate_kbps_t cra_pktpf;

	/** Maximum RBDC value converted to used unit (paquets per superframe) */
	rate_kbps_t max_rbdc_pktpf;

	/** The maximum VBDC value converted to used unit (paquets) */
	vol_pkt_t max_vbdc_pkt;

	/** The unit converter */
	const UnitConverter *converter;

};

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file TerminalContextDama.h
 * @brief The termial context
 * @author Julien Bernard / Viveris Technologies
 *
 */


#ifndef _TERMINAL_CONTEXT_DAMA_H_
#define _TERMINAL_CONTEXT_DAMA_H_

#include "TerminalContext.h"
#include "UnitConverter.h"
#include "FmtDefinition.h"

/**
 * @class TerminalContextDama
 * @brief Interface for a terminal context to be used in a DAMA controller.
 *        The requests values and handling MUST be treated in this context but
 *        they SHOULD be implemented in derived classes as these highly depends
 *        on access type
 */
class TerminalContextDama: public TerminalContext
{
 public:

	/**
	 * @brief  Create a terminal context for DAMA
	 *
	 * @param  tal_id           terminal id.
	 * @param  cra_kbps         terminal CRA (kb/s).
	 * @param  max_rbdc_kbps    maximum RBDC value (kb/s).
	 * @param  rbdc_timeout_sf  RBDC timeout (in superframe number).
	 * @param  max_vbdc_kb      maximum VBDC value (kb).
	 */
	TerminalContextDama(tal_id_t tal_id,
	                    rate_kbps_t cra_kbps,
	                    rate_kbps_t max_rbdc_kbps,
	                    time_sf_t rbdc_timeout_sf,
	                    vol_kb_t max_vbdc_kb);
	virtual ~TerminalContextDama();

	/**
	 * @brief  Update the RBDC timeout value.
	 *
	 * @param rbdc_timeout_sf  The timeout value (supeframe number)
	 */
	void updateRbdcTimeout(time_sf_t rbdc_timeout_sf);

	/** @brief  Set the terminal CRA
	 *
	 * @param  The new CRA value (kb/s).
	 */
	void setCra(rate_kbps_t cra_kbps);

	/**
	 * @brief   Get the terminal CRA.
	 *
	 * @return  CRA of terminal (kb/s).
	 */
	rate_kbps_t getCra() const;

	/** @brief  Set the terminal max RBDC value
	 *
	 * @param  max_rbdc_kbps The new max RBDC value (kb/s)
	 */
	void setMaxRbdc(rate_kbps_t max_rbdc_kbps);

	/**
	 * @brief   Get the terminal max RBDC value.
	 *
	 * @return  max RBDC value of terminal of terminal (kb/s).
	 */
	rate_kbps_t getMaxRbdc() const;

	/**
	 * @brief   Get the terminal max VBDC value.
	 *
	 * @return  max VBDC value of terminal of terminal (kb).
	 */
	vol_kb_t getMaxVbdc() const;

	/**
	 * @brief  Set the RBDC request value.
	 *         The timer, timeout credit and initial request are initialised
	 *
	 * @param  rbdc_request_kbps  The capacity request
	 */
	void setRequiredRbdc(rate_kbps_t rbdc_request_kbps);

	/**
	 * @brief Get the ST RBDC request
	 *
	 * @return The RBDC request value (kbps).
	 */
	rate_kbps_t getRequiredRbdc() const;

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
	 * @param  credit_kbps  the credit in kb/s to add
	 */
	void addRbdcCredit(double credit_kbps);

	/**
	 * @brief  Get the current RBDC credit
	 *
	 * @return  the RBDC credit in kb/s
	 */
	double getRbdcCredit() const;

	/**
	 * @brief  Set a credit to the request credit
	 *
	 * @param  credit_kbps  the new credit in kb/s
	 */
	void setRbdcCredit(double credit_kbps);

	/**
	 * @brief  Get the timer
	 *
	 * @return  the timer
	 */
	time_sf_t getTimer() const;

	/**
	 * @brief  Decrement the timer
	 */
	void decrementTimer();

	/**
	 * @brief  Set the VBDC request value.
	 *         The VBDC request are cumulated
	 *
	 * @param  vbdc_request_kb  The capacity request
	 */
	void setRequiredVbdc(vol_kb_t vbdc_request_kb);

	/**
	 * @brief  Set the VBDC allocation after DAMA computation.
	 *         The unit should be the unit for Ttp assignment_count
	 *
	 * @param  vbdc_alloc        The VBDC allocation (kb)
	 */
	void setVbdcAllocation(vol_kb_t vbdc_alloc_kb);

	/**
	 * @brief Get the ST VBDC request
	 *
	 * @return The VBDC request value (kb).
	 */
	vol_kb_t getRequiredVbdc() const;
	
	/**
	 * @brief Set the FCA allocation after DAMA computation
	 *
	 * @param fca_alloc_kbps  The FCA allocation (kb/s).
	 */
	void setFcaAllocation(rate_kbps_t fca_alloc_kbps);

	/**
	 * @brief Get the FCA allocation after DAMA computation
	 *
	 * @return fca_alloc_kbps  The FCA allocation (kb/s).
	 */
	rate_kbps_t getFcaAllocation() const;

	/**
	 * @brief Get the total rate allocation
	 *
	 * @return the total rate allocation (kb/s)
	 */
	rate_kbps_t getTotalRateAllocation() const;

	/**
	 * @brief Get the total volume allocation
	 *
	 * @return the total volume allocation (kb)
	 */
	vol_kb_t getTotalVolumeAllocation() const;

	/**
	 * @brief Functor to sort terminals by descending remaining credit
	 *
	 * @param e1  first terminal
	 * @param e2  second terminal
	 * @return true if remaining credit of e1 is greater than e2
	 */
	static bool sortByRemainingCredit(const TerminalContextDama *e1,
	                                  const TerminalContextDama *e2);

	/**
	 * @brief Functor to sort terminals by descending VBDC Request
	 *
	 * @param e1  first terminal
	 * @param e2  second terminal
	 * @return true if VBDC request of e1 is greater than e2
	 */
	static bool sortByVbdcReq(const TerminalContextDama *e1,
	                          const TerminalContextDama *e2);

  protected:

	/** CRA for the terminal (kb/s) */
	rate_kbps_t cra_kbps;

	/** Maximual RBDC value (kb/s) */
	rate_kbps_t max_rbdc_kbps;

	/** RBDC request timeout */
	time_sf_t rbdc_timeout_sf;

	/** The maximum VBDC value */
	vol_kb_t max_vbdc_kb;

	/** the RBDC credit: the decimal part of RBDC that may remain
	 *  after DAMA computation */
	double rbdc_credit_kbps;

	/** The timer for RBDC requests: initialized to rbdc_timeout_sf each request
	 *  and decreased on each SOF */
	time_sf_t timer_sf;

	/** the RBDC request */
	rate_kbps_t rbdc_request_kbps;

	/** The RBDC allocation */
	rate_kbps_t rbdc_alloc_kbps;

	/** the VBDC request */
	vol_kb_t vbdc_request_kb;

	/** The VBDC allocation */
	vol_kb_t vbdc_alloc_kb;

	/** The FCA allocation */
	rate_kbps_t fca_alloc_kbps;
};

#endif

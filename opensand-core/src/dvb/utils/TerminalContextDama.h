/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
	 * @param  tal_id          terminal id.
	 * @param  cra_kbps        terminal CRA (kb/s).
	 * @param  max_rbdc_kbps   maximum RBDC value (kb/s).
	 * @param  rbdc_timeout_sf RBDC timeout (in superframe number).
	 * @param  max_vbdc_kb     maximum VBDC value (kb).
	 */
	TerminalContextDama(tal_id_t tal_id,
	                    rate_kbps_t cra_kbps,
	                    rate_kbps_t max_rbdc_kbps,
	                    time_sf_t rbdc_timeout_sf,
	                    vol_kb_t max_vbdc_kb);
	virtual ~TerminalContextDama();

	/** @brief  Set the terminal CRA
	 *
	 * @param  The new CRA value (kb/s).
	 */
	virtual void setCra(rate_kbps_t cra_kbps);

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
	virtual void setMaxRbdc(rate_kbps_t max_rbdc_kbps);

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
	 * @brief  Called on SoF emission.
	 *
	 * @return  true on succes, false otherwise.
	 */
	virtual void onStartOfFrame() = 0;

	/**
	 * @brief Get the current FMT ID of the forward link of the terminal
	 *
	 * @return the ID of FMT
	 */
	unsigned int getForwardFmtId();

	/**
	 * @brief Set the current FMT ID of the forward link of the terminal
	 *
	 * @param fmt_id  The current FMT ID of the terminal
	 */
	void setForwardFmtId(unsigned int fmt_id);

	/**
	 * @brief Get the current FMT ID of the return link of the terminal
	 *
	 * @return the ID of FMT
	 */
	unsigned int getReturnFmtId();

	/**
	 * @brief Set the current FMT ID of the return link of the terminal
	 *
	 * @param fmt_id  The current FMT ID of the terminal
	 */
	void setReturnFmtId(unsigned int fmt_id);

	/**
	 * @brief Get the current carriers group for the terminal
	 *
	 * @return the ID of the carriers group
	 */
	unsigned int getCarrierId();

	/**
	 * @brief Set the current carriers group for the terminal
	 *
	 * @param carrier_id  The current carriers group
	 */
	void setCarrierId(unsigned int carrier_id);

  protected:

	/** CRA for the terminal (kb/s) */
	rate_kbps_t cra_kbps;

	/** Maximual RBDC value (kb/s) */
	rate_kbps_t max_rbdc_kbps;

	/** RBDC request timeout */
	time_sf_t rbdc_timeout_sf;

	/*** The maximum VBDC value */
	vol_kb_t max_vbdc_kb;

	/** The FMT ID of the forward link */
	unsigned int fwd_fmt_id;

	/** The FMT ID of the return link */
	unsigned int ret_fmt_id;

	/** The carrier ID */
	unsigned int carrier_id;
};

#endif

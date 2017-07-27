/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @author Aurelien DELRIEU <adelrieutoulouse.viveris.com>
 *
 */

#ifndef _TERMINAL_CONTEXT_DAMA_RCS_H_
#define _TERMINAL_CONTEXT_DAMA_RCS_H_

#include "TerminalContextDama.h"

#include "OpenSandCore.h"

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
	                       vol_kb_t max_vbdc_kb);

	virtual ~TerminalContextDamaRcs();

	/**
	 * @brief Get the required FMT of the terminal
	 *
	 * @return the required FMT
	 */
	FmtDefinition *getRequiredFmt() const;

	/**
	 * @brief Set the required FMT of the terminal
	 *
	 * @param fmt_def  The required FMT of the terminal
	 */
	virtual void setRequiredFmt(FmtDefinition *fmt);

	/**
	 * @brief Get the current FMT ID of the terminal
	 *
	 * @return the ID of FMT
	 */
	unsigned int getFmtId() const;

	/**
	 * @brief Get the current FMT of the terminal
	 *
	 * @return the FMT
	 */
	FmtDefinition *getFmt() const;

	/**
	 * @brief Set the current FMT of the terminal
	 *
	 * @param fmt_def  The current FMT of the terminal
	 */
	virtual void setFmt(FmtDefinition *fmt);

	/**
	 * @brief Get the current carriers group for the terminal
	 *
	 * @return the ID of the carriers group
	 */
	unsigned int getCarrierId() const;

	/**
	 * @brief Set the current carriers group for the terminal
	 *
	 * @param carrier_id  The current carriers group
	 */
	void setCarrierId(unsigned int carrier_id);

 protected:

	/** The required FMT */
	FmtDefinition *req_fmt_def;

	/** The FMT */
	FmtDefinition *fmt_def;

	/** The carrier ID */
	unsigned int carrier_id;
};

#endif

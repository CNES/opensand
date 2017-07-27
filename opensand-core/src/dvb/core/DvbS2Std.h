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
 * @file DvbS2Std.h
 * @brief DVB-S2 Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_S2_STD_H
#define DVB_S2_STD_H

#include "PhysicStd.h"
#include "BBFrame.h"
#include "FmtDefinitionTable.h"

#include <opensand_output/OutputLog.h>


/**
 * @class DvbS2Std
 * @brief DVB-S2 Transmission Standard
 */
class DvbS2Std: public PhysicStd
{

 private:

	// TODO create a type for modcod_id
	/** The real MODCOD of the ST */
	uint8_t real_modcod;

	/** The received MODCOD */
	uint8_t received_modcod;

	/** The modcod definition table */
	FmtDefinitionTable *modcod_def;

 protected:

	// whether this is a SCPC reception standard
	bool is_scpc;

	// Output Log
	OutputLog* log_rcv_from_down;

 public:

	/**
	 * Build a DVB-S2 Transmission Standard
	 *
	 * @param packet_handler the packet handler
	 */
	DvbS2Std(EncapPlugin::EncapPacketHandler *pkt_hdl);

	/**
	 * Build a DVB-S2 Transmission Standard
	 *
	 * @param type     the type of the DVB standard
	 * @param packet_handler the packet handler
	 */
	DvbS2Std(string type,
	         EncapPlugin::EncapPacketHandler *pkt_hdl);

	/**
	 * Destroy the DVB-S2 Transmission Standard
	 */
	virtual ~DvbS2Std();

	/* only for NCC and Terminals */
	bool onRcvFrame(DvbFrame *dvb_frame,
	                tal_id_t tal_id,
	                NetBurst **burst);

	/**
	 * @brief  Get the real MODCOD for the terminal
	 *
	 * @return the real MODCOD
	 */
	uint8_t getRealModcod() const
	{
		return this->real_modcod;
	};

	/**
	 * @brief  Set the real MODCOD for the terminal
	 *
	 * @param the real MODCOD
	 */
	void setRealModcod(uint8_t new_real_modcod)
	{
		this->real_modcod = new_real_modcod;
	};

	/**
	 * @brief  Get the received MODCOD for the terminal
	 *
	 * @return the received MODCOD
	 */
	uint8_t getReceivedModcod() const
	{
		return this->received_modcod;
	};

	/**
	 * @brief  Set the MODCOD definition table
	 *
	 * @param the new modcod_def
	 */
	void setModcodDef(FmtDefinitionTable *const new_modcod_def)
	{
		this->modcod_def = new_modcod_def;
	}

	/**
	 * @brief  get the required Es/N0
	 *
	 * @param   the modcod_id
	 * @return  the Es/N0
	 */
	double getRequiredEsN0(int modcod_id) const
	{
		return this->modcod_def->getRequiredEsN0(modcod_id);
	}


};

class DvbScpcStd: public DvbS2Std
{
  public:
	DvbScpcStd(EncapPlugin::EncapPacketHandler *pkt_hdl);
};
#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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

/* @file ErrorInsertion.h
 *  @brief ErrorInsertion
 *  @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef ERROR_INSERTION_H
#define ERROR_INSERTION_H

#include "lib_dvb_rcs.h"

/**
 * @class ErrorInsertion
 * @brief ErrorInsertion
 */
class ErrorInsertion 
{

	public:

		/**
		 * @brief Channel ctor
		 */
		ErrorInsertion();


		/**
		 * @brief ErrorInsertion dtor
		 */
		~ErrorInsertion();

		/**
		 * @brief Determine if a Packet shall be corrupted or not depending on
		 *        the attenuationModel conditions 
		 *
		 * @return true if it must be corrupted, false otherwise 
		 */
		virtual bool isToBeModifiedPacket(double CN_uplink,
		                                  double nominalCN,
		                                  double attenuation,
		                                  double threshold_qef) = 0;

		/**
		 * @brief Corrupt a package with error bits 
		 *
		 * @param frame the packet to be modified 
		 */
		virtual void modifyPacket(T_DVB_META *frame, long length) = 0;

};

#endif

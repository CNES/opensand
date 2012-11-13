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
 * @file ModulationType.h
 * @brief The different types of modulation for MODCOD or DRA schemes
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef MODULATION_TYPE_H
#define MODULATION_TYPE_H


/**
 * @brief The different types of modulations accepted for MODCOD or DRA schemes
 */
typedef enum
{
	MODULATION_UNKNOWN,  /**< An unknown modulation */
	MODULATION_BPSK,     /**< The BPSK modulation */
	MODULATION_QPSK,     /**< The QPSK modulation */
	MODULATION_8PSK,     /**< The 8PSK modulation */
	/* add new modulations here */
} modulation_type_t;


#endif

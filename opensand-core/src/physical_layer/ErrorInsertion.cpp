/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file ErrorInsertion.cpp
 * @brief ErrorInsertion: Attribute of class Channel that manages how bit errors
 *                        affect the frames. It is defined as a Virtual Class
 *                   -ErrorInsertion process the packets with an ON/OFF perspective.
 *                   If the Carrier to Noise ratio is below a certain threshold
 *                   , the whole packet will corrupted in all its bits.
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#include "ErrorInsertion.h"

ErrorInsertion::ErrorInsertion()
{
}

ErrorInsertion::~ErrorInsertion()
{
}

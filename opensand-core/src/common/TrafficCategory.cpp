/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file TrafficCategory.cpp
 * @brief A traffic flow category regroups several traffic flows served
 *        in the same way
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

// System includes
#include <stdio.h>
#include <stdlib.h>

// Project includes
#define DBG_PACKAGE PKG_QOS_DATA
#include "opensand_conf/uti_debug.h"
#include "TrafficCategory.h"


/**
 * constructor
 */
TrafficCategory::TrafficCategory(): name()
{
	id = 0;
}

/**
 * Destroy the TrafficCategory object
 */
TrafficCategory::~TrafficCategory()
{
}

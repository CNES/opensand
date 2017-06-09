/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 * @file BaseProbe.cpp
 * @brief The BaseProbe class represents an untyped probe
 *        (base class for Probe<T> classes).
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */

#include "BaseProbe.h"

#include <stdlib.h>
#include <string.h>

BaseProbe::BaseProbe(uint8_t id, const string &name,
                     const string &unit,
                     bool enabled, sample_type_t type):
	id(id),
	name(name),
	unit(unit),
	enabled(enabled),
	s_type(type),
	values_count(0)
{
}

BaseProbe::~BaseProbe()
{
}

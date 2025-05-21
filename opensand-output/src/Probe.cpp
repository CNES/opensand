/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
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
 * @file Probe.h
 * @brief Template specialization of the storage type ID for Probe<T> classes.
 * @author Vincent Duvert     <vduvert@toulouse.viveris.com>
 * @author Mathias Ettinger   <mathias.ettinger@viveris.fr>
 */

#include <arpa/inet.h>
#include <endian.h>
#include <cstring>
#include <cstdint>

#include "Probe.h"


template<>
datatype_t Probe<int32_t>::getDataType() const
{
  return INT32_TYPE;
}

template<>
datatype_t Probe<float>::getDataType() const
{
  return FLOAT_TYPE;
}

template<>
datatype_t Probe<double>::getDataType() const
{
  return DOUBLE_TYPE;
}

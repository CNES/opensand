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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file OutputMutex.h
 * @brief  Wrapper for using a mutex with RAII method
 * @author Julien BERNARD     <jbernard@toulouse.viveris.com>
 * @author Mathias Ettinger   <mathias.ettinger@viveris.fr>
 */



#ifndef OUTPUT_MUTEX_H
#define OUTPUT_MUTEX_H

#include <mutex>


using OutputMutex = std::mutex;
using OutputLock = std::lock_guard<OutputMutex>;


#endif

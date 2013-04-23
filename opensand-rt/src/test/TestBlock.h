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
 * @file TestBlock.h
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @brief This test check that we can raise a timer on a channel then
 *        write on a socket that will be monitored by the opposite channel
 */

#include "Block.h"

class TestBlock: public Block
{

  public:

	TestBlock(const string &name);
	~TestBlock();
	bool onUpwardEvent(const Event *const event);
	bool onDownwardEvent(const Event *const event);
	bool onInit(void);
	
  protected:

	// for upward
	uint32_t nbr_timeouts;
	int32_t output_fd;
	// for downward
	int32_t input_fd;

	/// the data written by timer that should be read on socket
	char last_written[64];
};




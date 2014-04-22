/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file TestMultiBlocks.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @brief This test check that we can read a file on a channel then
 *        transmit content to lower block, the bottom block transmit it
 *        to the following channel that will forward it to the top and
 *        compare output
 */


#ifndef TEST_MULTI_BLOCK_H
#define TEST_MULTI_BLOCK_H


#include "Block.h"
#include "NetSocketEvent.h"

class TopBlock: public Block
{

  public:

	TopBlock(const string &name, string input_file);
	~TopBlock();


  protected:

	bool onUpwardEvent(const RtEvent *const event);
	bool onDownwardEvent(const RtEvent *const event);
	bool onInit(void);

	string input_file;
	int32_t input_fd;
	char last_written[MAX_SOCK_SIZE + 1];

};

class MiddleBlock: public Block
{

  public:

	MiddleBlock(const string &name);
	~MiddleBlock();


  protected:

	bool onUpwardEvent(const RtEvent *const event);
	bool onDownwardEvent(const RtEvent *const event);
	bool onInit(void);


};

class BottomBlock: public Block
{

  public:

	BottomBlock(const string &name);
	~BottomBlock();


  protected:


	bool onUpwardEvent(const RtEvent *const event);
	bool onDownwardEvent(const RtEvent *const event);
	bool onInit(void);

	int32_t input_fd;
	int32_t output_fd;

};

#endif

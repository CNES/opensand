/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file TestInterconnectBlock.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.fr>
 * @brief This test check that we can read a file on a channel then
 *        transmit content to lower block, the bottom block transmit it
 *        to the following channel that will forward it to the top and
 *        compare output
 */


#ifndef TEST_INTERCONNECT_BLOCK_H
#define TEST_INTERCONNECT_BLOCK_H


#include "opensand_rt/Rt.h"
#include "Config.h"

struct top_specific
{
    string input_file;
    string output_file;
    bool must_wait;
};

class TopBlock: public Block
{

  public:

	TopBlock(const string &name, struct top_specific spec);
	~TopBlock();

    class Upward: public RtUpward
    {
     public:
        Upward(Block *const bl, struct top_specific spec __attribute__((unused))):
            RtUpward(bl)
        {};
        bool onInit(void) { return true; };
        bool onEvent(const RtEvent *const event);
    };

    class Downward: public RtDownward
    {
     public:
        Downward(Block *const bl, struct top_specific spec __attribute__((unused))):
            RtDownward(bl)
        {};
        bool onInit(void) { return true; };
        bool onEvent(const RtEvent *const event);
    };

    const string& getName() const { return this->name; };
    int32_t getOutputFd() const { return this->output_fd; };
    int32_t getInputFd() const { return this->input_fd; };
    ssize_t writeOutput(const void * buf, size_t length);
    void saveOutput();
    clock_t getStartT() const { return this->start_t; };
    clock_t getEndT() const { return this->end_t; };
    void setStartT(clock_t time) { this->start_t = time; };
    void setEndT(clock_t time) { this->end_t = time; };
    void startReading();
  
  protected:

	bool onUpwardEvent(const RtEvent *const event);
	bool onDownwardEvent(const RtEvent *const event);
	bool onInit(void);

    bool must_wait;
	string input_file;
    string output_file;
	int32_t input_fd;
    int32_t output_fd;
    clock_t start_t;
    clock_t end_t;

};

class BottomBlock: public Block
{

  public:

	BottomBlock(const string &name);
	~BottomBlock();
    
    const string name;
    
    class Upward: public RtUpward
    {
     public:
         Upward(Block *const bl):
            RtUpward(bl)
        { };
        bool onInit(void) { return true; };
        bool onEvent(const RtEvent *const event);
    };
    class Downward: public RtDownward
    {
     public:
        Downward(Block *const bl):
            RtDownward(bl)
        { };
        bool onInit(void) { return true; };
        bool onEvent(const RtEvent *const event);
    };
    
    const string& getName() const { return this->name; };
    const int32_t& getOutputFd() const { return this->output_fd; };
  
  protected:

	bool onUpwardEvent(const RtEvent *const event);
	bool onDownwardEvent(const RtEvent *const event);
	bool onInit(void);

	int32_t input_fd;
	int32_t output_fd;

};

#endif

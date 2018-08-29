/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file BlockInterconnect.h
 * @brief This file describes two blocks interconnected via an InterconnectChannel.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef BlockInterconnect_H
#define BlockInterconnect_H

#include "InterconnectChannel.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <unistd.h>
#include <signal.h>
#include <list>

struct ic_specific
{
  string interconnect_iface; // Interconnect interface name
  string interconnect_addr; // Interconnect interface IP address
};

/**
 * @class BlockInterconnectDownward
 * @brief This block implements an interconnection block facing downwards.
 */
class BlockInterconnectDownward: public Block
{
 public:

  /**
   * @brief The interconnect block, placed below
   *
   * @param name      The block name
   * @param specific  Specific block parameters
   */
  BlockInterconnectDownward(const string &name,
                            struct ic_specific specific):
    Block(name)
  {};

  ~BlockInterconnectDownward() {};

  class Upward: public RtUpward, public InterconnectChannelReceiver
  {
   public:
    Upward(const string &name, struct ic_specific specific):
      RtUpward(name),
      InterconnectChannelReceiver(name + ".Upward",
                                  specific.interconnect_iface,
                                  specific.interconnect_addr)
    {};

    bool onInit(void);
    bool onEvent(const RtEvent *const event);

   private:
  };

  class Downward: public RtDownward, public InterconnectChannelSender
  {
   public:
    Downward(const string &name, struct ic_specific specific):
      RtDownward(name),
      InterconnectChannelSender(name + ".Downward",
                                specific.interconnect_iface,
                                specific.interconnect_addr)
    {};

    bool onInit(void);
    bool onEvent(const RtEvent *const event);

   private:
  };

 protected:
  // Output log
  OutputLog *log_interconnect;

  /// event handlers
  bool onDownwardEvent(const RtEvent *const event);
  bool onUpwardEvent(const RtEvent *const event);

  // initialization method
  bool onInit();
};

/**
 * @class BlockInterconnectUpward
 * @brief This bloc implements an interconnection block facing upwards
 */
class BlockInterconnectUpward: public Block
{
 public:

  /**
   * @brief The interconnect block, placed below
   *
   * @param name      The block name
   * @param specific  Specific block parameters
   */
  BlockInterconnectUpward(const string &name,
                          struct ic_specific specific):
    Block(name)
  {};

  ~BlockInterconnectUpward() {};

  class Upward: public RtUpward, public InterconnectChannelSender
  {
   public:
    Upward(const string &name, struct ic_specific specific):
      RtUpward(name),
      InterconnectChannelSender(name + ".Upward",
                                specific.interconnect_iface,
                                specific.interconnect_addr)
    {};

    bool onInit(void);
    bool onEvent(const RtEvent *const event);

   private:
  };

  class Downward: public RtDownward, public InterconnectChannelReceiver
  {
   public:
    Downward(const string &name, struct ic_specific specific):
      RtDownward(name),
      InterconnectChannelReceiver(name + ".Downward",
                                  specific.interconnect_iface,
                                  specific.interconnect_addr)
    {};

    bool onInit(void);
    bool onEvent(const RtEvent *const event);

   private:
  };

 protected:
  // Output log 
  OutputLog *log_interconnect;

  /// event handlers
  bool onDownwardEvent(const RtEvent *const event);
  bool onUpwardEvent(const RtEvent *const event);

  // initialization method
  bool onInit();
};

#endif

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file BlockInterconnectUpward.h
 * @brief This block implements an interconnection block facing upwards.
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.fr>
 */

#ifndef BlockInterconnectUpward_H
#define BlockInterconnectUpward_H

#include "InterconnectChannel.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <opensand_conf/conf.h>

#include "OpenSandFrames.h"
#include "DvbFrame.h"

#include <unistd.h>
#include <signal.h>

struct icu_specific
{
	string interconnect_iface; // Interconnect interface name
	string interconnect_addr; // Interconnect interface IP address
};

/**
 * @class BlockInterconnectUpward
 * @brief This bloc implements an interconnection block facing upwards
 */
template <class T = DvbFrame>
class BlockInterconnectUpwardTpl: public Block
{
 public:

	/**
	 * @brief The interconnect block, placed below
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockInterconnectUpwardTpl(const string &name,
	                           struct icu_specific specific):
		Block(name)
	{};

	~BlockInterconnectUpwardTpl() {};

	template <class O = T>
	class UpwardTpl: public RtUpward, public InterconnectChannelSender
	{
	 public:
		UpwardTpl(const string &name, struct icu_specific specific):
			RtUpward(name),
			InterconnectChannelSender(name + ".Upward",
			                          specific.interconnect_iface,
			                          specific.interconnect_addr)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
	};
	typedef UpwardTpl<> Upward;

	template <class O = T>
	class DownwardTpl: public RtDownward, public InterconnectChannelReceiver
	{
	 public:
		DownwardTpl(const string &name, struct icu_specific specific):
			RtDownward(name),
			InterconnectChannelReceiver(name + ".Downward",
			                            specific.interconnect_iface,
			                            specific.interconnect_addr)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
	};
	typedef DownwardTpl<> Downward;

 protected:
	// Output log 
	OutputLog *log_interconnect;

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();
};

typedef BlockInterconnectUpwardTpl<> BlockInterconnectUpward;

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::DownwardTpl<O>::onEvent(const RtEvent *const event)
{
  bool status = true;

  switch(event->getType())
  {
    case evt_net_socket:
    {
      // Data to read in InterconnectChannel socket buffer
      O *object;
      interconnect_msg_buffer_t *buf = nullptr;
      int ret;

      LOG(this->log_interconnect, LEVEL_DEBUG,
          "NetSocket event received\n");

      // for UDP we need to retrieve potentially desynchronized
      // datagrams => loop on receive function
      do
      {
        ret = this->receiveMessage((NetSocketEvent *)event,
                                   &buf);
        if(ret < 0)
        {
          LOG(this->log_interconnect, LEVEL_ERROR,
              "failed to receive data on "
              "input channel\n");
          status = false;
        }
        else if(buf)
        {
          LOG(this->log_interconnect, LEVEL_DEBUG,
              "%zu bytes of data received\n",
              buf->data_len);
          if(buf->msg_type == msg_object)
          {
            // Reconstruct object
            O::newFromInterconnect(buf->msg_data, buf->data_len, &object);
            // Send message to the next block
            if(!this->enqueueMessage((void **)(&object)))
            {
              LOG(this->log_interconnect, LEVEL_ERROR,
                  "failed to send message to next block\n");
              status = false;
            }
          }
          else
          {
            // Must copy object to a new buffer, since enqueueMessage
            // will free it.
            unsigned char *tmp_buf = (unsigned char *)calloc(buf->data_len,
                                                             sizeof(unsigned char));
            memcpy(tmp_buf, buf->msg_data, buf->data_len);
            // Send message to the next block
            if(!this->enqueueMessage((void **) &tmp_buf,
                                     buf->data_len, buf->msg_type))
            {
              LOG(this->log_interconnect, LEVEL_ERROR,
                  "failed to send message to next block\n");
              status = false;
            }
          }
          // Free old data
          free(buf);
        }
      } while(ret > 0);
    }
    break;

    default:
      LOG(this->log_interconnect, LEVEL_ERROR,
          "unknown event received %s\n",
          event->getName().c_str());
      return false;
  }

  return status;
 
}

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::UpwardTpl<O>::onEvent(const RtEvent *const event)
{
	int ret; 

	switch(event->getType())
	{
		case evt_message:
		{
      size_t data_len;
      rt_msg_t message = ((MessageEvent *)event)->getMessage();
      O *object = (O *) message.data;

      // Check if object inside
      if(message.length == 0 && message.type == 0)
      {
        O::toInterconnect(object,
                          this->out_buffer.msg_data,
                          data_len);
        this->out_buffer.msg_type = msg_object;
        this->out_buffer.data_len = data_len;
        // delete original object
        delete object;
      } // TODO: handle other types of message

      if(!this->sendMessage())
      {
        LOG(this->log_interconnect, LEVEL_ERROR,
            "error when sending data\n");
        return false;
      }
		}
		break;

		default:
			LOG(this->log_interconnect, LEVEL_ERROR,
			    "unknown event received %s\n", 
			    event->getName().c_str());
			return false;
	}

	return true;
}


template <class T>
bool BlockInterconnectUpwardTpl<T>::onInit(void)
{
	// Register log	
	this->log_interconnect = Output::registerLog(LEVEL_WARNING, "InterconnectUpward.block");
	return true;
}

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::UpwardTpl<O>::onInit(void)
{
  unsigned int stack;
  unsigned int rmem;
  unsigned int wmem;
  unsigned int port;
  string remote_addr("");

  // Get configuration
  // NOTE: this works now that only one division is made per component. If we
  // wanted to split one component in more than three, dedicated configuration
  // is needed, to tell different configurations apart.
  // get remote IP address
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UPPER_IP, remote_addr))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UPPER_IP);
    return false;
  }
  // get port
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UPWARD_PORT, port))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UPWARD_PORT);
    return false;
  }
  // get UDP stack
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UDP_STACK, stack))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UDP_STACK);
    return false;
  }
  // get rmem
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UDP_RMEM, rmem))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UDP_RMEM);
    return false;
  }
  // get wmem
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UDP_WMEM, wmem))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UDP_WMEM);
    return false;
  }

  // Create channel
  this->initUdpChannel(port, remote_addr, stack, rmem, wmem);

	return true;
}

template <class T>
template <class O>
bool BlockInterconnectUpwardTpl<T>::DownwardTpl<O>::onInit()
{
	string name="DownwardInterconnectChannel";
  unsigned int stack;
  unsigned int rmem;
  unsigned int wmem;
  unsigned int port;
  string remote_addr("");

  // Get configuration
  // NOTE: this works now that only one division is made per component. If we
  // wanted to split one component in more than three, dedicated configuration
  // is needed, to tell different configurations apart.
  // get remote IP address
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UPPER_IP, remote_addr))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UPPER_IP);
    return false;
  }
  // get port
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_DOWNWARD_PORT, port))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_DOWNWARD_PORT);
    return false;
  }
  // get UDP stack
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UDP_STACK, stack))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UDP_STACK);
    return false;
  }
  // get rmem
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UDP_RMEM, rmem))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UDP_RMEM);
    return false;
  }
  // get wmem
  if(!Conf::getValue(Conf::section_map[INTERCONNECT_SECTION],
                     INTERCONNECT_UDP_WMEM, wmem))
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Section %s, %s missing\n",
        INTERCONNECT_SECTION, INTERCONNECT_UDP_WMEM);
    return false;
  }

  // Create channel
  this->initUdpChannel(port, remote_addr, stack, rmem, wmem);

  // Add NetSocketEvent
  this->socket_event = this->addNetSocketEvent(name, this->channel->getChannelFd(),
                                               MAX_SOCK_SIZE);
  if(this->socket_event < 0)
  {
    LOG(this->log_init, LEVEL_ERROR,
        "Cannot add event to Downward channel\n");
    return false;
  }

	return true;
}

#endif

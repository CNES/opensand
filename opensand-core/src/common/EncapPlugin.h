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
 * @file EncapPlugin.h
 * @brief Generic encapsulation / deencapsulation plugin
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef ENCAP_CONTEXT_H
#define ENCAP_CONTEXT_H

#include <map>
#include <string>

#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandCore.h"
#include "StackPlugin.h"

#include <opensand_output/Output.h>

/**
 * @class EncapPlugin
 * @brief Generic encapsulation / deencapsulation plugin
 */
class EncapPlugin: public StackPlugin
{

 public:

	EncapPlugin(uint16_t ether_type): StackPlugin(ether_type)
	{
	};

	/**
	 * @class EncapPacketHandler
	 * @brief Functions to handle the encapsulated packets
	 */
	class EncapPacketHandler: public StackPacketHandler
	{

	 public:

		/**
		 * @brief EncapPacketHandler constructor
		 */
		/* Allow packets to access EncapPlugin members */
		EncapPacketHandler(EncapPlugin &pl):
			StackPacketHandler(pl)
		{
		};

		/**
		 * @brief Get the source terminal ID of a packet
		 *
		 * @param data    The packet content
		 * @param tal_id  OUT: the source terminal ID of the packet
		 * @return true on success, false otherwise
		 */
		virtual bool getSrc(const Data &data, tal_id_t &tal_id) const = 0;


		virtual void init()
		{
			this->log = Output::registerLog(LEVEL_WARNING,
			                                "Encap.%s",
			                                this->getName().c_str());
		};

	 protected:

		/// Output Logs
		OutputLog *log;
	};

	/**
	 * @class EncapContext
	 * @brief The encapsulation/deencapsulation context
	 */
	class EncapContext: public StackContext
	{
	  public:

		/* Allow context to access EncapPlugin members */
		/**
		 * @brief EncapContext constructor
		 */
		EncapContext(EncapPlugin &pl):
			StackContext(pl)
		{
			this->dst_tal_id = BROADCAST_TAL_ID;
		};
		
		/**
		 * Flush the encapsulation context identified by context_id (after a context
		 * expiration for example). It's the caller charge to delete the returned
		 * NetBurst after use.
		 *
		 * @param context_id  the context to flush
		 * @return            a list of encapsulation packets
		 */
		// TODO replace int by uintXX_t
		virtual NetBurst *flush(int context_id) = 0;
		
		/**
		 * Flush all the encapsulation contexts. It's the caller charge to delete
		 * the returned NetBurst after use.
		 *
		 * @return  a list of encapsulation packets
		 */
		virtual NetBurst *flushAll() = 0;

		/**
		 * @brief Set the filter on destination TAL Id.
		 *
		 * @param tal_id  The destination TAL Id.
		 */
		void setFilterTalId(uint8_t tal_id)
		{
			this->dst_tal_id = tal_id;
		}

		virtual void init()
		{
			this->log = Output::registerLog(LEVEL_WARNING,
			                                "Encap.%s",
			                                this->getName().c_str());
		};

	  protected:

		/// The destination TAL Id to filter received packet on
		uint8_t dst_tal_id;

		/// Output Logs
		OutputLog *log;
	};

	virtual void init()
	{
		this->log = Output::registerLog(LEVEL_WARNING,
		                                "Encap.%s",
		                                this->getName().c_str());
	};

	/* for the following functions we use "covariant return type" */

	/**
	 * @brief Get the context
	 *
	 * @return the context
	 */
	EncapContext *getContext() const
	{
		return static_cast<EncapContext *>(this->context);
	};

	/**
	 * @brief Get the packet handler
	 *
	 * @return the packet handler
	 */
	EncapPacketHandler *getPacketHandler() const
	{
		return static_cast<EncapPacketHandler *>(this->packet_handler);
	};

};

typedef std::vector<EncapPlugin::EncapContext *> encap_contexts_t;

#ifdef CREATE
#undef CREATE
#define CREATE(CLASS, CONTEXT, HANDLER, pl_name) \
			CREATE_STACK(CLASS, CONTEXT, HANDLER, pl_name, encapsulation_plugin)
#endif

#endif

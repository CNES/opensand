/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
 * @file Ip.h
 * @brief IP lan adaptation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef IP_CONTEXT_H
#define IP_CONTEXT_H

#include "LanAdaptationPlugin.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "IpPacket.h"
#include "Ipv4Packet.h"
#include "Ipv6Packet.h"
#include "TrafficCategory.h"

#include <opensand_conf/conf.h>

#include <cassert>
#include <vector>
#include <map>


/**
 * @class Ip
 * @brief IP lan adaptation plugin implementation
 */
class Ip: public LanAdaptationPlugin
{
  public:

	/**
	 * @class Context
	 * @brief IP context
	 */
	class Context: public LanAdaptationContext
	{
	  public:

		/// constructor
		Context(LanAdaptationPlugin &plugin);

		/**
		 * Destroy the  context
		 */
		~Context();

		void init();
		NetBurst *encapsulate(NetBurst *burst, std::map<long, int> &(time_contexts));
		NetBurst *deencapsulate(NetBurst *burst);
		char getLanHeader(unsigned int pos, NetPacket *packet);
		bool handleTap() {return false;};

	  protected:
		/**
		 * @brief handle an IP message
		 *
		 * @param ip_packet  The IP packet
		 * @return true on success, false otherwise
		 */
		bool onMsgIp(IpPacket *ip_packet);

	  private:

		/**
		 * @brief Initialize the traffic categories from IP configuration
		 *
		 * @param config  The configuration elements
		 * @return true on success, false otherwise
		 */
		bool initTrafficCategories(ConfigurationFile &config);

		/// The traffic categories
		std::map<qos_t, TrafficCategory *> category_map;

		/// The default traffic category
		qos_t default_category;
	};

	/**
	 * @class Packet
	 * @brief IP packet
	 */
	class PacketHandler: public LanAdaptationPacketHandler
	{

	  public:

		PacketHandler(LanAdaptationPlugin &plugin):
			LanAdaptationPlugin::LanAdaptationPacketHandler(plugin)
		{
		};

		size_t getFixedLength() const {return 0;};

		size_t getLength(const unsigned char *UNUSED(data)) const
		{
			return 0;
		}

		std::string getName() const {return "IP";}

		NetPacket *build(const Data &data,
		                 size_t data_length,
		                 uint8_t qos,
		                 uint8_t src_tal_id,
		                 uint8_t dst_tal_id) const;
	};
	
	/// Constructor
	Ip();
};

CREATE(Ip, Ip::Context, Ip::PacketHandler, "IP");

#endif


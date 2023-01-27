/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file DvbFrame.h
 * @brief DVB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
 */

#ifndef DVB_FRAME_H
#define DVB_FRAME_H

#include <cstring>
#include <opensand_rt/Ptr.h>

#include "OpenSandFrames.h"
#include "NetContainer.h"
#include "NetPacket.h"


/**
 * @class DvbFrameTpl
 * @brief DVB frame template
 */
template<class T = T_DVB_FRAME>
class DvbFrameTpl: public NetContainer
{
protected:
	/** The maximum size (in bytes) of the DVB frame */
	size_t max_size;

	/** The number of encapsulation packets added to the DVB frame */
	uint16_t num_packets;

	/** The carrier Id */
	uint8_t carrier_id;

public:
	using DvbHeaderType = T;

	/**
	 * Build a DVB frame
	 *
	 * @param data    raw data from which a DVB frame can be created
	 * @param length  length of raw data
	 */
	DvbFrameTpl(const unsigned char *data, size_t length):
		NetContainer(data, length),
		max_size(sizeof(T)),
		num_packets(0),
		carrier_id(-1)
	{
		this->name = "DvbFrame";
		this->trailer_length = this->getTotalLength() - this->getMessageLength();
		this->header_length = sizeof(T);
	};

	/**
	 * Build a DVB frame
	 *
	 * @param data  raw data from which a DVB frame can be created
	 */
	DvbFrameTpl(const Rt::Data &data):
		NetContainer(data),
		max_size(sizeof(T)),
		num_packets(0),
		carrier_id(0)
	{
		this->name = "DvbFrame";
		this->trailer_length = this->getTotalLength() - this->getMessageLength();
		this->header_length = sizeof(T);
	};

	/**
	 * Build a DVB frame
	 *
	 * @param data    raw data from which a DVB frame can be created
	 * @param length  length of raw data
	 */
	DvbFrameTpl(const Rt::Data &data, size_t length):
		NetContainer(data, length),
		max_size(sizeof(T)),
		num_packets(0),
		carrier_id(0)
	{
		this->name = "DvbFrame";
		this->trailer_length = this->getTotalLength() - this->getMessageLength();
		this->header_length = sizeof(T);
	};

	/**
	 * Build an empty DVB frame
	 */
	DvbFrameTpl():
		NetContainer(),
		max_size(sizeof(T)),
		num_packets(0),
		carrier_id(0)
	{
		T header;
		this->name = "DvbFrame";
		this->data.reserve(this->max_size);
		// add at least the base header of the created frame
		memset(&header, 0, sizeof(T));
		this->data.append(reinterpret_cast<unsigned char *>(&header), sizeof(T));
		this->header_length = sizeof(T);
	};

	virtual ~DvbFrameTpl() {};


	// setters and getters on T_DVB_HDR

	/**
	 * @brief Set the DVB header message type
	 * 
	 * @param type  The DVB frame message type
	 */
	void setMessageType(EmulatedMessageType type)
	{
		this->frame()->hdr.msg_type = type;
	};
	
	/**
	 * @brief Set the DVB header message type
	 * 
	 * @param type  The DVB frame message type
	 */
	void setCorrupted(bool corrupted)
	{
		if(corrupted)
		{
			this->frame()->hdr.corrupted = 1;
		}
		else
		{
			this->frame()->hdr.corrupted = 0;
		}
	};
	
	/**
	 * @brief Set the DVB frame length
	 * 
	 * @param length  The DVB frame length
	 */
	void setMessageLength(uint16_t length)
	{
		this->frame()->hdr.msg_length = htons(length);
	};
	
	/**
	 * @brief Get the DVB header message type
	 * 
	 * @return  The DVB frame message type
	 */
	EmulatedMessageType getMessageType() const
	{
		return this->frame()->hdr.msg_type;
	};
	
	/**
	 * @brief Get the DVB corrupted status
	 * 
	 * @return  The status of the DVB frame
	 */
	bool isCorrupted() const
	{
		return ((this->frame()->hdr.corrupted & 0x1) == 1);
	};
	
	/**
	 * @brief Get the DVB frame length
	 * 
	 * @return  The DVB frame length
	 */
	uint16_t getMessageLength() const
	{
		return ntohs(this->frame()->hdr.msg_length);
	};


	// Setter and getters on DVB frame attributes

	/**
	 * Get the maximum size of the DVB frame
	 *
	 * @return  the size (in bytes) of the DVB frame
	 */
	size_t getMaxSize() const
	{
		return this->max_size;
	};

	/**
	 * Set the maximum size of the DVB frame
	 *
	 * @param size  the size (in bytes) of the DVB frame
	 */
	void setMaxSize(unsigned int size)
	{
		this->max_size = size;
		this->data.reserve(size);
		// we need to do that again because data may have moved
		// this is very important to set max size because data may also move
		// when using append
	};

	/**
	 * Get the carrier ID of the DVB frame
	 *
	 * @return The carrier ID the frame will be sent on
	 */
	uint8_t getCarrierId() const
	{
		return this->carrier_id;
	};

	/**
	 * Set the carrier ID of the DVB frame
	 *
	 * @param carrier_id  the carrier ID the frame will be sent on
	 */
	void setCarrierId(uint8_t carrier_id)
	{
		this->carrier_id = carrier_id;
	};

	/**
	 * How many free bytes are available in the DVB frame ?
	 *
	 * @return  the size (in bytes) of the free space in the DVB frame
	 */
	size_t getFreeSpace() const
	{
		return (this->max_size - this->getTotalLength());
	};

	/**
	 * Add an encapsulation packet to the DVB frame
	 *
	 * @param packet  the encapsulation packet to add to the DVB frame
	 * @return        true if the packet was added to the DVB frame,
	 *                false if an error occurred
	 */
	virtual bool addPacket(const NetPacket &packet)
	{
		// is the frame large enough to contain the packet ?
		if(packet.getTotalLength() > this->getFreeSpace())
		{
			// too few free space in the frame
			return false;
		}

		this->data.append(packet.getData());
		this->num_packets++;

		return true;
	};

	/**
	 * Get the encapsulation packets count into the DVB frame
	 *
	 * @return  the encapsulation packets count
	 */
	virtual unsigned int getPacketsCount() const
	{
		return this->num_packets;
	}

	/**
	 * Empty the DVB frame
	 */
	virtual void empty() {};

	/**
	 * Get the C/N value carried by the frame
	 *
	 * @return the C/N value
	 */
	double getCn() const
	{
		size_t msg_length = this->getMessageLength();
		Rt::Data phy_data = this->getData(msg_length);
		return ncntoh(reinterpret_cast<T_DVB_PHY*>(phy_data.data())->cn_previous);
	};

	/**
	 * Set the C/N value carried by the frame
	 *
	 * @param cn  The C/N value
	 */
	void setCn(double cn)
	{
		T_DVB_PHY phy;
		phy.cn_previous = hcnton(cn);

		unsigned char *raw_phy = reinterpret_cast<unsigned char *>(&phy);
		if(this->trailer_length == 0)
		{
			this->data.append(raw_phy, sizeof(T_DVB_PHY));
			this->trailer_length = sizeof(T_DVB_PHY);
		}
		else
		{
			std::size_t msg_length = this->getMessageLength();
			this->data.replace(msg_length, this->trailer_length,
			                   raw_phy, this->trailer_length);
		}
	};

	/**
	 * @brief Accessor on the frame data
	 */
	T *frame()
	{
		return reinterpret_cast<T *>(this->data.data());
	}
	const T *frame() const
	{
		return reinterpret_cast<const T *>(this->data.data());
	}

	template<typename DVB> friend Rt::Ptr<DVB> dvb_frame_upcast(Rt::Ptr<DvbFrameTpl<>> ptr);
	template<typename DVB> friend DVB& dvb_frame_upcast(DvbFrameTpl<>& frame);
};


using DvbFrame = DvbFrameTpl<>;


template<typename DVB_FRAME>
Rt::Ptr<DVB_FRAME> dvb_frame_upcast(Rt::Ptr<DvbFrame> ptr)
{
	using HeaderType = typename DVB_FRAME::DvbHeaderType;
	static_assert(std::is_base_of<DvbFrameTpl<HeaderType>, DVB_FRAME>::value,
	              "Trying to cast a non dvb frame into a dvb frame");
	ptr->header_length = sizeof(HeaderType);
	return {reinterpret_cast<DVB_FRAME*>(ptr.release()), std::move(ptr.get_deleter())};
}
template<typename DVB_FRAME>
DVB_FRAME& dvb_frame_upcast(DvbFrame& frame)
{
	using HeaderType = typename DVB_FRAME::DvbHeaderType;
	static_assert(std::is_base_of<DvbFrameTpl<HeaderType>, DVB_FRAME>::value,
	              "Trying to cast a non dvb frame into a dvb frame");
	frame.header_length = sizeof(HeaderType);
	return reinterpret_cast<DVB_FRAME&>(frame);
}


template<typename DVB_FRAME>
Rt::Ptr<DvbFrame> dvb_frame_downcast(Rt::Ptr<DVB_FRAME> ptr)
{
	static_assert(std::is_base_of<DvbFrameTpl<typename DVB_FRAME::DvbHeaderType>, DVB_FRAME>::value,
	              "Trying to cast a non dvb frame into a dvb frame");
	return {reinterpret_cast<DvbFrame*>(ptr.release()), std::move(ptr.get_deleter())};
}


#endif

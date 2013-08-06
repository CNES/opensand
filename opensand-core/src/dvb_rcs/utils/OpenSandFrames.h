/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file OpenSandFrames.h
 * @brief The headers and related information for OpenSAND frames
 */


#ifndef _OPENSAND_FRAMES_H_
#define _OPENSAND_FRAMES_H_

#include <opensand_conf/uti_debug.h>

#include "CapacityRequest.h"
#include "Ttp.h"

#include <string>
#include <stdint.h>
#include <arpa/inet.h>
#include <cstring>


/// The maximum size of a DVB-RCS frame is choosen to be totally
/// included in one sat_carrier packet
#define MSG_DVB_RCS_SIZE_MAX 1200
/// The maximum size of a BBFrame
#define MSG_BBFRAME_SIZE_MAX 8100
/// The maximum size of the pysical layer data
#define MSG_PHYFRAME_SIZE_MAX 8


/**
 * Here are defined internal dvb message types
 * They are of different kind:
 *       NCC -> ST:  this message is to be emitted from the NCC to the ST  only
 *       ST -> NCC:  -------------------------------------- ST  ------ NCC ----
 *       ST -> ST:   -----------------------------------an  ST  to an  ST  ----
 *       NCC internal: Internal NCC message but also SE to NCC message
 *       NCC -> ST: etc
 *       ST -> NCC: etc
 */

/**
 * Error type, could be used as default value that should later be replaced
 */
#define MSG_TYPE_ERROR 0

/**
 * Start of Frame, NCC -> ST
 */
#define MSG_TYPE_SOF 1

/**
 * The message has been corrupted by the physical layer
 */
#define MSG_TYPE_CORRUPTED 5

/**
 * Capacity Request, ST -> NCC
 */
#define MSG_TYPE_CR 10

/**
 * FIXME: to be documented, ST->NCC
 */
#define MSG_TYPE_CSC 11

/**
 * DVB burst, ST->ST
 */
#define MSG_TYPE_DVB_BURST 12

/**
 * BBFRAME
 */
#define MSG_TYPE_BBFRAME 13

/**
 * Normally emmitted by the Satellite Emulator to  the NCC
 * Used Internally by the Geocast hence: NCC internal
 */
//#define MSG_TYPE_SACT 20

/**
 * Allocation Table, NCC -> ST
 */
#define MSG_TYPE_TTP 21

/**
 * Synchronization message (unused), NCC->ST
 */
#define MSG_TYPE_SYNC 22

/**
 * Request a logon, ST -> NCC
 */
#define MSG_TYPE_SESSION_LOGON_REQ 50

/**
 * Response from the NCC, NCC ->ST
 */
#define MSG_TYPE_SESSION_LOGON_RESP 52

/**
 * Announce a logoff, ST -> NCC
 */
#define MSG_TYPE_SESSION_LOGOFF 51


/**
 * Basic DVB Header, other structures defined below should follow in a packet
 */
typedef struct
{
	uint16_t msg_length; ///< Total length of the message (including _this_ header)
	uint8_t msg_type;   ///< Type of the message (see \#defines above)
} __attribute__((__packed__)) T_DVB_HDR;


/**
 * Internal structure between DVB and Carrier blocks.
 * Encapsulate a DVB Frame and some metadata.
 */
typedef struct
{
	uint8_t carrier_id; ///< Identifier of the carrier
	T_DVB_HDR *hdr;  ///< Pointer to the DVB Header
} T_DVB_META; // internal, no need to pack

/**
 * Internal structure between DVB and Carrier blocks.
 * Carry information about physicalLayer block.
 */
typedef struct
{
    double cn_previous;
} T_DVB_PHY; // internal, no need to pack

/**
 * This message type is a trick.
 * It is managed by the lowest layer on top of ethernet in order to emulate
 * a synchronization algorithm.
 * Namely it is a "Start of superFrame, let us go" message.
 * It is used to tick entities every superframes.
 * An internal mechanism must be designed to awake a process every frame
 * A SOF message isn't subject to satellite delay emulation (it goes quicker
 * than light !)
 */
typedef struct
{
	T_DVB_HDR hdr;    ///< Basic DVB Header, used only to be caught by the dvb layer
	uint16_t sf_nbr;  ///< SuperFrame Number
} __attribute__((__packed__)) T_DVB_SOF;


/**
 * Logon Request
 */
typedef struct
{
	T_DVB_HDR hdr;         ///< Basic DVB Header
//	uint8_t capa;          ///< Capability of the ST, to be set to 0
	uint16_t mac;          ///< ST MAC address
	uint16_t rt_bandwidth; ///< the real time fixed bandwidth in kbits/s
	// TODO add max RBDC, min VBDC ?
} __attribute__((__packed__)) T_DVB_LOGON_REQ;


/**
 * Logon response emitted by the NCC
 */
typedef struct
{
	T_DVB_HDR hdr;      ///< Basic DVB Header
	uint16_t mac;       ///< Terminal MAC address
	uint8_t group_id;   ///< Assigned Group Id
	uint16_t logon_id;  ///< Assigned Logon Id
//	uint8_t traffic_burst_type; ///< Type of traffic, set to 0
	// TODO used ???
//	uint8_t return_vpi; ///< VPI used for Signalling on Return Link
//	uint8_t return_vci; ///< VCI used for Signalling on Return Link
} __attribute__((__packed__)) T_DVB_LOGON_RESP;


/**
 * Logoff Signalling from the ST
 */
typedef struct
{
	T_DVB_HDR hdr; ///< Basic DVB Header
	uint16_t mac;  ///< Satellite MAC ST address
} __attribute__((__packed__)) T_DVB_LOGOFF;

/**
 * BB frame header
 */
typedef struct
{
	T_DVB_HDR hdr;
	uint16_t pkt_type;   ///< EtherType of the packets contained in the BBFrame
	uint16_t data_length;
	uint8_t used_modcod;
	uint8_t real_modcod_nbr;
} __attribute__((__packed__)) T_DVB_BBFRAME;


/**
 * RealModcod option for the BB frames
 */
typedef struct
{
	uint16_t terminal_id;
	uint8_t real_modcod;
} __attribute__((__packed__)) T_DVB_REAL_MODCOD;


/**
 * Capacity Request
 */
typedef struct
{
	T_DVB_HDR hdr;           ///< Basic DVB Header
	emu_sac_t sac;
} __attribute__((packed)) T_DVB_SAC_CR;


#if 0
// TODO remove if SACT no used anymore
/**
 * Capacity demand information structure
 */
// TODO check with CR for sizes
typedef struct
{
	uint8_t route_id;        ///< Set to 0. used for on board routing
	uint8_t scaling_factor;  ///< The scale of the request
	uint8_t type;            ///< Type of CR
	uint8_t channel_id;      ///< Set to 0
	uint8_t xbdc;            ///< Number of slot requested
	uint16_t group_id;       ///< Terminal Group Id
	uint16_t logon_id;       ///< Terminal Logon Id
	uint8_t M_and_C;         ///< Set to 0
} __attribute__((__packed__)) T_DVB_SAC_CR_INFO;


/**
 * SACT, emitted by SE, a compound of CR
 */
typedef struct
{
	T_DVB_HDR hdr;         ///< Basic DVB Header
	uint16_t qty_element;  ///< Number of requests (followed by qty_element
	                       ///< T_DVB_SAC_CR_INFO)
	T_DVB_SAC_CR_INFO sac; ///< 1st element of the array (should be demoted)
} __attribute__((__packed__)) T_DVB_SACT;


/**
 * @brief return the lenght of a sac packet according to weirds thing (sac)
 * above in T_DVB_SACT
 */
#define len_sac_pkt(buff) \
	(sizeof(T_DVB_SACT) + \
	(((T_DVB_SACT *)(buff))->qty_element - 1) * sizeof(T_DVB_SAC_CR_INFO))

/**
 * @brief return the first T_DVB_SAC_CR_INFO pointer associated with a buffer
 * pointing to a T_DVB_SACT struct (counting from 0)
 * @param buff the pointer to the T_DVB_SACT struct
 */
#define first_sac_ptr(buff) (&(buff->sac))

/**
 * @brief return the ith T_DVB_SAC_CR_INFO pointer associated with a buffer
 * pointing to a T_DVB_SACT struct
 * @param i the index (input)
 * @param buff the pointer to the T_DVB_SACT struct
 */
#define ith_sac_ptr(i,buff) (&(buff->sac) + (i-1))

/**
 * @brief return the next T_DVB_SAC_CR_INFO pointer after the current one
 * @param buff the pointer to the T_DVB_SAC_CR_INFO struct
 */
#define next_sac_ptr(buff) ((T_DVB_SAC_CR_INFO *)buff + 1)
#endif

/**
 * Time Burst Time plan, essentially A basic DVB Header
 * followed by an array descriptor of T_DVB_FRAME structures
 */
typedef struct
{
	T_DVB_HDR hdr;  ///< Basic DVB_RCS Header
	emu_ttp_t ttp;  ///< The emulated TTP
} __attribute__((packed)) T_DVB_TTP;


/**
 * Format of an encapsulation frame burst
 * Essentially an encapsulation packets array descriptor
 */
typedef struct
{
	T_DVB_HDR hdr;         ///< Basic DVB_RCS Header
	uint16_t pkt_type;     ///< EtherType of the packets contained in the BBFrame
	uint16_t qty_element;  ///< Number of following encapsulation packets
} __attribute__((__packed__)) T_DVB_ENCAP_BURST;


enum
{
	msg_data = 0,  ///< message containing useful data (DVB, encap, ...)
	               //   default value of sendUp/Down function
	msg_link_up,   ///< link up message
};


/// This message is used by dvb rcs layer to advertise the upper layer
/// that the link is up
typedef struct
{
	uint8_t group_id;  /// The id of the station
	uint16_t  tal_id;
} T_LINK_UP;



/**
 * @class OpenSandFrame
 * @brief The DVB header common part
 */
template<class T>
class OpenSandFrame
{
 protected:
	/**
	 * @brief Set a DVB header from a DVB frame coming from network
	 * 
	 * @param frame   The DVB frame
	 * @param length  The DVB frame length
	 */
	OpenSandFrame(unsigned char *frame, size_t length)
	{
		this->frame = (T *)frame;
		if(this->getLength() != length)
		{
			UTI_ERROR("Wrong length received %zu, %u expected\n",
			          length, this->getLength());
			// this will segfault or return errors if we use this
			this->frame = NULL;
		}
	};
	
	/**
	 * @brief Create a new DVB header
	 * 
	 * @param length  The length allocated for the DVB frame
	 */
	OpenSandFrame(size_t length)
	{
		this->frame = (T *)calloc(sizeof(unsigned char), length);
		if(!this->frame)
		{
			UTI_ERROR("cannot allocate memory\n");
		}
	};

	/// The DVB frame
	T *frame;

 public:

	/**
	 * @brief Set the DVB header message type
	 * 
	 * @param  type the DVB frame message type
	 */
	void setMessageType(uint8_t type)
	{
		memcpy(&this->frame->hdr.msg_type, &type, sizeof(uint8_t));
	};
	
	/**
	 * @brief Set the DVB frame length
	 * 
	 * @param  length  The DVB frame length
	 */
	void setLength(uint16_t length)
	{
		uint16_t len = htons(length);
		memcpy(&this->frame->hdr.msg_length, &len, sizeof(uint16_t));
	};
	
	/**
	 * @brief Get the DVB header message type
	 * 
	 * @return  the type the DVB frame message type
	 */
	uint8_t getMessageType(void) const
	{
		return this->frame->hdr.msg_type;
	};
	
	/**
	 * @brief Get the DVB frame length
	 * 
	 * @return  the length  The DVB frame length
	 */
	uint16_t getLength(void) const
	{
		return ntohs(this->frame->hdr.msg_length);
	};
	
	/**
	 * @brief Get the DVB frame
	 * 
	 * @return  the DVB frame
	 */
	T_DVB_HDR *getFrame(void)
	{
		return (T_DVB_HDR *)this->frame;
	};

	/**
	 * @brief Build the frame
	 *
	 * @param frame   the DVB frame
	 * @param length  the frame length
	 */
	virtual void build(unsigned char *frame, size_t &length)
	{
		frame = (unsigned char *)this->frame;
		length = this->getLength();
	};
	
};


#endif

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
 * @file OpenSandCore.h
 * @brief Some OpenSAND core utilities
 */

#ifndef OPENSAND_CORE_H
#define OPENSAND_CORE_H

#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <stdint.h>
#include <cmath>
#include <sys/time.h>
#include <arpa/inet.h>


/** unused macro to avoid compilation warning with unused parameters. */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

/// Broadcast tal id is maximal tal_id value authorized (5 bits).
constexpr const uint8_t BROADCAST_TAL_ID = 0x1F;


/** The different types of DVB components */
enum class Component
{
	satellite,
	gateway,
	terminal,
	unknown,
};


/** @brief Get the name of a component
 *
 * @param host The component type
 * @return the abbreviated name of the component
 */
inline std::string getComponentName(Component host)
{
	switch(host)
	{
    case Component::satellite:
			return "sat";
    case Component::gateway:
			return "gw";
    case Component::terminal:
			return "st";
	default:
		return "unknown";
	}
};

/// Carrier access type
enum class AccessType
{
	DAMA,
	TDM,
	ALOHA,
	SCPC,
	ERROR
};


/**
 * @brief get the access type according to its name
 *
 * @param access_type the access type name
 *
 * @return the access type enum
 */
inline AccessType strToAccessType(const std::string& access_type)
{
	if(access_type == "DAMA")
		return AccessType::DAMA;
	else if(access_type == "ACM")
		return AccessType::TDM;
	else if(access_type == "ALOHA")
		return AccessType::ALOHA;
	else if(access_type == "VCM")
		return AccessType::TDM;
	else if(access_type == "SCPC")
		return AccessType::SCPC;
	return AccessType::ERROR;
}


enum class SatelliteLinkState
{
	DOWN,
	UP
};


enum class InternalMessageType : uint8_t
{
	encap_data = 0,  ///< message containing encapsulated data (type DvbFrame, below BlockDvb)
	decap_data,      ///< message containing decapsulated data (type NetBurst, above BlockDvb)
	link_up,         ///< link up message
	sig,             ///< message containing signalisation
	saloha,          ///< message containing Slotted Aloha content
	unknown,         ///< when the msg type is unknown or unused
};


enum class EncapSchemeList
{
	RETURN_UP,
	FORWARD_DOWN,
	TRANSPARENT_NO_SCHEME,
};

enum struct IslType
{
	None,
	LanAdaptation,
	Interconnect,
};

enum struct RegenLevel {
	Unknown,
	Transparent, 
	BBFrame,
	IP
};

inline RegenLevel strToRegenLevel(const std::string &regen_level)
{
	if (regen_level == "Transparent")
	{
		return RegenLevel::Transparent;
	}
	else if (regen_level == "BBFrame")
	{
		return RegenLevel::BBFrame;
	}
	else if (regen_level == "IP")
	{
		return RegenLevel::IP;
	}
	else
	{
		return RegenLevel::Unknown;
	}
}

/**
 * @brief Convert a strongly typed enum value into its underlying integral type
 */
template <typename E>
constexpr auto to_underlying(E e) noexcept -> typename std::enable_if<std::is_enum<E>::value, typename std::underlying_type<E>::type>::type {
	return static_cast<typename std::underlying_type<E>::type>(e);
}


/**
 * @brief Convert an integral type into a strongly typed enum iff the integral type
 * is the underlying type for the enum
 */
template <typename E, typename I>
constexpr auto to_enum(I i) noexcept -> typename std::enable_if<std::is_same<I, typename std::underlying_type<E>::type>::value, E>::type {
	return static_cast<E>(i);
}


/**
 * @brieg Get the current time
 *
 * @return the current time
 */
inline clock_t getCurrentTime(void)
{
	timeval current;
	gettimeofday(&current, NULL);
	return current.tv_sec * 1000 + current.tv_usec / 1000;
};

/**
 * @brief  Tokenize a string
 *
 * @param  str        The string to tokenize
 * @param  tokens     The list to add tokens into
 * @param  delimiter  The tokens' delimiter
 */
inline void tokenize(const std::string &str,
                     std::vector<std::string> &tokens,
                     const std::string& delimiters=":")
{
	// Skip delimiters at beginning.
	std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, last_pos);

	while(std::string::npos != pos || std::string::npos != last_pos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(last_pos, pos - last_pos));
		// Skip delimiters.  Note the "not_of"
		last_pos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, last_pos);
	}
};

/**
 * @brief  Convert a C/N value from host to network
 * @warning This code has not be tested and may be incorrect for endianess
 *
 * @param cn  The CN value
 * return the CN value than can be carried on network
 */
inline uint32_t hcnton(double cn)
{
	int16_t tmp_cn = static_cast<int16_t>(std::round(cn * 100));  // we take two digits in decimal part
	return htonl(static_cast<uint32_t>(tmp_cn));
};

/**
 * @brief  Convert a C/N value from network to host
 * @warning This code has not be tested and may be incorrect for endianess
 *
 * @param cn  The CN value
 * return the CN value than can be handled on host
 */
inline double ncntoh(uint32_t cn)
{
	int16_t tmp_cn = static_cast<int16_t>(ntohl(cn));
	return tmp_cn / 100.0;
};


// The types used in OpenSAND

// addressing
typedef uint16_t tal_id_t;  ///< Terminal ID (5 bits but 16 needed for simulated terminal)
typedef uint8_t spot_id_t;  ///< Spot ID (5 bits)
typedef uint8_t qos_t;      ///< QoS (3 bits)
typedef uint16_t group_id_t; ///< Groupe ID

// TODO check types according to max value
// TODO link with config
// data
typedef uint16_t rate_bps_t;    ///< Bitrate in b/s (suffix bps)
typedef uint16_t rate_kbps_t;   ///< Bitrate in kb/s (suffix kbps)
typedef uint16_t rate_pktpf_t;  ///< Rate in packets per frame (suffix pktpf)
typedef double rate_symps_t;    ///< Rate in symbols per second (bauds) (suffix symps)

// time
typedef uint16_t time_sf_t;    ///< time in number of superframes (suffix sf)
typedef uint8_t time_frame_t;  ///< time in number of frames (5 bits) (suffix frame)
typedef uint32_t time_ms_t;    ///< time in ms (suffix ms)
typedef uint16_t time_pkt_t;   ///< time in number of packets, cells, ... (suffix pkt)

// volume
typedef uint16_t vol_pkt_t;    ///< volume in number of packets/cells (suffix pkt)
typedef uint16_t vol_kb_t;     ///< volume in kbits (suffix kb)
typedef uint32_t vol_b_t;      ///< volume in bits (suffix b)
typedef uint32_t vol_bytes_t;  ///< volume in Bytes (suffix bytes)
typedef uint32_t vol_sym_t;    ///< volume in number of symbols (suffix sym)

// frequency
typedef float freq_mhz_t;    ///< frequency (MHz)
typedef uint32_t freq_khz_t; ///< frequency (kHz)

// fmt
typedef uint8_t fmt_id_t;  ///< fmt id

/**
 * @brief Generic Superframe description
 *
 *  freq
 *  ^
 *  |
 *  | +--------------+
 *  | |  f   |       |
 *  | |---+--|  sf   | sf_id
 *  | | f |f |       |
 *  | |--------------+
 *  | |   |  sf   |  | sf_id
 *  | +--------------+
 *  |
 *  +-----------------------> time
 *
 *  with sf = superframe and f = frame
 */

/**
 * @brief Superframe for DVB-RCS in OpenSAND
 *
 *  freq
 *  ^
 *  | frame duration (default: 53ms)
 *  | <-->
 *  | +---------------+
 *  | | f | f |  sf   | sf_id
 *  | |---------------+
 *  | |  sf   |  sf   | sf_id
 *  | +---------------+
 *  |
 *  +-----------------------> time
 */

struct SpotTopology
{
	spot_id_t spot_id;
	tal_id_t gw_id;
	std::unordered_set<tal_id_t> st_ids; ///< the terminals that belong to the spot
	tal_id_t sat_id_gw;                  ///< The satellite connected to the gateway of this spot
	tal_id_t sat_id_st;                  ///< The satellite connected to the terminals of this spot
	RegenLevel forward_regen_level;      ///< The regeneration level of the forward channel
	RegenLevel return_regen_level;       ///< The regeneration level of the return channel
};

struct IslConfig
{
	tal_id_t linked_sat_id;
	IslType type;
	std::string interco_addr;
	std::string tap_iface;
};

inline std::string generateProbePrefix(spot_id_t spot_id, Component entity_type, bool is_sat)
{
	std::ostringstream ss{};
	ss << "spot_" << int{spot_id} << ".";
	if (is_sat)
	{
		ss << "sat.";
	}
	ss << getComponentName(entity_type) << ".";
	return ss.str();
}

#endif


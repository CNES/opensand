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
 * @file BlockDvb.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


// FIXME we need to include uti_debug.h before...
#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>

#include "BlockDvb.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"

#include <opensand_conf/conf.h>

#include <string.h>
#include <errno.h>


// output events
Event *BlockDvb::error_init = NULL;
Event *BlockDvb::event_login_received = NULL;
Event *BlockDvb::event_login_response = NULL;

/**
 * Constructor
 */
BlockDvb::BlockDvb(const string &name):
	Block(name),
	satellite_type(),
	frame_duration_ms(),
	frames_per_superframe(-1),
	fmt_simu(),
	dvb_scenario_refresh(-1),
	emissionStd(NULL),
	receptionStd(NULL)
{
	// TODO we need a mutex here because some parameters are used in upward and downward
	this->enableChannelMutex();

	if(error_init == NULL)
	{
		error_init = Output::registerEvent("bloc_dvb:init", LEVEL_ERROR);
		event_login_received = Output::registerEvent("bloc_dvb:login_received", LEVEL_INFO);
		event_login_response = Output::registerEvent("bloc_dvb:login_response", LEVEL_INFO);
	}
}

/**
 * Destructor
 */
BlockDvb::~BlockDvb()
{
}

/** brief Read the common configuration parameters
 *
 * @return true on success, false otherwise
 */
bool BlockDvb::initCommon()
{
	const char *FUNCNAME = DBG_PREFIX "[initCommon]";

	string encap_name;
	int encap_nbr;
	EncapPlugin *plugin;
	string sat_type;

	// satellite type
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          sat_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_INFO("satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	// get the packet types
	if(!globalConfig.getNbListItems(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          UP_RETURN_ENCAP_SCHEME_LIST);
		goto error;
	}


	// get all the encapsulation to use from lower to upper
	if(!globalConfig.getValueInList(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME_LIST,
	                                POSITION, toString(encap_nbr - 1),
	                                ENCAP_NAME, encap_name))
	{
		UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
		          FUNCNAME, GLOBAL_SECTION, encap_nbr - 1, POSITION);
		goto error;
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		UTI_ERROR("%s cannot get plugin for %s encapsulation",
		          FUNCNAME, encap_name.c_str());
		goto error;
	}

	this->up_return_pkt_hdl = plugin->getPacketHandler();
	if(!this->up_return_pkt_hdl)
	{
		UTI_ERROR("cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	UTI_INFO("up/return encapsulation scheme = %s\n",
	         this->up_return_pkt_hdl->getName().c_str());

	if(!globalConfig.getNbListItems(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                encap_nbr))
	{
		UTI_ERROR("%s Section %s, %s missing\n", FUNCNAME, GLOBAL_SECTION,
		          DOWN_FORWARD_ENCAP_SCHEME_LIST);
		goto error;
	}

	// get all the encapsulation to use from lower to upper
	if(!globalConfig.getValueInList(GLOBAL_SECTION, DOWN_FORWARD_ENCAP_SCHEME_LIST,
	                                POSITION, toString(encap_nbr - 1),
	                                ENCAP_NAME, encap_name))
	{
		UTI_ERROR("%s Section %s, invalid value %d for parameter '%s'\n",
		          FUNCNAME, GLOBAL_SECTION, encap_nbr - 1, POSITION);
		goto error;
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		UTI_ERROR("%s missing plugin for %s encapsulation",
		          FUNCNAME, encap_name.c_str());
		goto error;
	}

	this->down_forward_pkt_hdl = plugin->getPacketHandler();
	if(!this->down_forward_pkt_hdl)
	{
		UTI_ERROR("cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	UTI_INFO("down/forward encapsulation scheme = %s\n",
	         this->down_forward_pkt_hdl->getName().c_str());

	// frame duration
	if(!globalConfig.getValue(GLOBAL_SECTION, DVB_F_DURATION,
	                          this->frame_duration_ms))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, DVB_F_DURATION);
		goto error;
	}
	UTI_INFO("frameDuration set to %d\n", this->frame_duration_ms);

	// number of frame per superframe
	if(!globalConfig.getValue(DVB_MAC_SECTION, DVB_FPF,
	                          this->frames_per_superframe))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_MAC_SECTION, DVB_FPF);
		goto error;
	}
	UTI_INFO("frames_per_superframe set to %d\n",
	         this->frames_per_superframe);

	// scenario refresh interval
	if(!globalConfig.getValue(GLOBAL_SECTION, DVB_SCENARIO_REFRESH,
	                          this->dvb_scenario_refresh))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, DVB_SCENARIO_REFRESH);
		goto error;
	}
	UTI_INFO("dvb_scenario_refresh set to %d\n", this->dvb_scenario_refresh);

	return true;
error:
	return false;
}

bool BlockDvb::initModcodFiles()
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD/DRA simulations and definitions
	if(!globalConfig.getValue(GLOBAL_SECTION, MODCOD_SIMU,
	                          modcod_simu_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, MODCOD_SIMU);
		goto error;
	}
	UTI_INFO("MODCOD simulation path set to %s\n", modcod_simu_file.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, MODCOD_DEF,
	                          modcod_def_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, MODCOD_DEF);
		goto error;
	}
	UTI_INFO("MODCOD definition path set to %s\n", modcod_def_file.c_str());


	if(access(modcod_def_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		           modcod_def_file.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod definition file = '%s'\n", modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!this->fmt_simu.setModcodDefFile(modcod_def_file))
	{
		goto error;
	}

	if(access(modcod_simu_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		           modcod_simu_file.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod simulation file = '%s'\n", modcod_simu_file.c_str());

	// set the MODCOD simulation file
	if(!this->fmt_simu.setModcodSimuFile(modcod_simu_file))
	{
		goto error;
	}

	return true;

error:
	return false;
}

// TODO one function for both modcod init
bool BlockDvb::initDraFiles()
{
	string dra_simu_file;
	string dra_def_file;

	if(!globalConfig.getValue(GLOBAL_SECTION, DRA_SIMU,
	                          dra_simu_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		           GLOBAL_SECTION, DRA_SIMU);
		goto error;
	}
	UTI_INFO("DRA simulation path set to %s\n", dra_simu_file.c_str());

	if(access(dra_simu_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		          dra_simu_file.c_str(), strerror(errno));
		goto error;
	}

	// set the DRA simulation file
	if(!this->fmt_simu.setDraSchemeSimuFile(dra_simu_file))
	{
		goto error;
	}

	if(!globalConfig.getValue(GLOBAL_SECTION, DRA_DEF,
	                          dra_def_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		           GLOBAL_SECTION, DRA_DEF);
		goto error;
	}
	UTI_INFO("DRA definition path set to %s\n", dra_def_file.c_str());

	if(access(dra_def_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		          dra_def_file.c_str(), strerror(errno));
		goto error;
	}
	// load all the DRA definitions from file
	if(!this->fmt_simu.setDraSchemeDefFile(dra_def_file))
	{
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvb::sendBursts(std::list<DvbFrame *> *complete_frames,
                          long carrier_id)
{
	std::list<DvbFrame *>::iterator frame_it;
	bool status = true;

	// send all complete DVB-RCS frames
	UTI_DEBUG_L3("send all %zu complete DVB-RCS frames...\n",
	             complete_frames->size());
	for(frame_it = complete_frames->begin();
	    frame_it != complete_frames->end();
	    ++frame_it)
	{
		DvbFrame *frame = dynamic_cast<DvbFrame *>(*frame_it);

		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(frame, carrier_id))
		{
			status = false;
			delete frame;
			continue;
		}

		// DVB frame is now sent, so delete its content
		delete frame;
		UTI_DEBUG("complete DVB frame sent to carrier %ld\n", carrier_id);
	}
	// clear complete DVB frames
	complete_frames->clear();

	return status;
}

/**
 * @brief Send message to lower layer with the given DVB frame
 *
 * @param frame       the DVB frame to put in the message
 * @param carrier_id  the carrier ID used to send the message
 * @return            true on success, false otherwise
 */
bool BlockDvb::sendDvbFrame(DvbFrame *frame, long carrier_id)
{
	unsigned char *dvb_frame;
	unsigned int dvb_length;

	if(frame->getTotalLength() <= 0)
	{
		UTI_ERROR("empty frame, header and payload are not present\n");
		goto error;
	}

	if(frame->getNumPackets() <= 0)
	{
		UTI_ERROR("empty frame, header is present but not payload\n");
		goto error;
	}

	// get memory for a DVB frame
	dvb_frame = (unsigned char *)calloc(sizeof(unsigned char),
	                                    MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX);
	if(dvb_frame == NULL)
	{
		UTI_ERROR("cannot get memory for DVB frame\n");
		goto error;
	}

	// copy the DVB frame
	dvb_length = frame->getTotalLength();
	memcpy(dvb_frame, frame->getData().c_str(), dvb_length);

	if(!this->sendDvbFrame((T_DVB_HDR *) dvb_frame, carrier_id, (long)dvb_length))
	{
		UTI_ERROR("failed to send message\n");
		goto release_dvb_frame;
	}

	UTI_DEBUG_L3("end of message sending on carrier %li\n", carrier_id);

	return true;

release_dvb_frame:
	free(dvb_frame);
error:
	return false;
}


/**
 * @brief Create a message with the given DVB frame
 *        and send it to lower layer
 *
 * @param dvb_frame     the DVB frame
 * @param carrier_id    the DVB carrier Id
 * @param l_len         XXX
 * @return              true on success, false otherwise
 */
bool BlockDvb::sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id, long l_len)
{
	T_DVB_META *dvb_meta; // encapsulates the DVB Frame in a structure

	dvb_meta = new T_DVB_META;
	dvb_meta->carrier_id = carrier_id;
	dvb_meta->hdr = dvb_frame;

	// send the message to the lower layer
	// do not count carrier_id in len, this is the dvb_meta->hdr length
	if(!this->sendDown((void **)(&dvb_meta), l_len))
	{
		UTI_ERROR("failed to send DVB frame to lower layer\n");
		return false;
	}
	UTI_DEBUG("DVB frame sent to the lower layer\n");

	return true;
}


bool BlockDvb::SendNewMsgToUpperLayer(NetBurst *burst)
{
	// send the message to the upper layer
	if(!this->sendUp((void **)&burst))
	{
		UTI_ERROR("failed to send burst of packets to upper layer\n");
		goto release_burst;
	}
	UTI_DEBUG("burst sent to the upper layer\n");

	return true;

release_burst:
	delete burst;
	return false;
}

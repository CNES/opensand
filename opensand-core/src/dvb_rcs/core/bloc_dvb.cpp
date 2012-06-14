/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file bloc_dvb.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "bloc_dvb.h"

#include <string.h>
#include <errno.h>

#include <opensand_conf/conf.h>

#include "DvbS2Std.h"
#include "EncapPlugin.h"

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include <opensand_conf/uti_debug.h>


/**
 * Constructor
 */
BlocDvb::BlocDvb(mgl_blocmgr *blocmgr,
                 mgl_id fatherid,
                 const char *name,
                 std::map<std::string, EncapPlugin *> &encap_plug):
	mgl_bloc(blocmgr, fatherid, name),
	encap_plug(encap_plug)
{
	this->satellite_type = "";
	this->dama_algo = "";
	this->frame_duration = -1;
	this->frames_per_superframe = -1;
	this->modcod_def = "";
	this->modcod_simu = "";
	this->dra_def = "";
	this->dra_simu = "";
	this->dvb_scenario_refresh = -1;
}

/**
 * Destructor
 */
BlocDvb::~BlocDvb()
{
}

/** brief Read the common configuration parameters
 *
 * @return true on success, false otherwise
 */
bool BlocDvb::initCommon()
{
	const char *FUNCNAME = DBG_PREFIX "[initCommon]";

	string encap_name;
	int encap_nbr;

	// satellite type
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          this->satellite_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_INFO("satellite type = %s\n", this->satellite_type.c_str());

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

	if(this->encap_plug[encap_name] == NULL)
	{
		UTI_ERROR("%s missing plugin for %s encapsulation",
		          FUNCNAME, encap_name.c_str());
		goto error;
	}

	this->up_return_pkt_hdl = this->encap_plug[encap_name]->getPacketHandler();
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

	if(this->encap_plug[encap_name] == NULL)
	{
		UTI_ERROR("%s missing plugin for %s encapsulation",
		          FUNCNAME, encap_name.c_str());
		goto error;
	}

	this->down_forward_pkt_hdl = this->encap_plug[encap_name]->getPacketHandler();
	if(!this->down_forward_pkt_hdl)
	{
		UTI_ERROR("cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	UTI_INFO("down/forward encapsulation scheme = %s\n",
	         this->down_forward_pkt_hdl->getName().c_str());

	// dama algorithm
	if(!globalConfig.getValue(DVB_GLOBAL_SECTION, DVB_NCC_DAMA_ALGO,
	                          this->dama_algo))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	// frame duration
	if(!globalConfig.getValue(GLOBAL_SECTION, DVB_F_DURATION,
	                          this->frame_duration))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, DVB_F_DURATION);
		goto error;
	}
	UTI_INFO("frameDuration set to %d\n", this->frame_duration);

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

	// MODCOD/DRA simulations and definitions
	if(!globalConfig.getValue(GLOBAL_SECTION, MODCOD_SIMU,
	                          this->modcod_simu))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, MODCOD_SIMU);
		goto error;
	}
	UTI_INFO("MODCOD simulation path set to %s\n", this->modcod_simu.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, MODCOD_DEF,
	                          this->modcod_def))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, MODCOD_DEF);
		goto error;
	}
	UTI_INFO("MODCOD definition path set to %s\n", this->modcod_def.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, DRA_SIMU,
	                          this->dra_simu))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		           GLOBAL_SECTION, DRA_SIMU);
		goto error;
	}
	UTI_INFO("DRA simulation path set to %s\n", this->dra_simu.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, DRA_DEF,
	                          this->dra_def))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		           GLOBAL_SECTION, DRA_DEF);
		goto error;
	}
	UTI_INFO("DRA definition path set to %s\n", this->dra_def.c_str());

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


/**
 * @brief Read configuration for the MODCOD definition/simulation files
 *
 * Always run this function after initEncap !
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDvb::initModcodFiles()
{
	int bandwidth;

	if(access(this->modcod_def.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		           this->modcod_def.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod definition file = '%s'\n", this->modcod_def.c_str());

	// load all the MODCOD definitions from file
	if(!dynamic_cast<DvbS2Std *>
			(this->emissionStd)->loadModcodDefinitionFile(this->modcod_def))
	{
		goto error;
	}

	if(access(this->modcod_simu.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
		           this->modcod_simu.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod simulation file = '%s'\n", this->modcod_simu.c_str());

	// associate the simulation file with the list of STs
	if(!dynamic_cast<DvbS2Std *>
			(this->emissionStd)->loadModcodSimulationFile(this->modcod_simu))
	{
		goto error;
	}

	// Get the value of the bandwidth
	if(!globalConfig.getValue(GLOBAL_SECTION, BANDWIDTH,
	                          bandwidth))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, BANDWIDTH);
		goto error;
	}

	// Set the bandwidth value for DVB-S2 emission standard
	if(this->emissionStd->type() == "DVB-S2")
	{
		this->emissionStd->setBandwidth(bandwidth);
	}

	return 0;

error:
	return -1;
}


/**
 * Send the complete DVB frames created
 * by ef DvbRcsStd::scheduleEncapPackets or
 * \ ref DvbRcsDamaAgent::globalSchedule for Terminal
 *
 * @param complete_frames the list of complete DVB frames
 * @param carrier_id      the ID of the carrier where to send the frames
 * @return 0 if successful, -1 otherwise
 */
int BlocDvb::sendBursts(std::list<DvbFrame *> *complete_frames,
                        long carrier_id)
{
	std::list<DvbFrame *>::iterator frame;
	int retval = 0;

	// send all complete DVB-RCS frames
	UTI_DEBUG_L3("send all %u complete DVB-RCS frames...\n",
	             complete_frames->size());
	for(frame = complete_frames->begin();
	    frame != complete_frames->end();
	    frame = complete_frames->erase(frame))
	{
		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(dynamic_cast<DvbFrame *>(*frame), carrier_id))
		{
			retval = -1;
			continue;
		}

		// DVB frame is now sent, so delete its content
		// and remove it from the list (done in the for() statement)
		delete (*frame);
		UTI_DEBUG("complete DVB frame sent to carrier %ld\n", carrier_id);
	}

	return retval;
}

/**
 * @brief Send message to lower layer with the given DVB frame
 *
 * @param frame       the DVB frame to put in the message
 * @param carrier_id  the carrier ID used to send the message
 * @return            true on success, false otherwise
 */
bool BlocDvb::sendDvbFrame(DvbFrame *frame, long carrier_id)
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
	dvb_frame = (unsigned char *) g_memory_pool_dvb_rcs.get(HERE());
	if(dvb_frame == NULL)
	{
		UTI_ERROR("cannot get memory for DVB frame\n");
		goto error;
	}

	// copy the DVB frame
	dvb_length = frame->getTotalLength();
	memcpy(dvb_frame, frame->getData().c_str(), dvb_length);

	if (!this->sendDvbFrame((T_DVB_HDR *) dvb_frame, carrier_id))
	{
		UTI_ERROR("failed to send message\n");
		goto release_dvb_frame;
	}

	UTI_DEBUG("end of message sending\n");

	return true;

release_dvb_frame:
	g_memory_pool_dvb_rcs.release((char *) dvb_frame);
error:
	return false;
}


/**
 * @brief Create a margouilla message with the given DVB frame
 *        and send it to lower layer
 *
 * @param dvb_frame     the DVB frame
 * @param carrier_id    the DVB carrier Id
 * @return              true on success, false otherwise
 */
bool BlocDvb::sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id)
{
	T_DVB_META *dvb_meta; // encapsulates the DVB Frame in a structure
	mgl_msg *msg; // Margouilla message to send to lower layer

	dvb_meta = (T_DVB_META *) g_memory_pool_dvb_rcs.get(HERE());
	dvb_meta->carrier_id = carrier_id;
	dvb_meta->hdr = dvb_frame;

	// create the Margouilla message with burst as data
	msg = this->newMsgWithBodyPtr(msg_dvb,
	                              dvb_meta, dvb_frame->msg_length);
	if(msg == NULL)
	{
		UTI_ERROR("failed to create message to send DVB frame, drop the frame\n");
		return false;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) < 0)
	{
		UTI_ERROR("failed to send DVB frame to lower layer\n");
		return false;
	}
	UTI_DEBUG("DVB frame sent to the lower layer\n");

	return true;
}


/**
 * @brief Create a margouilla message with the given burst
 *        and sned it to upper layer
 *
 * @param burst the burst of encapsulated packets
 * @return      0 on success, -1 on error
 */

int BlocDvb::SendNewMsgToUpperLayer(NetBurst *burst)
{
	mgl_msg *msg; // Margouilla message to send to upper layer

	// create the Margouilla message with burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst,
	                              burst, sizeof(burst));
	if(msg == NULL)
	{
		UTI_ERROR("failed to create message to send burst, drop the burst\n");
		goto release_burst;
	}

	// send the message to the upper layer
	if(this->sendMsgTo(this->getUpperLayer(), msg) < 0)
	{
		UTI_ERROR("failed to send burst of packets to upper layer\n");
		goto release_burst;
	}
	UTI_DEBUG("burst sent to the upper layer\n");

	return 0;

release_burst:
	delete burst;
	return -1;
}

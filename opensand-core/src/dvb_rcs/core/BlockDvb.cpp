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
	super_frame_counter(0),
	frame_counter(0),
	fmt_simu(),
	dvb_scenario_refresh(-1),
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
	UTI_INFO("frame duration set to %d\n", this->frame_duration_ms);

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

bool BlockDvb::initForwardModcodFiles()
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD simulations and definitions for down/forward link
	if(!globalConfig.getValue(GLOBAL_SECTION, DOWN_FORWARD_MODCOD_SIMU,
	                          modcod_simu_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, DOWN_FORWARD_MODCOD_SIMU);
		goto error;
	}
	UTI_INFO("down/forward link MODCOD simulation path set to %s\n",
	         modcod_simu_file.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, DOWN_FORWARD_MODCOD_DEF,
	                          modcod_def_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, DOWN_FORWARD_MODCOD_DEF);
		goto error;
	}
	UTI_INFO("up/return link MODCOD definition path set to %s\n",
	         modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!this->fmt_simu.setForwardModcodDef(modcod_def_file))
	{
		goto error;
	}

	// set the MODCOD simulation file
	if(!this->fmt_simu.setForwardModcodSimu(modcod_simu_file))
	{
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockDvb::initReturnModcodFiles()
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD simulations and definitions for up/return link
	if(!globalConfig.getValue(GLOBAL_SECTION, UP_RETURN_MODCOD_SIMU,
	                          modcod_simu_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, UP_RETURN_MODCOD_SIMU);
		goto error;
	}
	UTI_INFO("up/return link MODCOD simulation path set to %s\n",
	         modcod_simu_file.c_str());

	if(!globalConfig.getValue(GLOBAL_SECTION, UP_RETURN_MODCOD_DEF,
	                          modcod_def_file))
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, UP_RETURN_MODCOD_DEF);
		goto error;
	}
	UTI_INFO("up/return link MODCOD definition path set to %s\n",
	          modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!this->fmt_simu.setReturnModcodDef(modcod_def_file))
	{
		goto error;
	}

	// set the MODCOD simulation file
	if(!this->fmt_simu.setReturnModcodSimu(modcod_simu_file))
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
			if(frame)
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

	if(!frame)
	{
		UTI_ERROR("frame is %p\n", frame);
		goto error;
	}

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
 * @param l_len         the DVB frame length
 * @return              true on success, false otherwise
 */
bool BlockDvb::sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id, long l_len)
{
	T_DVB_META *dvb_meta; // encapsulates the DVB Frame in a structure

	if(!dvb_frame)
	{
		UTI_ERROR("frame is %p\n", dvb_frame);
		return false;
	}

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


bool BlockDvb::onRcvEncapPacket(NetPacket *packet,
                                DvbFifo *fifo,
                                int fifo_delay)
{

	MacFifoElement *elem;
	long current_time = this->getCurrentTime();

	// create a new satellite cell to store the packet
	elem = new MacFifoElement(packet, current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		UTI_ERROR("cannot allocate FIFO element, drop packet\n");
		goto error;
	}

	// append the new satellite cell in the ST FIFO of the appropriate
	// satellite spot
	if(!fifo->push(elem))
	{
		UTI_ERROR("FIFO is full: drop packet\n");
		goto release_elem;
	}

	UTI_DEBUG("encapsulation packet %s stored in FIFO "
	          "(tick_in = %ld, tick_out = %ld)\n",
	          packet->getName().c_str(),
	          elem->getTickIn(), elem->getTickOut());

	return true;

release_elem:
	delete elem;
error:
	delete packet;
	return false;
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

bool BlockDvb::initBand(const char *band,
                        TerminalCategories &categories,
                        TerminalMapping &terminal_affectation,
                        TerminalCategory **default_category,
                        fmt_groups_t &fmt_groups)
{
	freq_khz_t bandwidth_khz;
	double roll_off;
	freq_mhz_t bandwidth_mhz = 0;
	ConfigurationList conf_list;
	ConfigurationList aff_list;
	TerminalCategories::iterator cat_iter;
	unsigned int carrier_id = 0;
	int i;
	string default_category_name;

	// Get the value of the bandwidth for return link
	if(!globalConfig.getValue(band, BANDWIDTH,
	                          bandwidth_mhz))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          band, BANDWIDTH);
		goto error;
	}
	bandwidth_khz = bandwidth_mhz * 1000;
	UTI_DEBUG("%s: bandwitdh is %u kHz\n", band, bandwidth_khz);

	// Get the value of the roll off
	if(!globalConfig.getValue(band, ROLL_OFF,
	                          roll_off))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          band, ROLL_OFF);
		goto error;
	}

	// get the FMT groups
	if(!globalConfig.getListItems(band,
	                              FMT_GROUP_LIST,
	                              conf_list))
	{
		UTI_ERROR("Section %s, %s missing\n",
		          band, FMT_GROUP_LIST);
		goto error;
	}

	// create group list
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		unsigned int group_id;
		string fmt_id;
		FmtGroup *group;

		// Get group id name
		if(!globalConfig.getAttributeValue(iter, GROUP_ID, group_id))
		{
			UTI_ERROR("Section %s, problem retrieving %s in FMT groups\n",
			          band, GROUP_ID);
			goto error;
		}

		// Get FMT IDs
		if(!globalConfig.getAttributeValue(iter, FMT_ID, fmt_id))
		{
			UTI_ERROR("Section %s, problem retrieving %s in FMT groups\n",
			          band, FMT_ID);
			goto error;
		}

		if(fmt_groups.find(group_id) != fmt_groups.end())
		{
			UTI_ERROR("Section %s, duplicated FMT group %u\n", band, group_id);
			goto error;
		}
		group = new FmtGroup(group_id, fmt_id);
		fmt_groups[group_id] = group;
	}

	conf_list.clear();
	// get the carriers distribution
	if(!globalConfig.getListItems(band, CARRIERS_DISTRI_LIST, conf_list))
	{
		UTI_ERROR("Section %s, %s missing\n", band, CARRIERS_DISTRI_LIST);
		goto error;
	}

	i = 0;
	carrier_id = 0;
	// create terminal categories according to channel distribution
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		string name;
		double ratio;
		rate_symps_t symbol_rate_symps;
		unsigned int group_id;
		string access_type;
		TerminalCategory *category;
		fmt_groups_t::const_iterator group_it;

		i++;

		// Get carriers' name
		if(!globalConfig.getAttributeValue(iter, CATEGORY, name))
		{
			UTI_ERROR("Section %s, problem retrieving %s in carriers distribution table "
			          "entry %u\n", band, CATEGORY, i);
			goto error;
		}

		// Get carriers' ratio
		if(!globalConfig.getAttributeValue(iter, RATIO, ratio))
		{
			UTI_ERROR("Section %s, problem retrieving %s in carriers distribution table "
			          "entry %u\n", band, RATIO, i);
			goto error;
		}

		// Get carriers' symbol ratge
		if(!globalConfig.getAttributeValue(iter, SYMBOL_RATE, symbol_rate_symps))
		{
			UTI_ERROR("Section %s, problem retrieving %s in carriers distribution table "
			          "entry %u\n", band, SYMBOL_RATE, i);
			goto error;
		}

		// Get carriers' FMT id
		if(!globalConfig.getAttributeValue(iter, FMT_GROUP, group_id))
		{
			UTI_ERROR("Section %s, problem retrieving %s in carriers distribution table "
			          "entry %u\n", band, FMT_GROUP, i);
			goto error;
		}

		// Get carriers' access type
		if(!globalConfig.getAttributeValue(iter, ACCESS_TYPE, access_type))
		{
			UTI_ERROR("Section %s, problem retrieving %s in carriers distribution table "
			          "entry %u\n", band, ACCESS_TYPE, i);
			goto error;
		}

		UTI_INFO("%s: new carriers: category=%s, Rs=%G, FMT group=%u, "
		         "ratio=%G, access type=%s\n", band, name.c_str(),
		         symbol_rate_symps, group_id, ratio, access_type.c_str());
		if((!strcmp(band, UP_RETURN_BAND) && access_type != "DAMA") ||
		   (!strcmp(band, DOWN_FORWARD_BAND) && access_type != "TDM"))
		{
			UTI_ERROR("%s access type is not supported\n", access_type.c_str());
			goto error;
		}

		group_it = fmt_groups.find(group_id);
		if(group_it == fmt_groups.end())
		{
			UTI_ERROR("Section %s, nentry for FMT group with ID %u\n", band, group_id);
			goto error;
		}

		// create the category if it does not exist
		cat_iter = categories.find(name);
		category = (*cat_iter).second;
		if(cat_iter == categories.end())
		{
			category = new TerminalCategory(name);
			categories[name] = category;
		}
		category->addCarriersGroup(carrier_id, (*group_it).second, ratio,
		                           symbol_rate_symps);
		carrier_id++;
	}

	// get the default terminal category
	if(!globalConfig.getValue(band, DEFAULT_AFF,
	                          default_category_name))
	{
		UTI_ERROR("Section %s, missing %s parameter\n", band, DEFAULT_AFF);
		goto error;
	}

	// Look for associated category
	*default_category = NULL;
	cat_iter = categories.find(default_category_name);
	if(cat_iter != categories.end())
	{
		*default_category = (*cat_iter).second;
	}
	if(*default_category == NULL)
	{
		UTI_ERROR("Section %s, could not find categorie %s\n",
		          band, default_category_name.c_str());
		goto error;
	}
	UTI_INFO("ST default category: %s in %s\n",
	         (*default_category)->getLabel().c_str(), band);

	// get the terminal affectations
	if(!globalConfig.getListItems(band, TAL_AFF_LIST, aff_list))
	{
		UTI_INFO("Section %s, missing %s parameter\n", band, TAL_AFF_LIST);
		goto error;
	}

	i = 0;
	for(ConfigurationList::iterator iter = aff_list.begin();
	    iter != aff_list.end(); ++iter)
	{
		// To prevent compilator to issue warning about non initialised variable
		tal_id_t tal_id = -1;
		string name;
		TerminalCategory *category;

		i++;
		if(!globalConfig.getAttributeValue(iter, TAL_ID, tal_id))
		{
			UTI_ERROR("Section %s, problem retrieving %s in terminal affection table"
			          "entry %u\n", band, TAL_ID, i);
			goto error;
		}
		if(!globalConfig.getAttributeValue(iter, CATEGORY, name))
		{
			UTI_ERROR("Section %s, problem retrieving %s in terminal affection table"
			          "entry %u\n", band, CATEGORY, i);
			goto error;
		}

		// Look for the category
		category = NULL;
		cat_iter = categories.find(name);
		if(cat_iter != categories.end())
		{
			category = (*cat_iter).second;
		}
		if(category == NULL)
		{
			UTI_ERROR("Section %s, could not find category %s", band, name.c_str());
			goto error;
		}

		terminal_affectation[tal_id] = category;
		UTI_DEBUG("%s: terminal %u will be affected to category %s\n",
		          band, tal_id, name.c_str());
	}

	// Compute bandplan
	if(!this->computeBandplan(bandwidth_khz, roll_off, categories))
	{
		UTI_ERROR("Cannot compute band plan for %s\n", band);
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvb::computeBandplan(freq_khz_t available_bandplan_khz,
                               double roll_off,
                               TerminalCategories &categories)
{
	TerminalCategories::const_iterator category_it;

	double weighted_sum_ksymps = 0.0;

	// compute weighted sum
	for(category_it = categories.begin();
	    category_it != categories.end();
	    ++category_it)
	{
		TerminalCategory *category = (*category_it).second;

		// Compute weighted sum in ks/s since available bandplan is in kHz.
		weighted_sum_ksymps += category->getWeightedSum();
	}

	UTI_DEBUG_L3("Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if(equals(weighted_sum_ksymps, 0.0))
	{
		UTI_ERROR("Weighted ratio sum is 0\n");
		goto error;
	}

	// compute carrier number per category
	for(category_it = categories.begin();
	    category_it != categories.end();
		category_it++)
	{
		unsigned int carriers_number = 0;
		TerminalCategory *category = (*category_it).second;
		unsigned int ratio = category->getRatio();

		carriers_number = ceil(
		    (ratio / weighted_sum_ksymps) *
		    (available_bandplan_khz / (1 + roll_off)));
		UTI_INFO("Number of carriers for category %s: %d\n",
		         category->getLabel().c_str(), carriers_number);

		// set the carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number,
		                               this->frame_duration_ms *
		                               this->frames_per_superframe);
	}

	return true;
error:
	return false;
}



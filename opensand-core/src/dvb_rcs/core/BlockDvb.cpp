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


#include "BlockDvb.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>


bool DvbChannel::initSatType(void)
{
	string sat_type;

	// satellite type
	if(!Conf::getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                   sat_type))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, SATELLITE_TYPE);
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	return true;
}


bool DvbChannel::initPktHdl(const char *encap_schemes,
                            EncapPlugin::EncapPacketHandler **pkt_hdl)
{
	string encap_name;
	int encap_nbr;
	EncapPlugin *plugin;

	// get the packet types
	if(!Conf::getNbListItems(GLOBAL_SECTION, encap_schemes,
	                         encap_nbr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    GLOBAL_SECTION, encap_schemes);
		goto error;
	}

	// get all the encapsulation to use from lower to upper
	if(!Conf::getValueInList(GLOBAL_SECTION, encap_schemes,
	                         POSITION, toString(encap_nbr - 1),
	                                ENCAP_NAME, encap_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, invalid value %d for parameter '%s'\n",
		    GLOBAL_SECTION, encap_nbr - 1, POSITION);
		goto error;
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get plugin for %s encapsulation",
		    encap_name.c_str());
		goto error;
	}

	*pkt_hdl = plugin->getPacketHandler();
	if(!pkt_hdl)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "encapsulation scheme = %s\n",
	    (*pkt_hdl)->getName().c_str());

	return true;
error:
	return false;
}


bool DvbChannel::initCommon(const char *encap_schemes)
{
	if(!this->initSatType())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize satellite type\n");
		goto error;
	}

	// Retrieve the value of the ‘enable’ parameter for the physical layer
	if(!Conf::getValue(PHYSICAL_LAYER_SECTION, ENABLE,
	                   this->with_phy_layer))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, ENABLE);
		goto error;
	}

	if(!this->initPktHdl(encap_schemes, &this->pkt_hdl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize packet handler\n");
		goto error;
	}

	// statistics timer
	if(!Conf::getValue(PERF_SECTION, STATS_TIMER,
	                   this->stats_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    PERF_SECTION, STATS_TIMER);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "statistics_timer set to %d\n",
	    this->stats_period_ms);

	return true;
error:
	return false;
}




BlockDvb::~BlockDvb()
{
}


BlockDvb::DvbUpward::~DvbUpward()
{
	// release the reception DVB standards
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}
}


bool BlockDvb::DvbDownward::initDown(void)
{
	// frame duration
	if(!Conf::getValue(GLOBAL_SECTION, DVB_F_DURATION,
	                   this->frame_duration_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, DVB_F_DURATION);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "frame duration set to %d\n", this->frame_duration_ms);


	// forward timer
	if(!Conf::getValue(PERF_SECTION, FWD_TIMER,
	                   this->fwd_timer_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    PERF_SECTION, FWD_TIMER);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "forward timer set to %u\n",
	    this->fwd_timer_ms);

	// number of frame per superframe
	if(!Conf::getValue(DVB_MAC_SECTION, DVB_FPF,
	                   this->frames_per_superframe))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_MAC_SECTION, DVB_FPF);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "frames_per_superframe set to %d\n",
	    this->frames_per_superframe);

	// scenario refresh interval
	if(!Conf::getValue(GLOBAL_SECTION, DVB_SCENARIO_REFRESH,
	                   this->dvb_scenario_refresh))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    GLOBAL_SECTION, DVB_SCENARIO_REFRESH);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "dvb_scenario_refresh set to %d\n",
	    this->dvb_scenario_refresh);

	return true;
error:
	return false;
}


bool BlockDvb::DvbDownward::initModcodFiles(const char *def, const char *simu)
{
	return this->initModcodFiles(def, simu, this->fmt_simu);
}


bool BlockDvb::DvbDownward::initModcodFiles(const char *def,
                                            const char *simu,
                                            FmtSimulation &fmt_simu)
{
	string modcod_simu_file;
	string modcod_def_file;

	// MODCOD simulations and definitions for down/forward link
	if(!Conf::getValue(GLOBAL_SECTION, simu,
	                   modcod_simu_file))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    GLOBAL_SECTION, simu);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "down/forward link MODCOD simulation path set to %s\n",
	    modcod_simu_file.c_str());

	if(!Conf::getValue(GLOBAL_SECTION, def,
	                   modcod_def_file))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    GLOBAL_SECTION, def);
		goto error;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "down/forward link MODCOD definition path set to %s\n",
	    modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!fmt_simu.setModcodDef(modcod_def_file))
	{
		goto error;
	}

	// no need for simulation file if there is a physical layer
	if(!this->with_phy_layer)
	{
		// set the MODCOD simulation file
		if(!fmt_simu.setModcodSimu(modcod_simu_file))
		{
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvb::DvbDownward::sendBursts(list<DvbFrame *> *complete_frames,
                                       uint8_t carrier_id)
{
	list<DvbFrame *>::iterator frame_it;
	bool status = true;

	// send all complete DVB-RCS frames
	LOG(this->log_send, LEVEL_DEBUG,
	    "send all %zu complete DVB frames...\n",
	    complete_frames->size());
	for(frame_it = complete_frames->begin();
	    frame_it != complete_frames->end();
	    ++frame_it)
	{
		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(*frame_it, carrier_id))
		{
			status = false;
			if(*frame_it)
			{
				delete *frame_it;
			}
			continue;
		}

		// DVB frame is now sent, so delete its content
		LOG(this->log_send, LEVEL_INFO,
		    "complete DVB frame sent to carrier %u\n", carrier_id);
	}
	// clear complete DVB frames
	complete_frames->clear();

	return status;
}

bool BlockDvb::DvbDownward::sendDvbFrame(DvbFrame *dvb_frame, uint8_t carrier_id)
{
	if(!dvb_frame)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "frame is %p\n", dvb_frame);
		goto error;
	}

	dvb_frame->setCarrierId(carrier_id);

	if(dvb_frame->getTotalLength() <= 0)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "empty frame, header and payload are not present\n");
		goto error;
	}

	// send the message to the lower layer
	// do not count carrier_id in len, this is the dvb_meta->hdr length
	if(!this->enqueueMessage((void **)(&dvb_frame)))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "failed to send DVB frame to lower layer\n");
		goto release_dvb_frame;
	}
	// TODO make a log_send_frame and a log_send_sig
	LOG(this->log_send, LEVEL_INFO,
	    "DVB frame sent to the lower layer\n");

	return true;

release_dvb_frame:
	delete dvb_frame;
error:
	return false;
}


bool BlockDvb::DvbDownward::onRcvEncapPacket(NetPacket *packet,
                                             DvbFifo *fifo,
                                             time_ms_t fifo_delay)
{

	MacFifoElement *elem;
	clock_t current_time = this->getCurrentTime();

	// create a new satellite cell to store the packet
	elem = new MacFifoElement(packet, current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "cannot allocate FIFO element, drop packet\n");
		goto error;
	}

	// append the new packet in the fifo
	if(!fifo->push(elem))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "FIFO is full: drop packet\n");
		goto release_elem;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "encapsulation packet %s stored in FIFO "
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


bool BlockDvb::DvbDownward::initBand(const char *band,
	                        time_ms_t duration_ms,
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

	// create the log here as it may not be used by all blocks
	this->log_band = Output::registerLog(LEVEL_WARNING, "Dvb.Ncc.Band");

	// Get the value of the bandwidth for return link
	if(!Conf::getValue(band, BANDWIDTH,
	                   bandwidth_mhz))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    band, BANDWIDTH);
		goto error;
	}
	bandwidth_khz = bandwidth_mhz * 1000;
	LOG(this->log_band, LEVEL_INFO,
	    "%s: bandwitdh is %u kHz\n", band, bandwidth_khz);

	// Get the value of the roll off
	if(!Conf::getValue(band, ROLL_OFF,
	                   roll_off))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    band, ROLL_OFF);
		goto error;
	}

	// get the FMT groups
	if(!Conf::getListItems(band,
	                       FMT_GROUP_LIST,
	                       conf_list))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "Section %s, %s missing\n",
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
		if(!Conf::getAttributeValue(iter, GROUP_ID, group_id))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in FMT "
			    "groups\n", band, GROUP_ID);
			goto error;
		}

		// Get FMT IDs
		if(!Conf::getAttributeValue(iter, FMT_ID, fmt_id))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in FMT "
			    "groups\n", band, FMT_ID);
			goto error;
		}

		if(fmt_groups.find(group_id) != fmt_groups.end())
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, duplicated FMT group %u\n", band,
			    group_id);
			goto error;
		}
		group = new FmtGroup(group_id, fmt_id);
		fmt_groups[group_id] = group;
	}

	conf_list.clear();
	// get the carriers distribution
	if(!Conf::getListItems(band, CARRIERS_DISTRI_LIST, conf_list))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "Section %s, %s missing\n", band,
		    CARRIERS_DISTRI_LIST);
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
		if(!Conf::getAttributeValue(iter, CATEGORY, name))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    CATEGORY, i);
			goto error;
		}

		// Get carriers' ratio
		if(!Conf::getAttributeValue(iter, RATIO, ratio))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band, RATIO, i);
			goto error;
		}

		// Get carriers' symbol ratge
		if(!Conf::getAttributeValue(iter, SYMBOL_RATE, symbol_rate_symps))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    SYMBOL_RATE, i);
			goto error;
		}

		// Get carriers' FMT id
		if(!Conf::getAttributeValue(iter, FMT_GROUP, group_id))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    FMT_GROUP, i);
			goto error;
		}

		// Get carriers' access type
		if(!Conf::getAttributeValue(iter, ACCESS_TYPE, access_type))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", band,
			    ACCESS_TYPE, i);
			goto error;
		}

		LOG(this->log_band, LEVEL_NOTICE,
		    "%s: new carriers: category=%s, Rs=%G, FMT group=%u, "
		    "ratio=%G, access type=%s\n", band, name.c_str(),
		    symbol_rate_symps, group_id, ratio,
		    access_type.c_str());
		if((!strcmp(band, UP_RETURN_BAND) && access_type != "DAMA") ||
		   (!strcmp(band, DOWN_FORWARD_BAND) && access_type != "TDM"))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "%s access type is not supported\n",
			    access_type.c_str());
			goto error;
		}

		group_it = fmt_groups.find(group_id);
		if(group_it == fmt_groups.end())
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, nentry for FMT group with ID %u\n",
			    band, group_id);
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
	if(!Conf::getValue(band, DEFAULT_AFF,
	                   default_category_name))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "Section %s, missing %s parameter\n", band,
		    DEFAULT_AFF);
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
		LOG(this->log_band, LEVEL_ERROR,
		    "Section %s, could not find categorie %s\n",
		    band, default_category_name.c_str());
		goto error;
	}
	LOG(this->log_band, LEVEL_NOTICE,
	    "ST default category: %s in %s\n",
	    (*default_category)->getLabel().c_str(), band);

	// get the terminal affectations
	if(!Conf::getListItems(band, TAL_AFF_LIST, aff_list))
	{
		LOG(this->log_band, LEVEL_NOTICE,
		    "Section %s, missing %s parameter\n", band,
		    TAL_AFF_LIST);
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
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in terminal "
			    "affection table entry %u\n", band, TAL_ID, i);
			goto error;
		}
		if(!Conf::getAttributeValue(iter, CATEGORY, name))
		{
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in terminal "
			    "affection table entry %u\n", band, CATEGORY, i);
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
			LOG(this->log_band, LEVEL_ERROR,
			    "Section %s, could not find category %s", band,
			    name.c_str());
			goto error;
		}

		terminal_affectation[tal_id] = category;
		LOG(this->log_band, LEVEL_INFO,
		    "%s: terminal %u will be affected to category %s\n",
		    band, tal_id, name.c_str());
	}

	// Compute bandplan
	if(!this->computeBandplan(bandwidth_khz, roll_off, duration_ms, categories))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "Cannot compute band plan for %s\n", band);
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvb::DvbDownward::computeBandplan(freq_khz_t available_bandplan_khz,
                                            double roll_off,
                                            time_ms_t duration_ms,
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

	LOG(this->log_band, LEVEL_DEBUG,
	    "Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if(equals(weighted_sum_ksymps, 0.0))
	{
		LOG(this->log_band, LEVEL_ERROR,
		    "Weighted ratio sum is 0\n");
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
		LOG(this->log_band, LEVEL_NOTICE,
		    "Number of carriers for category %s: %d\n",
		    category->getLabel().c_str(), carriers_number);

		// set the carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number,
		                               duration_ms);
	}

	return true;
error:
	return false;
}



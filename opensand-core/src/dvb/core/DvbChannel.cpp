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
 * @file DvbChannel.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "DvbChannel.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"

#include <unistd.h>
#include <errno.h>

OutputLog *DvbChannel::dvb_fifo_log = NULL;

/**
 * @brief Check if a file exists
 *
 * @return true if the file is found, false otherwise
 */
inline bool fileExists(const string &filename)
{
	if(access(filename.c_str(), R_OK) < 0)
	{
		DFLTLOG(LEVEL_ERROR,
		        "cannot access '%s' file (%s)\n",
		        filename.c_str(), strerror(errno));
		return false;
	}
	return true;
}


bool DvbChannel::initSatType(void)
{
	string sat_type;

	// satellite type
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               SATELLITE_TYPE,
	                   sat_type))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, SATELLITE_TYPE);
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	return true;
}

bool DvbChannel::initModcodDefinitionTypes(void)
{
	// return link standard type
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               RETURN_LINK_STANDARD,
	                   this->return_link_std_str))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, RETURN_LINK_STANDARD);
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "return link standard type = %s\n",
	    this->return_link_std_str.c_str());
	this->return_link_std = strToReturnLinkStd(this->return_link_std_str);

	// Set the MODCOD definition type
	if(this->return_link_std == DVB_RCS2)
	{
		unsigned int dummy;
		this->modcod_def_rcs_type = MODCOD_DEF_RCS2;

		if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
			               RCS2_BURST_LENGTH,
		                   dummy))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "section '%s': missing parameter '%s'\n",
			    COMMON_SECTION, RCS2_BURST_LENGTH);
			return false;
		}
		this->req_burst_length = dummy;
	}
	else
	{
		this->modcod_def_rcs_type = MODCOD_DEF_RCS;
		this->req_burst_length = 0;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "required burst length = %d\n",
	    this->req_burst_length);

	return true;
}

bool DvbChannel::initPktHdl(const char *encap_schemes,
                            EncapPlugin::EncapPacketHandler **pkt_hdl)
{
	string encap_name;
	int encap_nbr;
	EncapPlugin *plugin;

	// get the packet types
	if(!Conf::getNbListItems(Conf::section_map[COMMON_SECTION],
							 encap_schemes,
						 encap_nbr))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Section %s, %s missing\n",
			COMMON_SECTION, encap_schemes);
		return false;
	}
	if (encap_nbr <= 0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Section %s, invalid value for %s (%d)\n",
			COMMON_SECTION, encap_schemes, encap_nbr);
		return false;
	}

	// get all the encapsulation to use from lower to upper
	if(!Conf::getValueInList(Conf::section_map[COMMON_SECTION],
							 encap_schemes,
							 POSITION, toString(encap_nbr - 1),
							 ENCAP_NAME, encap_name))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
			"Section %s, invalid value %d for parameter '%s'\n",
			COMMON_SECTION, encap_nbr - 1, POSITION);
		return false;
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get plugin for %s encapsulation\n",
		    encap_name.c_str());
		return false;
	}

	*pkt_hdl = plugin->getPacketHandler();
	if(!pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get %s packet handler\n", encap_name.c_str());
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "encapsulation scheme = %s\n",
	    (*pkt_hdl)->getName().c_str());

	return true;
}

bool DvbChannel::initScpcPktHdl(EncapPlugin::EncapPacketHandler **pkt_hdl)
{
	vector<string> encap_stack;
	string encap_name;
	EncapPlugin *plugin;

	// Get SCPC encapsulation name stack
	if (!OpenSandConf::getScpcEncapStack(this->return_link_std_str,
		                                 encap_stack) ||
		encap_stack.size() <= 0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get SCPC encapsulation names\n");
		return false;
	}
	encap_name = encap_stack.back();

	// if GSE is imposed
	// (e.g. if Tal is in SCPC mode or for receiving GSE packet in the GW)
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "New packet handler for ENCAP type = %s\n", encap_name.c_str());

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get plugin for %s encapsulation\n",
		    encap_name.c_str());
		return false;
	}

	*pkt_hdl = plugin->getPacketHandler();
	if(!pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get %s packet handler\n", encap_name.c_str());
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "encapsulation scheme = %s\n",
	    (*pkt_hdl)->getName().c_str());

	return true;
}

bool DvbChannel::initCommon(const char *encap_schemes)
{
	//********************************************************
	//         init Common values from sections
	//********************************************************
	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize satellite type\n");
		goto error;
	}

	// frame duration
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   RET_UP_CARRIER_DURATION,
	                   this->ret_up_frame_duration_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, RET_UP_CARRIER_DURATION);
		goto error;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "frame duration set to %d\n", this->ret_up_frame_duration_ms);

	if(!this->initPktHdl(encap_schemes, &this->pkt_hdl))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize packet handler\n");
		goto error;
	}

	// statistics timer
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		               STATS_TIMER,
	                   this->stats_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    COMMON_SECTION, STATS_TIMER);
		goto error;
	}

	return true;
error:
	return false;
}


void DvbChannel::initStatsTimer(time_ms_t frame_duration_ms)
{
	// convert the pediod into a number of frames here to be
	// precise when computing statistics
	this->stats_period_frame = std::max(1, (int)round((double)this->stats_period_ms /
	                                                  (double)frame_duration_ms));
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "statistics_timer set to %d, converted into %d frame(s)\n",
	    this->stats_period_ms, this->stats_period_frame);
	this->stats_period_ms = this->stats_period_frame * frame_duration_ms;
}


bool DvbChannel::pushInFifo(DvbFifo *fifo,
                            NetContainer *data,
                            time_ms_t fifo_delay)
{
	MacFifoElement *elem;
	time_ms_t current_time = getCurrentTime();

	// create a new FIFO element to store the packet
	elem = new MacFifoElement(data, current_time, current_time + fifo_delay);
	if(!elem)
	{
		LOG(DvbChannel::dvb_fifo_log, LEVEL_ERROR,
		    "cannot allocate FIFO element, drop data\n");
		goto error;
	}

	// append the data in the fifo
	if(!fifo->push(elem))
	{
		LOG(DvbChannel::dvb_fifo_log, LEVEL_ERROR,
		    "FIFO is full: drop data\n");
		goto release_elem;
	}

	LOG(DvbChannel::dvb_fifo_log, LEVEL_NOTICE,
	    "%s data stored in FIFO %s (tick_in = %ld, tick_out = %ld)\n",
	    data->getName().c_str(), fifo->getName().c_str(),
	    elem->getTickIn(), elem->getTickOut());

	return true;

release_elem:
	delete elem;
error:
	delete data;
	return false;
}


bool DvbChannel::doSendStats(void)
{
	bool res = (this->check_send_stats == 0);
	this->check_send_stats = (this->check_send_stats + 1)
	                          % this->stats_period_frame;
	return res;
}


/***** DvbFmt ****/

bool DvbFmt::initModcodDefFile(const char *def, FmtDefinitionTable **modcod_def, vol_sym_t req_burst_length)
{
	string modcod_def_file;
	*modcod_def = new FmtDefinitionTable();

	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
		               def, modcod_def_file))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, def);
		return false;
	}
	LOG(this->log_fmt, LEVEL_NOTICE,
	    "down/forward link MODCOD definition path set to %s\n",
	    modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!fileExists(modcod_def_file.c_str()))
	{
		return false;
	}
	if(!(*modcod_def)->load(modcod_def_file, req_burst_length))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "failed to load the MODCOD definitions from file "
		    "'%s'\n", modcod_def_file.c_str());
		return false;
	}
	return true;
}

bool DvbFmt::addInputTerminal(tal_id_t id,
                              const FmtDefinitionTable *const modcod_def)
{
	fmt_id_t modcod;
	
	// set less robust modcod at init
	modcod = modcod_def->getMaxId();

	this->input_sts->addTerminal(id, modcod, modcod_def);
	return true;
}


bool DvbFmt::addOutputTerminal(tal_id_t id,
                               const FmtDefinitionTable *const modcod_def)
{
	fmt_id_t modcod = modcod_def->getMaxId();
	this->output_sts->addTerminal(id, modcod, modcod_def);
	return true;
}


bool DvbFmt::delTerminal(tal_id_t st_id, StFmtSimuList *sts)
{
	return sts->delTerminal(st_id);
}


bool DvbFmt::delInputTerminal(tal_id_t id)
{
	return this->delTerminal(id, this->input_sts);
}


bool DvbFmt::delOutputTerminal(tal_id_t id)
{
	return this->delTerminal(id, this->output_sts);
}


void DvbFmt::setInputSts(StFmtSimuList *new_input_sts)
{
	this->input_sts = new_input_sts;
}


void DvbFmt::setOutputSts(StFmtSimuList *new_output_sts)
{
	this->output_sts = new_output_sts;
}


void DvbFmt::setRequiredCniInput(tal_id_t tal_id,
                                 double cni)
{
	this->input_sts->setRequiredCni(tal_id, cni);
}


void DvbFmt::setRequiredCniOutput(tal_id_t tal_id,
                                  double cni)
{
	this->output_sts->setRequiredCni(tal_id, cni);
}


uint8_t DvbFmt::getCurrentModcodIdInput(tal_id_t id) const
{
	return this->input_sts->getCurrentModcodId(id);
}


uint8_t DvbFmt::getCurrentModcodIdOutput(tal_id_t id) const
{
	return this->output_sts->getCurrentModcodId(id);
}


double DvbFmt::getRequiredCniInput(tal_id_t tal_id)
{
	return this->input_sts->getRequiredCni(tal_id);
}


double DvbFmt::getRequiredCniOutput(tal_id_t tal_id)
{
	return this->output_sts->getRequiredCni(tal_id);
}

bool DvbFmt::getCniInputHasChanged(tal_id_t tal_id)
{
	return this->input_sts->getCniHasChanged(tal_id);
}

bool DvbFmt::getCniOutputHasChanged(tal_id_t tal_id)
{
	return this->output_sts->getCniHasChanged(tal_id);
}

bool DvbFmt::setPacketExtension(EncapPlugin::EncapPacketHandler *pkt_hdl,
                                MacFifoElement *elem,
                                DvbFifo *fifo,
                                std::vector<NetPacket*> packet_list,
                                NetPacket **extension_pkt,
                                tal_id_t source,
                                tal_id_t dest,
                                string extension_name,
                                time_sf_t super_frame_counter,
                                bool is_gw)
{
	uint32_t opaque = 0;
	double cni;
	if(is_gw)
	{
		cni = this->getRequiredCniInput(dest);
		LOG(this->log_fmt, LEVEL_INFO,
		    "Add CNI extension with value %.2f dB for ST%u\n",
		    cni, dest);
	}
	else
	{
		cni = this->getRequiredCniInput(source);
		LOG(this->log_fmt, LEVEL_INFO,
		    "Add CNI extension with value %.2f dB\n", cni);
	}
	opaque = hcnton(cni);

	bool replace = false;
	NetPacket *selected_pkt = pkt_hdl->
	                  getPacketForHeaderExtensions(packet_list);
	if(selected_pkt != NULL)
	{
		LOG(this->log_fmt, LEVEL_DEBUG,
		    "SF#%d: found no-fragmented packet without extensions\n",
		    super_frame_counter);
		replace = true;
	}
	else
	{
		LOG(this->log_fmt, LEVEL_DEBUG,
		    "SF#%d: no non-fragmented or without extension packet found, "
		    "create empty packet\n", super_frame_counter);
	}

	if(!pkt_hdl->setHeaderExtensions(selected_pkt,
	                                 extension_pkt,
	                                 source,
	                                 dest,
	                                 extension_name,
	                                 &opaque))
	{
		LOG(this->log_fmt, LEVEL_DEBUG,
		    "SF#%d: cannot add header extension in packet",
		    super_frame_counter);
		return false;
	}

	if(extension_pkt == NULL)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "SF#%d: failed to create the GSE packet with "
		    "extensions\n", super_frame_counter);
		return false;
	}
	if(replace)
	{
		// And replace the packet in the FIFO
		elem->setElem(*extension_pkt);
	}
	else
	{
		MacFifoElement *new_el = new MacFifoElement(*extension_pkt, 0, 0);
		fifo->pushBack(new_el);
	}

	return true;
}

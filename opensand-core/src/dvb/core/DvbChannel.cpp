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
 * @file DvbChannel.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 */


#include "DvbChannel.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "EncapPlugin.h"

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


bool DvbChannel::initSpots(void)
{
	map<tal_id_t, spot_id_t>::iterator iter;

	if(OpenSandConf::spot_table.empty())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "The terminal map is empty");
		return false;
	}
	
	for(iter = OpenSandConf::spot_table.begin() ; iter != OpenSandConf::spot_table.end() ;
	    ++iter)
	{
		this->spots[iter->second] = NULL;
	}
	
	if(!Conf::getValue(Conf::section_map[SPOT_TABLE_SECTION], 
	                   DEFAULT_SPOT, this->default_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR, 
		    "failed to get default terminal ID\n");
		return false;
	}
	
	if(OpenSandConf::spot_table.find(this->default_spot) == OpenSandConf::spot_table.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Default spot does not exist\n");
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


bool DvbChannel::initPktHdl(const char *encap_schemes,
                            EncapPlugin::EncapPacketHandler **pkt_hdl, bool force)
{
	string encap_name;
	int encap_nbr;
	EncapPlugin *plugin;

	// if GSE is imposed
	// (e.g. if Tal is in SCPC mode or for receiving GSE packet in the GW)
	if(force)
	{
		encap_name = "GSE";
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "New packet handler for ENCAP type = %s\n", encap_name.c_str());
	}
	else
	{
		// get the packet types
		if(!Conf::getNbListItems(Conf::section_map[COMMON_SECTION],
		                         encap_schemes,
	                         encap_nbr))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Section %s, %s missing\n",
			    COMMON_SECTION, encap_schemes);
			goto error;
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
			goto error;
		}
	}

	if(!Plugin::getEncapsulationPlugin(encap_name, &plugin))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get plugin for %s encapsulation\n",
		    encap_name.c_str());
		goto error;
	}

	*pkt_hdl = plugin->getPacketHandler();
	if(!pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get %s packet handler\n", encap_name.c_str());
		goto error;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "encapsulation scheme = %s\n",
	    (*pkt_hdl)->getName().c_str());

	return true;
error:
	return false;
}

bool DvbChannel::initCommon(const char *encap_schemes)
{
	//********************************************************
	//        init spot list
	//********************************************************
	if(!this->initSpots())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize satellite type\n");
		goto error;
	}
	
	//********************************************************
	//         init Common values from sections
	//********************************************************	
	if(!this->initSatType())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize satellite type\n");
		goto error;
	}

	// Retrieve the value of the ‘enable’ parameter for the physical layer
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   ENABLE, this->with_phy_layer))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, ENABLE);
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

	if(!this->initPktHdl(encap_schemes, &this->pkt_hdl, false))
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

bool DvbChannel::initModcodDefFile(const char *def, FmtDefinitionTable &modcod_def)
{
	string modcod_def_file;

	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
		               def, modcod_def_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing parameter '%s'\n",
		    PHYSICAL_LAYER_SECTION, def);
		return false;
	}
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "down/forward link MODCOD definition path set to %s\n",
	    modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!fileExists(modcod_def_file.c_str()))
	{
		return false;
	}
	if(!modcod_def.load(modcod_def_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to load the MODCOD definitions from file "
		    "'%s'\n", modcod_def_file.c_str());
		return false;
	}
	return true;
}

bool DvbChannel::initModcodFiles(const char *simu,
                                 tal_id_t gw_id, spot_id_t spot_id)
{
	return this->initModcodFiles(simu, this->fmt_simu, gw_id, spot_id);
}


bool DvbChannel::initModcodFiles(const char *simu,
                                 FmtSimulation &fmt_simu,
                                 tal_id_t gw_id,
                                 spot_id_t spot_id)
{
	string modcod_simu_file;
	ConfigurationList current_gw;

	if(!OpenSandConf::getSpot(PHYSICAL_LAYER_SECTION,
		               spot_id, gw_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d and gw %d\n",
		    PHYSICAL_LAYER_SECTION, spot_id, gw_id);
		return false;
	}

	if(!Conf::getValue(current_gw, simu, modcod_simu_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s/spot_%d_gw_%d', missing section '%s'\n",
		    PHYSICAL_LAYER_SECTION, spot_id, gw_id, simu);
		return false;
	}

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "down/forward link MODCOD simulation path set to %s\n",
	    modcod_simu_file.c_str());

	// no need for simulation file if there is a physical layer
	if(!this->with_phy_layer)
	{
		// set the MODCOD simulation file
		if(!fmt_simu.setModcodSimu(modcod_simu_file))
		{
			return false;
		}
	}

	return true;
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


DvbChannel *DvbChannel::getSpot(spot_id_t spot_id) const
{
	map<spot_id_t, DvbChannel *>::const_iterator spot_it;

	spot_it = this->spots.find(spot_id);
	if(spot_it == this->spots.end())
	{
		// TODO log receive ?
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "spot %d does not exist\n",
		    spot_id);
		return NULL;
	}
	return (*spot_it).second;
}


bool DvbChannel::addInputTerminal(tal_id_t id, tal_id_t gw_id, spot_id_t spot_id)
{
	uint8_t modcod;
	// the column is the id
	unsigned long column = id;

	if(this->fmt_simu.getIsModcodSimuDefined() &&
	   this->fmt_simu.getModcodList().size() <= column)
	{
		LOG(this->log_init_channel, LEVEL_WARNING,
		    "cannot access MODCOD column for ST%u\n"
		    "defaut MODCOD is used\n", id);
		column = this->fmt_simu.getModcodList().size() - 1;
	}
	// if scenario are not defined, set less robust modcod at init
	// in order to authorize any MODCOD
	modcod = (this->fmt_simu.getIsModcodSimuDefined() ?
	           atoi(this->fmt_simu.getModcodList()[column].c_str()) :
	           this->input_modcod_def.getMaxId());

	DFLTLOG(LEVEL_ERROR, "TOTO %u %u %lu\n", id, this->input_modcod_def.getMaxId(), column);
		DFLTLOG(LEVEL_ERROR, "LA %s", this->fmt_simu.getModcodList()[column].c_str());

	this->input_sts->addTerminal(id, modcod, gw_id, spot_id);

	return true;
}


bool DvbChannel::addOutputTerminal(tal_id_t id, tal_id_t gw_id, spot_id_t spot_id)
{
	uint8_t modcod = this->output_modcod_def.getMaxId();
	this->output_sts->addTerminal(id, modcod, gw_id, spot_id);
	return true;
}


bool DvbChannel::delTerminal(tal_id_t st_id, StFmtSimuList* sts, tal_id_t gw_id,
                             spot_id_t spot_id)
{
	return sts->delTerminal(st_id, gw_id, spot_id);
}


bool DvbChannel::delInputTerminal(tal_id_t id, tal_id_t gw_id, spot_id_t spot_id)
{
	return this->delTerminal(id, this->input_sts, gw_id, spot_id);
}


bool DvbChannel::delOutputTerminal(tal_id_t id, tal_id_t gw_id, spot_id_t spot_id)
{
	return this->delTerminal(id, this->output_sts, gw_id, spot_id);
}


bool DvbChannel::goNextScenarioStepInput(double &duration, tal_id_t gw_id,
                                         spot_id_t spot_id)
{
	if(!this->fmt_simu.goNextScenarioStep(duration))
	{
		return false;
	}

	this->input_sts->updateModcod(gw_id, spot_id, &this->fmt_simu);

	return true;
}


void DvbChannel::setInputSts(StFmtSimuList* new_input_sts)
{
	this->input_sts = new_input_sts;
}


void DvbChannel::setOutputSts(StFmtSimuList* new_output_sts)
{
	this->output_sts = new_output_sts;
}


void DvbChannel::setRequiredModcod(tal_id_t id, double cni,
                                   FmtDefinitionTable modcod_def,
                                   StFmtSimuList* sts)
{
	uint8_t modcod_id;

	modcod_id = modcod_def.getRequiredModcod(cni);
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "Terminal %u required %.2f dB, will receive allocation "
	    "with MODCOD %u\n", id, cni, modcod_id);
	sts->setRequiredModcod(id, modcod_id);
}


void DvbChannel::setRequiredModcodInput(tal_id_t id, double cni)
{
	this->setRequiredModcod(id, cni, this->input_modcod_def,
	                        this->input_sts);
}


void DvbChannel::setRequiredModcodOutput(tal_id_t id, double cni)
{
	this->setRequiredModcod(id, cni, this->output_modcod_def,
	                        this->output_sts);
}


uint8_t DvbChannel::getCurrentModcodIdInput(tal_id_t id) const
{
	return this->input_sts->getCurrentModcodId(id);
}


uint8_t DvbChannel::getCurrentModcodIdOutput(tal_id_t id) const
{
	return this->output_sts->getCurrentModcodId(id);
}



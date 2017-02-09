/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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

/*
 * @file SatDelayMap.cpp
 * @brief  Class containing all SatDelay plugins during the simulation
 * @author Joaquin MUGUERZA / Viveris Technologies
 */


#include "SatDelayMap.h"
#include "Plugin.h"
#include "OpenSandFrames.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

#include <unistd.h>
#include <stdlib.h>
#include <cstring>


SatDelayMap::SatDelayMap():
	carrier_delay(),
	spot_delay(),
	gw_delay(),
	refresh_period_ms(0)
{
	// Output log
	this->log_init = Output::registerLog(LEVEL_WARNING, "SatCarrier.SatDelayMap.init");
	this->log_delay = Output::registerLog(LEVEL_WARNING, "SatCarrier.SatDelayMap.log");
}

/**
 * Destructor
 */
SatDelayMap::~SatDelayMap()
{
	map<uint8_t, SatDelayPlugin**>::iterator it;
	for(it = this->carrier_delay.begin();
			it != this->carrier_delay.end();
			it++)
	{
		delete[] ((*it).second);
	}
}

bool SatDelayMap::init(bool probes)
{
	ConfigurationList delays_list;
	ConfigurationList::iterator iter_list;
	ConfigurationList global_plugin_conf;
	ConfigurationList spot_list;
	ConfigurationList::iterator iter;
	time_ms_t refresh_period_ms;
	string orbit;
	string target;
	string satdelay_name;
	uint8_t id;
	uint8_t gw_id;
	uint8_t spot_id;
	uint8_t carrier_id;
	string carrier_type;
	SatDelayPlugin *satdelay_plugin;
	SatDelayPlugin **carrier_array;
	// ** LOAD ALL PLUGINS **
	// get the orbit type
	if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
	                   SATELLITE_ORBIT, orbit))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", SATELLITE_ORBIT);
		goto error;
	}

	// get the refresh period
	if(!Conf::getValue(Conf::section_map[SAT_DELAYS_SECTION],
	                   REFRESH_PERIOD_MS, refresh_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get '%s' value", REFRESH_PERIOD_MS);
		goto error;
	}
	this->refresh_period_ms = refresh_period_ms;

	// get all plugins 
	if(!Conf::getListItems(Conf::section_map[SAT_DELAYS_SECTION],
												 DELAYS_LIST, delays_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"section '%s': missing list '%s'\n",
				SAT_DELAYS_SECTION, DELAYS_LIST);
		goto error;
	}
	
	// if GEO, get the global delay configuration first
	if(!strcmp(orbit.c_str(), ORBIT_GEO))
	{
		if(!Conf::getItemNode(Conf::section_map[SAT_DELAYS_SECTION],
													GLOBAL_DELAY, global_plugin_conf))
		{
			LOG(this->log_init, LEVEL_ERROR,
					"missing parameter '%s'", GLOBAL_DELAY);
			goto error;
		}
	}
	
	for(iter_list = delays_list.begin();
			iter_list != delays_list.end();
			iter_list++)
	{
		ConfigurationList plugin_conf;
		// get the target
		if(!Conf::getAttributeValue(iter_list, TARGET, target))
		{
			LOG(this->log_init, LEVEL_ERROR,
					"cannot get delay target");
			goto error;
		}
		// get the id
		if(!Conf::getAttributeValue(iter_list, ID, id))
		{
			LOG(this->log_init, LEVEL_ERROR,
					"cannot get delay id");
			goto error;
		}
		// If GEO, use the global conf
		if(!strcmp(orbit.c_str(), ORBIT_GEO))
		{
			if(!Plugin::getSatDelayPlugin(CONSTANT_DELAY,
																		&satdelay_plugin))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"error when getting the sat delay plugin '%s'",
						satdelay_name.c_str());
				goto error;
			}
			plugin_conf = global_plugin_conf;
			satdelay_name = CONSTANT_DELAY;
		}
		// If LEO/MEO, get conf, and load proper plugin
		// get the plugin conf
		else
		{
			if(!Conf::getItemNode(*iter_list, SAT_DELAY_CONF, plugin_conf))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"missing parameter '%s' for delay %s id %u",
						SAT_DELAY_CONF, target.c_str(), id);
				goto error;
			}
			// get plugin name
			if(!Conf::getAttributeValue(iter_list, DELAY_TYPE, satdelay_name))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"missing parameter '%s' for %s id %u",
						DELAY_TYPE, target.c_str(), id);
				goto error;
			}
			
			// load plugin
			if(!Plugin::getSatDelayPlugin(satdelay_name,
																		&satdelay_plugin))
			{
				LOG(this->log_init, LEVEL_ERROR,
						"error when getting the sat delay plugin '%s'",
						satdelay_name.c_str());
				goto error;
			}
		}
		// insert plugin into map
		if(!strcmp(target.c_str(), GW))
		{
			this->gw_delay[id] = satdelay_plugin;
		}
		else
		{
			this->spot_delay[id] = satdelay_plugin;
		}
		// register probes
		if(probes)
		{
			if(!strcmp(target.c_str(), GW))
			{
				this->gw_probe[id] = Output::registerProbe<int>("ms", true,
				                                                SAMPLE_LAST,
				                                                "Delays.GW_%d", id);
			}
			else
			{
				this->spot_probe[id] = Output::registerProbe<int>("ms", true,
				                                                  SAMPLE_LAST,
				                                                  "Delays.Spot_%d", id);
			}
		}
		// init plugin
		if(!satdelay_plugin->init(plugin_conf))
		{
			LOG(this->log_init, LEVEL_ERROR,
					"cannot initialize sat delay plugin '%s'"
					" for %s id %u ", satdelay_name.c_str(),
					target.c_str(), id);
			goto error;
		}
	}
	// ** CREATE THE CARRIER ID MAP **
	if(!Conf::getListNode(Conf::section_map[SATCAR_SECTION], SPOT_LIST,
	   spot_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing satellite channels\n",
		    SATCAR_SECTION, SPOT_LIST);
		goto error;
	}
	for(iter_list = spot_list.begin();
	    iter_list != spot_list.end();
	    iter_list++)
	{
		ConfigurationList carrier_list;

		if(!Conf::getAttributeValue(iter_list, ID, spot_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "there is no attribute %s in %s/%s",
			    ID, SATCAR_SECTION, SPOT_LIST);
			goto error;
		}
		if(!Conf::getAttributeValue(iter_list, GW, gw_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "there is no attribute %s in %s/%s",
			    GW, SATCAR_SECTION, SPOT_LIST);
			goto error;
		}
		if(!Conf::getListItems(*iter_list, CARRIER_LIST, carrier_list))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "section '%s, %s': missing satellite channels\n",
			    SATCAR_SECTION, CARRIER_LIST);
			goto error;
		}
		// get all carriers from this spot
		for(iter = carrier_list.begin();
		    iter != carrier_list.end();
		    iter++)
		{
			// get carrier ID
			if(!Conf::getAttributeValue(iter, CARRIER_ID, carrier_id))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s\n"
				    SPOT_LIST, spot_id, SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_ID);
				goto error;
			}
			// get carrier type
			if(!Conf::getAttributeValue(iter, CARRIER_TYPE, carrier_type))
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "section '%s %d/%s/%s': failed to retrieve %s\n"
				    SPOT_LIST, spot_id, SATCAR_SECTION, CARRIER_LIST,
				    CARRIER_TYPE);
				goto error;
			}
			// Fill carrier_array. Each carrier ID is linked to a 2-elements array
			// For most carrier types, only the first element is used, and it points
			// to the satdelay plugin por this carrier. For the case of CTRL carriers,
			// where IN and OUT are not splitted, the first element corresponds to the
			// spot delay, and the second to the GW delay.
			// If necessary, register probe for each carrier
			carrier_array = new SatDelayPlugin*[2];
			if(!strcmp(carrier_type.c_str(), LOGON_OUT))
			{
				carrier_array[0] = this->gw_delay[gw_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to GW%d",
				    carrier_id, carrier_type.c_str(), gw_id);
			}
			else if(!strcmp(carrier_type.c_str(), LOGON_IN))
			{
				carrier_array[0] = this->spot_delay[spot_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to SPOT %d",
				    carrier_id, carrier_type.c_str(), spot_id);
			}
			else if(!strcmp(carrier_type.c_str(), DATA_OUT_ST))
			{
				carrier_array[0] = this->spot_delay[spot_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to SPOT %d",
				    carrier_id, carrier_type.c_str(), spot_id);
			}
			else if(!strcmp(carrier_type.c_str(), DATA_IN_ST))
			{
				carrier_array[0] = this->spot_delay[spot_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to SPOT %d",
				    carrier_id, carrier_type.c_str(), spot_id);
			}
			else if(!strcmp(carrier_type.c_str(), DATA_OUT_GW))
			{
				carrier_array[0] = this->gw_delay[gw_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to GW%d",
				    carrier_id, carrier_type.c_str(), gw_id);
			}
			else if(!strcmp(carrier_type.c_str(), DATA_IN_GW))
			{
				carrier_array[0] = this->gw_delay[gw_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to GW%d",
				    carrier_id, carrier_type.c_str(), gw_id);
			}
			else if(!strcmp(carrier_type.c_str(), CTRL_IN))
			{
				carrier_array[0] = this->spot_delay[spot_id];
				carrier_array[1] = this->gw_delay[gw_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to GW%d and SPOT%d",
				    carrier_id, carrier_type.c_str(), gw_id, spot_id);
			}
			else if(!strcmp(carrier_type.c_str(), CTRL_OUT))
			{
				carrier_array[0] = this->spot_delay[spot_id];
				carrier_array[1] = this->gw_delay[gw_id];
				LOG(this->log_init, LEVEL_DEBUG,
				    "carrier number %u (%s) is associated to GW%d and SPOT%d",
				    carrier_id, carrier_type.c_str(), gw_id, spot_id);
			}
			else
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "unknown carrier type '%s', in spot %u"
				    " with gw %u\n", carrier_type.c_str(),
				    spot_id, gw_id);
				delete[] carrier_array;
				goto error;
			}
			this->carrier_delay[carrier_id] = carrier_array;
		}
	}
	return true;
error:
	return false;
}

bool SatDelayMap::updateSatDelays()
{
	SatDelayPlugin *satdelay;
	
	for(map<tal_id_t, SatDelayPlugin*>::iterator it = this->gw_delay.begin();
	    it != this->gw_delay.end();
	    it++)
	{
		satdelay = (*it).second;
		if(!satdelay->updateSatDelay())
		{
			LOG(this->log_delay, LEVEL_ERROR,
			    "error when updating satdelay for GW %u",
			    (*it).first);
			goto error;
		}
		// update probe
		if(this->gw_probe[(*it).first])
		{
			this->gw_probe[(*it).first]->put(satdelay->getSatDelay());
		}
	}
	for(map<spot_id_t, SatDelayPlugin*>::iterator it = this->spot_delay.begin();
	    it != this->spot_delay.end();
	    it++)
	{
		satdelay = (*it).second;
		if(!satdelay->updateSatDelay())
		{
			LOG(this->log_delay, LEVEL_ERROR,
			    "error when updating satdelay for spot %u",
			    (*it).first);
			goto error;
		}
		// update probe
		if(this->spot_probe[(*it).first])
		{
			this->spot_probe[(*it).first]->put(satdelay->getSatDelay());
		}
	}
	return true;
error:
	return false;
}

bool SatDelayMap::getMaxDelay(time_ms_t &delay)
{
	time_ms_t current_delay;
	time_ms_t delay_a;
	time_ms_t delay_b;
	SatDelayPlugin *satdelay;

	delay = 0;

	for(map<spot_id_t, SatDelayPlugin*>::iterator it = this->spot_delay.begin();
	    it != this->spot_delay.end(); it++)
	{
		satdelay = (*it).second;
		if(!satdelay->getMaxDelay(delay_a))
		{
			LOG(this->log_delay, LEVEL_ERROR,
			    "error when getting max delay of spot %u",
			    (*it).first);
			goto error;
		}
		// test all possible combinations between spot<->spot and spot<->gw
		for(map<spot_id_t, SatDelayPlugin*>::iterator it_b = this->spot_delay.begin();
		    it_b != this->spot_delay.end(); it++)
		{
			current_delay = delay_a;
			satdelay = (*it_b).second;
			if(!satdelay->getMaxDelay(delay_b))
			{
				LOG(this->log_delay, LEVEL_ERROR,
				    "error when getting max delay of spot %u",
				    (*it_b).first);
				goto error;
			}
			current_delay += delay_b;
			if(current_delay > delay)
				delay = current_delay;
		}
		for(map<tal_id_t, SatDelayPlugin*>::iterator it_b = this->gw_delay.begin();
		    it_b != this->gw_delay.end(); it++)
		{
			current_delay = delay_a;
			satdelay = (*it_b).second;
			if(!satdelay->getMaxDelay(delay_b))
			{
				LOG(this->log_delay, LEVEL_ERROR,
				    "error when getting max delay of gw %u",
				    (*it_b).first);
				goto error;
			}
			current_delay += delay_b;
			if(current_delay > delay)
				delay = current_delay;
		}
	}
	return true;
error:
	return false;
}

bool SatDelayMap::getDelayIn(uint8_t carrier_id,
                             uint8_t msg_type,
                             time_ms_t &delay)
{
	// TODO: handle case where CTRL message is in incorrect carrier?
	// (this should never happen)
	 
	map<uint8_t, SatDelayPlugin**>::iterator it = this->carrier_delay.find(carrier_id);
	if(it == this->carrier_delay.end())
	{
		LOG(this->log_delay, LEVEL_ERROR,
		    "cannot find carrier id %u in carrier delays map",
		    carrier_id);
		goto error;
	}
	switch(msg_type)
	{
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_SALOHA_DATA:
		case MSG_TYPE_SALOHA_CTRL:
		case MSG_TYPE_SESSION_LOGON_REQ:
		case MSG_TYPE_SESSION_LOGOFF:
		{
			delay = ((SatDelayPlugin*)((*it).second[0]))->getSatDelay();
			break;
		}
		case MSG_TYPE_SOF:
		{
			delay = 0;
			break;
		}
		case MSG_TYPE_SAC:
		{
			delay = ((SatDelayPlugin*)((*it).second[0]))->getSatDelay();
			break;
		}
		case MSG_TYPE_TTP:
		case MSG_TYPE_SYNC:
		case MSG_TYPE_SESSION_LOGON_RESP:
		{
			delay = ((SatDelayPlugin*)((*it).second[1]))->getSatDelay();
			break;
		}
		// not used as of now
		case MSG_TYPE_ERROR:
		case MSG_TYPE_CSC:
		default:
		{
			// TODO: show error???
			delay = 0;
			break;
		}
	}
	return true;
error:
	return false;
}

bool SatDelayMap::getDelayOut(uint8_t carrier_id,
                              uint8_t msg_type,
                              time_ms_t &delay)
{
	// TODO: handle case where CTRL message is in incorrect carrier?
	// (this should never happen)
	
	map<uint8_t, SatDelayPlugin**>::iterator it = this->carrier_delay.find(carrier_id);
	if(it == this->carrier_delay.end())
	{
		LOG(this->log_delay, LEVEL_ERROR,
		    "cannot find carrier id %u in carrier delays map",
		    carrier_id);
		goto error;
	}
	switch(msg_type)
	{
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_SALOHA_DATA:
		case MSG_TYPE_SALOHA_CTRL:
		case MSG_TYPE_SESSION_LOGON_REQ:
		case MSG_TYPE_SESSION_LOGOFF:
		{
			delay = ((SatDelayPlugin*)((*it).second[0]))->getSatDelay();
			break;
		}
		case MSG_TYPE_SOF:
		{
			delay = 0;
			break;
		}
		case MSG_TYPE_SAC:
		{
			delay = ((SatDelayPlugin*)((*it).second[1]))->getSatDelay();
			break;
		}
		case MSG_TYPE_TTP:
		case MSG_TYPE_SYNC:
		case MSG_TYPE_SESSION_LOGON_RESP:
		{
			delay = ((SatDelayPlugin*)((*it).second[0]))->getSatDelay();
			break;
		}
		// not used as of now
		case MSG_TYPE_ERROR:
		case MSG_TYPE_CSC:
		default:
		{
			// TODO: show error???
			delay = 0;
			break;
		}
	}
	return true;
error:
	return false;
}

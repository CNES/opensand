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
 * @file FileSimulator.cpp
 * @brief simulation using files
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#include "FileSimulator.h"

#include <errno.h>


FileSimulator::FileSimulator(spot_id_t spot_id,
                             tal_id_t mac_id,
                             sat_type_t sat_type,
                             bool phy_layer,
                             FILE** evt_file,
                             Simulate simulate,
						     ConfigurationList current_gw):
	RequestSimulator(spot_id, mac_id, 
	                  sat_type, phy_layer, evt_file,
	                  simulate, current_gw)
{
	string str_config;
	if(!Conf::getValue(current_gw, DVB_SIMU_FILE, str_config))
	{
		LOG(this->log_init, LEVEL_ERROR,
				"cannot load parameter %s from section %s\n",
				DVB_SIMU_FILE, DVB_NCC_SECTION);
		// cannot return value from constructer
		assert(0);
	}
	
	if(str_config == "stdin")
	{
		this->simu_file = stdin;
	}
	else
	{
		this->simu_file = fopen(str_config.c_str(), "r");
	}
	if(this->simu_file == NULL && str_config != "none")
	{
		LOG(this->log_init, LEVEL_ERROR,
				"%s\n", strerror(errno));
		LOG(this->log_init, LEVEL_ERROR,
				"no simulation file will be used.\n");
	}
	else
	{
		LOG(this->log_init, LEVEL_NOTICE,
				"events simulated from %s.\n",
				str_config.c_str());
		this->simulate = file_simu;
	}
}

FileSimulator::~FileSimulator()
{
}


// TODO create a class for simulation and subclass file/random
bool FileSimulator::simulation(list<DvbFrame *>* msgs,
                               time_sf_t super_frame_counter)
{
	enum
	{ none, cr, logon, logoff } event_selected;

	int resul;
	time_sf_t sf_nr;
	tal_id_t st_id;
	uint32_t st_request;
	rate_kbps_t st_rt;
	rate_kbps_t st_rbdc;
	vol_kb_t st_vbdc;
	int cr_type;

	if(this->simu_eof)
	{
		LOG(this->log_request_simulation, LEVEL_DEBUG,
		    "End of file\n");
		goto end;
	}

	sf_nr = 0;
	while(sf_nr <= super_frame_counter)
	{
		if(4 ==
		   sscanf(this->simu_buffer,
		          "SF%hu CR st%hu cr=%u type=%d",
		          &sf_nr, &st_id, &st_request, &cr_type))
		{
			event_selected = cr;
		}
		else if(5 ==
		        sscanf(this->simu_buffer,
		               "SF%hu LOGON st%hu rt=%hu rbdc=%hu vbdc=%hu",
		               &sf_nr, &st_id, &st_rt, &st_rbdc, &st_vbdc))
		{
			event_selected = logon;
		}
		else if(2 ==
		        sscanf(this->simu_buffer, "SF%hu LOGOFF st%hu", &sf_nr, &st_id))
		{
			event_selected = logoff;
		}
		else
		{
			event_selected = none;
		}
		if(event_selected != none && st_id <= BROADCAST_TAL_ID)
		{
			LOG(this->log_request_simulation, LEVEL_WARNING,
			    "Simulated ST%u ignored, IDs smaller than %u "
			    "reserved for emulated terminals\n",
			    st_id, BROADCAST_TAL_ID);
			goto loop_step;
		}
		if(event_selected == none)
			goto loop_step;
		if(sf_nr < super_frame_counter)
			goto loop_step;
		if(sf_nr > super_frame_counter)
			break;
		switch (event_selected)
		{
			case cr:
			{
				Sac *sac = new Sac(st_id);
				sac->addRequest(0, cr_type, st_request);
				msgs->push_back((DvbFrame*)sac);
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "SF#%u: send a simulated CR of type %u with "
				    "value = %u for ST %hu\n",
				    super_frame_counter, cr_type,
				    st_request, st_id);
				break;
			}
			case logon:
			{
				LogonRequest *logon_req = new LogonRequest(st_id,
				                                           st_rt,
				                                           st_rbdc,
				                                           st_vbdc);
				msgs->push_back((DvbFrame *)logon_req);
				
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "SF#%u: send a simulated logon for ST %d\n",
				    super_frame_counter, st_id);
				break;
			}
			case logoff:
			{
				Logoff *logoff_req = new Logoff(st_id);
				msgs->push_back((DvbFrame*)logoff_req);
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "SF#%u: send a simulated logoff for ST %d\n",
				    super_frame_counter, st_id);
				break;
			}
			default:
				break;
		}
	 loop_step:
		resul = -1;
		while(resul < 1)
		{
			resul = fscanf(this->simu_file, "%254[^\n]\n", this->simu_buffer);
			if(resul == 0)
			{
				int ret;
				// No conversion occured, we simply skip the line
				ret = fscanf(this->simu_file, "%*s");
				if ((ret == 0) || (ret == EOF))
				{
					return false;
				}
			}
			LOG(this->log_request_simulation, LEVEL_DEBUG,
			    "fscanf result=%d: %s", resul, this->simu_buffer);
			//fprintf (stderr, "frame %d\n", super_frame_counter);
			LOG(this->log_request_simulation, LEVEL_DEBUG,
			    "frame %u\n", super_frame_counter);
			if(resul == -1)
			{
				this->simu_eof = true;
				LOG(this->log_request_simulation, LEVEL_DEBUG,
				    "End of file.\n");
				goto end;
			}
		}
	}

end:
	fflush(this->event_file);
	return true;
}


bool FileSimulator::stopSimulation(void)
{
	fclose(this->simu_file);
	this->simulate = none_simu;
	this->simu_file = NULL;
	return true;
}

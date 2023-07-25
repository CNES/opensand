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
 * @file FileSimulator.cpp
 * @brief simulation using files
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#include <errno.h>
#include <cstring>
#include <cinttypes>

#include <opensand_output/Output.h>

#include "FileSimulator.h"
#include "Sac.h"
#include "Logon.h"
#include "Logoff.h"
#include "DvbFifo.h"


enum class EventType
{
	none,
	cr,
	logon,
	logoff,
};


static void in_deletor(std::istream* istream_ptr)
{
	if (istream_ptr && istream_ptr != &std::cin)
	{
		auto ifstream = static_cast<std::ifstream*>(istream_ptr);
		ifstream->close();
		delete ifstream;
	}
};


FileSimulator::FileSimulator(spot_id_t spot_id,
                             tal_id_t mac_id,
                             std::ostream* &evt_file,
                             const std::string &str_config):
	RequestSimulator(spot_id, mac_id, evt_file),
	simu_buffer(""),
	simu_file(nullptr, in_deletor)
{

	if(str_config == "stdin")
	{
		this->simu_file.reset(&std::cin);
	}
	else
	{
		try
		{
			this->simu_file.reset(new std::ifstream(str_config));
		}
		catch (const std::bad_alloc&)
		{
		}
		if (!this->simu_file || !(*this->simu_file))
		{
			// LOG(this->log_init, LEVEL_ERROR,
			//     "%s\n", strerror(errno));
			LOG(this->log_init, LEVEL_ERROR,
			    "no simulation file will be used.\n");
			this->simu_file.reset();
		}
	}

	if(this->simu_file != nullptr)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "events simulated from %s.\n",
		    str_config.c_str());
	}
}


// TODO create a class for simulation and subclass file/random
bool FileSimulator::simulation(std::list<Rt::Ptr<DvbFrame>> &msgs,
                               time_sf_t super_frame_counter)
{
	EventType event_selected;

	time_sf_t sf_nr;
	tal_id_t st_id;
	uint32_t st_request;
	rate_kbps_t st_rt;
	rate_kbps_t st_rbdc;
	vol_kb_t st_vbdc;
	uint8_t cr_type;

	sf_nr = 0;
	while(!this->simu_eof)
	{
		if(4 == sscanf(this->simu_buffer.c_str(),
		               "SF%hu CR st%hu cr=%u type=%" SCNu8,
		               &sf_nr, &st_id, &st_request, &cr_type))
		{
			event_selected = EventType::cr;
		}
		else if(5 == sscanf(this->simu_buffer.c_str(),
		                    "SF%hu LOGON st%hu rt=%hu rbdc=%hu vbdc=%hu",
		                    &sf_nr, &st_id, &st_rt, &st_rbdc, &st_vbdc))
		{
			event_selected = EventType::logon;
		}
		else if(2 == sscanf(this->simu_buffer.c_str(),
		                    "SF%hu LOGOFF st%hu",
		                    &sf_nr, &st_id))
		{
			event_selected = EventType::logoff;
		}
		else
		{
			event_selected = EventType::none;
			st_id = BROADCAST_TAL_ID + 1;  // silence next warning
		}

		if(sf_nr > super_frame_counter)
			break;

		if(st_id <= BROADCAST_TAL_ID)
		{
			LOG(this->log_request_simulation, LEVEL_WARNING,
			    "Simulated ST%u ignored, IDs smaller than %u "
			    "reserved for emulated terminals\n",
			    st_id, BROADCAST_TAL_ID);
		}
		else if (sf_nr == super_frame_counter)
		{
			switch (event_selected)
			{
				case EventType::cr:
				{
					auto sac = Rt::make_ptr<Sac>(st_id);
					sac->addRequest(0, to_enum<ReturnAccessType>(cr_type), st_request);
					sac->setAcm(0xffff);
					msgs.push_back(dvb_frame_downcast(std::move(sac)));
					LOG(this->log_request_simulation, LEVEL_INFO,
					    "SF#%u: send a simulated CR of type %u with "
					    "value = %u for ST %hu\n",
					    super_frame_counter, cr_type,
					    st_request, st_id);
					break;
				}
				case EventType::logon:
				{
					auto logon_req = Rt::make_ptr<LogonRequest>(st_id, st_rt, st_rbdc, st_vbdc);
					msgs.push_back(dvb_frame_downcast(std::move(logon_req)));
					LOG(this->log_request_simulation, LEVEL_INFO,
					    "SF#%u: send a simulated logon for ST %d\n",
					    super_frame_counter, st_id);
					break;
				}
				case EventType::logoff:
				{
					auto logoff_req = Rt::make_ptr<Logoff>(st_id);
					msgs.push_back(dvb_frame_downcast(std::move(logoff_req)));
					LOG(this->log_request_simulation, LEVEL_INFO,
					    "SF#%u: send a simulated logoff for ST %d\n",
					    super_frame_counter, st_id);
					break;
				}
				default:
					break;
			}
		}

		std::istream &stream = std::getline(*this->simu_file, this->simu_buffer);
		if (stream.eof())
		{
			this->simu_eof = true;
			break;
		}

		if (!stream)
		{
			return false;
		}
		LOG(this->log_request_simulation, LEVEL_DEBUG,
		    "fscanf result is: %s", this->simu_buffer);
		LOG(this->log_request_simulation, LEVEL_DEBUG,
		    "frame %u\n", super_frame_counter);
	}

	if(this->simu_eof)
	{
		LOG(this->log_request_simulation, LEVEL_DEBUG,
		    "End of file\n");
	}
	this->event_file->flush();
	return true;
}


bool FileSimulator::stopSimulation()
{
	this->simu_file = std::unique_ptr<std::istream, void(*)(std::istream*)>(nullptr, [](std::istream*){});
	return true;
}

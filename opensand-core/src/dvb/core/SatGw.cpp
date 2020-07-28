/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file SatGw.cpp
 * @brief This bloc implements a satellite spots
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "SatGw.h"
#include "OpenSandFrames.h"
#include "MacFifoElement.h"
#include "ForwardSchedulingS2.h"
#include "DvbRcsFrame.h"

#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

#include <stdlib.h>

// TODO size per fifo ?
// TODO to not do...: do not create all fifo in the regenerative case
SatGw::SatGw(tal_id_t gw_id,
             spot_id_t spot_id,
             uint8_t log_id,
             uint8_t ctrl_id,
             uint8_t data_in_st_id,
             uint8_t data_in_gw_id,
             uint8_t data_out_st_id,
             uint8_t data_out_gw_id,
             size_t fifo_size):
	gw_id(gw_id),
	spot_id(spot_id),
	data_in_st_id(data_in_st_id),
	data_in_gw_id(data_in_gw_id),
	l2_from_st_bytes(0),
	l2_from_gw_bytes(0),
	gw_mutex(),
	probe_sat_output_gw_queue_size(),
	probe_sat_output_gw_queue_size_kb(),
	probe_sat_output_st_queue_size(),
	probe_sat_output_st_queue_size_kb(),
	probe_sat_l2_from_st(),
	probe_sat_l2_to_st(),
	probe_sat_l2_from_gw(),
	probe_sat_l2_to_gw()
{
	// initialize MAC FIFOs
#define SIG_FIFO_SIZE 1000
	this->logon_fifo = new DvbFifo(log_id, SIG_FIFO_SIZE, "logon_fifo");
	this->control_fifo = new DvbFifo(ctrl_id, SIG_FIFO_SIZE, "control_fifo");
	this->data_out_st_fifo = new DvbFifo(data_out_st_id, fifo_size,
	                                     "data_out_st");
	this->data_out_gw_fifo = new DvbFifo(data_out_gw_id, fifo_size,
	                                     "data_out_gw");
	// Output Log
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING,
                                              "Dvb.spot_%d.gw_%d.init",
                                              this->spot_id, this->gw_id);
	this->log_receive = Output::Get()->registerLog(LEVEL_WARNING,
                                                 "Dvb.spot_%d.gw_%d.receive",
                                                 this->spot_id, this->gw_id);
	this->input_sts = new StFmtSimuList("in");
	this->output_sts = new StFmtSimuList("out");
}

SatGw::~SatGw()
{
	delete this->input_sts;
	delete this->output_sts;

	delete this->logon_fifo;
	delete this->control_fifo;
	delete this->data_out_st_fifo;
	delete this->data_out_gw_fifo;
}

bool SatGw::init()
{
	if(!this->initProbes())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize probes\n");
		return false;
	}

	return true;
}

bool SatGw::initProbes()
{
	std::shared_ptr<Probe<int>> probe_output_st;
	std::shared_ptr<Probe<int>> probe_output_st_kb;
	std::shared_ptr<Probe<int>> probe_l2_to_st;
	std::shared_ptr<Probe<int>> probe_l2_from_st;
	std::shared_ptr<Probe<int>> probe_l2_to_gw;
	std::shared_ptr<Probe<int>> probe_l2_from_gw;
	std::shared_ptr<Probe<int>> probe_output_gw;
	std::shared_ptr<Probe<int>> probe_output_gw_kb;
	char probe_name[128];
  auto output = Output::Get();


	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Delay buffer size.Output_ST",
					 this->spot_id, this->gw_id);
	probe_output_st = output->registerProbe<int>(
			probe_name, "Packets", false, SAMPLE_LAST);
	this->probe_sat_output_st_queue_size.emplace(gw_id, probe_output_st);

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Delay buffer size.Output_ST_kb",
					 this->spot_id, this->gw_id);
	probe_output_st_kb = output->registerProbe<int>(
			probe_name, "Kbits", false, SAMPLE_LAST);
	this->probe_sat_output_st_queue_size_kb.emplace(gw_id, probe_output_st_kb);

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Throughputs.L2_to_ST",
					 this->spot_id, this->gw_id);
	probe_l2_to_st = output->registerProbe<int>(
			probe_name, "Kbits/s", true, SAMPLE_LAST);
	this->probe_sat_l2_to_st.emplace(gw_id, probe_l2_to_st);

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Throughputs.L2_from_ST",
					 this->spot_id, this->gw_id);
	probe_l2_from_st = output->registerProbe<int>(
			probe_name, "Kbits/s", true, SAMPLE_LAST);
	this->probe_sat_l2_from_st.emplace(gw_id, probe_l2_from_st);


	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Throughputs.L2_to_GW",
					 this->spot_id, this->gw_id);
	probe_l2_to_gw = output->registerProbe<int>(
			probe_name, "Kbits/s", true, SAMPLE_LAST);
	this->probe_sat_l2_to_gw.emplace(gw_id, probe_l2_to_gw);

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Throughputs.L2_from_GW",
					 this->spot_id, this->gw_id);
	probe_l2_from_gw = output->registerProbe<int>(
			probe_name, "Kbits/s", true, SAMPLE_LAST);
	this->probe_sat_l2_from_gw.emplace(gw_id, probe_l2_from_gw);

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Delay buffer size.Output_GW",
					 this->spot_id, this->gw_id);
	probe_output_gw = output->registerProbe<int>(
			probe_name, "Packets", false, SAMPLE_LAST);
	this->probe_sat_output_gw_queue_size.emplace(gw_id, probe_output_gw);

	snprintf(probe_name, sizeof(probe_name),
	         "Spot_%d.GW_%d.Delay buffer size.Output_GW_kb",
					 this->spot_id, this->gw_id);
	probe_output_gw_kb = output->registerProbe<int>(
			probe_name, "Kbits", false, SAMPLE_LAST);
	this->probe_sat_output_gw_queue_size_kb.emplace(gw_id, probe_output_gw_kb);

	return true;
}

bool SatGw::updateProbes(time_ms_t stats_period_ms)
{
	// Queue sizes
	mac_fifo_stat_context_t output_gw_fifo_stat;
	mac_fifo_stat_context_t output_st_fifo_stat;
	this->data_out_st_fifo->getStatsCxt(output_st_fifo_stat);

	this->probe_sat_output_st_queue_size[this->gw_id]->put(
			output_st_fifo_stat.current_pkt_nbr);
	this->probe_sat_output_st_queue_size_kb[this->gw_id]->put(
			((int) output_st_fifo_stat.current_length_bytes * 8 / 1000));

	// Throughputs
	// L2 from ST
	this->probe_sat_l2_from_st[this->gw_id]->put(
			this->getL2FromSt() * 8 / stats_period_ms);

	// L2 to ST
	this->probe_sat_l2_to_st[this->gw_id]->put(
			((int) output_st_fifo_stat.out_length_bytes * 8 /
			 stats_period_ms));

	// L2 from GW
	this->probe_sat_l2_from_gw[this->gw_id]->put(
			this->getL2FromGw() * 8 / stats_period_ms);

	// L2 to GW
	data_out_gw_fifo->getStatsCxt(output_gw_fifo_stat);
	this->probe_sat_l2_to_gw[this->gw_id]->put(
			((int) output_gw_fifo_stat.out_length_bytes * 8 /
			 stats_period_ms));

	// Queue sizes
	this->probe_sat_output_gw_queue_size[this->gw_id]->put(
			output_gw_fifo_stat.current_pkt_nbr);
	this->probe_sat_output_gw_queue_size_kb[this->gw_id]->put(
			((int) output_gw_fifo_stat.current_length_bytes * 8 / 1000));

	return true;
}


uint16_t SatGw::getGwId(void) const
{
	return this->gw_id;
}

uint8_t SatGw::getDataInStId(void) const
{
	return this->data_in_st_id;
}

uint8_t SatGw::getDataInGwId(void) const
{
	return this->data_in_gw_id;
}

DvbFifo *SatGw::getDataOutStFifo(void) const
{
	return this->data_out_st_fifo;
}

DvbFifo *SatGw::getDataOutGwFifo(void) const
{
	return this->data_out_gw_fifo;
}

DvbFifo *SatGw::getControlFifo(void) const
{
	return this->control_fifo;
}

uint8_t SatGw::getControlCarrierId(void) const
{
	return this->control_fifo->getCarrierId();
}

DvbFifo *SatGw::getLogonFifo(void) const
{
	return this->logon_fifo;
}

void SatGw::updateL2FromSt(vol_bytes_t bytes)
{
	RtLock lock(this->gw_mutex);
	this->l2_from_st_bytes += bytes;
}

void SatGw::updateL2FromGw(vol_bytes_t bytes)
{
	RtLock lock(this->gw_mutex);
	this->l2_from_gw_bytes += bytes;
}

vol_bytes_t SatGw::getL2FromSt(void)
{
	RtLock lock(this->gw_mutex);
	vol_bytes_t val = this->l2_from_st_bytes;
	this->l2_from_st_bytes = 0;
	return val;
}

vol_bytes_t SatGw::getL2FromGw(void)
{
	RtLock lock(this->gw_mutex);
	vol_bytes_t val = this->l2_from_gw_bytes;
	this->l2_from_gw_bytes = 0;
	return val;
}

spot_id_t SatGw::getSpotId(void)
{
	return this->spot_id;
}

void SatGw::print(void)
{
	DFLTLOG(LEVEL_ERROR, "gw_id = %d, spot_id = %d\n",
	        this->gw_id, this->spot_id);
}


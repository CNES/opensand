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
 * @file SpotDownwardTransp.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */

#include "SpotDownwardTransp.h"

#include "ForwardSchedulingS2.h"
#include "DamaCtrlRcs2Legacy.h"
#include "OpenSandModelConf.h"

#include <errno.h>


SpotDownwardTransp::SpotDownwardTransp(spot_id_t spot_id,
                           tal_id_t mac_id,
                           time_ms_t fwd_down_frame_duration,
                           time_ms_t ret_up_frame_duration,
                           time_ms_t stats_period,
                           EncapPlugin::EncapPacketHandler *pkt_hdl,
                           StFmtSimuList *input_sts,
                           StFmtSimuList *output_sts):
	SpotDownward(spot_id, mac_id,
	             fwd_down_frame_duration,
	             ret_up_frame_duration,
	             stats_period,
	             pkt_hdl, input_sts, output_sts)
{
}

SpotDownwardTransp::~SpotDownwardTransp()
{
	this->categories.clear();
}


void SpotDownwardTransp::generateConfiguration()
{
	auto types = OpenSandModelConf::Get()->getModelTypesDefinition();
	types->addEnumType("dama_algorithm", "DAMA Algorithm", {"Legacy",});

	auto conf = OpenSandModelConf::Get()->getOrCreateComponent("network", "Network", "The DVB layer configuration");
	conf->addParameter("fca", "FCA", types->getType("int"));
	conf->addParameter("dama_algorithm", "DAMA Algorithm", types->getType("dama_algorithm"));
}


bool SpotDownwardTransp::onInit(void)
{
	if(!this->initPktHdl(RETURN_UP_ENCAP_SCHEME_LIST,
	                     &this->up_return_pkt_hdl))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed get packet handler\n");
		return false;
	}

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// Initialization of the modcod def
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward link definition MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS2,
	                            &this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the return link definition MODCOD file\n");
		return false;
	}

	if(!SpotDownward::onInit())
	{
		return false;
	}
	return true;

}


bool SpotDownwardTransp::initMode(void)
{
	TerminalCategoryDama *cat;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

	// initialize scheduling
	// depending on the satellite type
	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotForwardCarriers(this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into forward down frequency plan\n",
		    this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "forward down frequency plan",
	                                         TDM,
	                                         this->fwd_down_frame_duration_ms,
	                                         this->s2_modcod_def,
	                                         this->categories,
	                                         this->terminal_affectation,
	                                         &this->default_category,
	                                         this->fwd_fmt_groups))
	{
		return false;
	}


	// check that there is at least DVB fifos for VCM carriers
	for(cat_it = this->categories.begin();
	    cat_it != this->categories.end(); ++cat_it)
	{
		bool is_vcm_carriers = false;
		bool is_acm_carriers = false;
		bool is_vcm_fifo = false;
		fifos_t fifos;
		std::string label;
		Scheduling *schedule;

		cat = (*cat_it).second;
		label = cat->getLabel();
		if(!this->initFifo(fifos))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed initialize fifos for category %s\n", label.c_str());
			return false;
		}
		this->dvb_fifos.insert({label, fifos});

		// check if there is VCM carriers in this category
		vector<CarriersGroupDama *>::iterator carrier_it;
		vector<CarriersGroupDama *> carriers_group;
		carriers_group = (*cat_it).second->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			vector<CarriersGroupDama *> vcm_carriers;
			vector<CarriersGroupDama *>::iterator vcm_it;
			vcm_carriers = (*carrier_it)->getVcmCarriers();
			if(vcm_carriers.size() > 1)
			{
				is_vcm_carriers = true;
			}
			else
			{
				is_acm_carriers = true;
			}
		}

		for(fifos_t::iterator it = fifos.begin();
		    it != fifos.end(); ++it)
		{
			if((*it).second->getAccessType() == access_vcm)
			{
				is_vcm_fifo = true;
				break;
			}
		}
		if(is_vcm_carriers && !is_vcm_fifo)
		{
			if(!is_acm_carriers)
			{
				LOG(this->log_init_channel, LEVEL_CRITICAL,
				    "There is VCM carriers in category %s but no VCM FIFOs, "
				    "as there is no other carriers, "
				    "terminals in this category "
				    "won't be able to send any trafic. "
				    "Please check your configuration",
				    (*cat_it).second->getLabel().c_str());
				return false;
			}
			else
			{
				LOG(this->log_init_channel, LEVEL_WARNING,
				    "There is VCM carriers in category %s but no VCM FIFOs, "
				    "the VCM carriers won't be used",
				    (*cat_it).second->getLabel().c_str());
			}
		}

		schedule =  new ForwardSchedulingS2(this->fwd_down_frame_duration_ms,
		                                    this->pkt_hdl,
		                                    this->dvb_fifos.at(label),
		                                    this->output_sts,
		                                    this->s2_modcod_def,
		                                    cat, this->spot_id,
		                                    true, this->mac_id, "");
		if(!schedule)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed initialize forward scheduling for category %s\n", label.c_str());
			return false;
		}
		this->scheduling.emplace(label, schedule);
	}

	return true;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownwardTransp::initDama(void)
{
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	TerminalCategoryDama *dc_default_category = NULL;

	auto Conf = OpenSandModelConf::Get();
	auto ncc = Conf->getProfileData()->getComponent("network");

	// Retrieving the free capacity assignement parameter
	int fca;
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("fca"), fca))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing FCA parameter\n");
		return false;
	}

	fca_kbps = fca;
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);


	if(!Conf->getSynchroPeriod(sync_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Missing synchronisation period\n");
		return false;
	}
	sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                        (double)this->ret_up_frame_duration_ms);
	rbdc_timeout_sf = sync_period_frame + 1;

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "rbdc_timeout = %d superframes computed from sync period %d superframes\n",
	    rbdc_timeout_sf, sync_period_frame);

	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotReturnCarriers(this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into return up frequency plan\n",
		    this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "return up frequency plan",
	                                         DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->rcs_modcod_def,
	                                         dc_categories,
	                                         dc_terminal_affectation,
	                                         &dc_default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}


	// check if there is DAMA carriers
	if(dc_categories.size() == 0)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "No TDM carrier, won't allocate DAMA\n");
		// Also disable request simulation
		this->simulate = none_simu;
		return true;
	}

	// dama algorithm
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("dama_algorithm"), dama_algo))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section 'ncc': missing parameter 'dama_algorithm'\n");
		return false;
	}

	if(!this->up_return_pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "up return pkt hdl has not been initialized first.\n");
		return false;
	}

	/* select the specified DAMA algorithm */
	if(dama_algo == "Legacy")
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "creating Legacy DAMA controller\n");
		this->dama_ctrl = new DamaCtrlRcs2Legacy(this->spot_id);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section 'ncc': bad value '%s' for "
		    "parameter 'dama_algorithm'\n",
		    dama_algo.c_str());
		return false;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the DAMA controller\n");
		return false;
	}

	// Initialize the DamaCtrl parent class
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration_ms,
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                this->input_sts,
	                                this->rcs_modcod_def,
	                                (this->simulate == none_simu) ?
	                                false : true))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_dama;
	}
	this->dama_ctrl->setRecordFile(this->event_file);

	return true;

release_dama:
	delete this->dama_ctrl;
	return false;
}


bool SpotDownwardTransp::handleSalohaAcks(const list<DvbFrame *> *ack_frames)
{
	list<DvbFrame *>::const_iterator ack_it;
	for(ack_it = ack_frames->begin(); ack_it != ack_frames->end();
	    ++ack_it)
	{
		this->complete_dvb_frames.push_back(*ack_it);
	}
	return true;
}

/**
 * Add a CNI extension in the next GSE packet header
 * Only for SCPC
 * @return false if failed, true if succeed
 */
// TODO At the moment, the CNI is only sent when it changes and with the
//      current MODCOD that can leads to CNI not transmitted. This can be
//      correct either with a timer (base en acm_period) that may call
//      setCniInputHasChanged on all SCPC terminals or using the most
//      robust MODCOD to transmit packets with CNI extension
bool SpotDownwardTransp::addCniExt(void)
{
	list<tal_id_t> list_st;
	map<string, fifos_t>::iterator dvb_fifos_it;

	// Create list of first packet from FIFOs
	for(dvb_fifos_it = this->dvb_fifos.begin();
	    dvb_fifos_it != this->dvb_fifos.end();
	    ++dvb_fifos_it)
	{
		fifos_t::iterator fifos_it;
		fifos_t fifos = (*dvb_fifos_it).second;
		for(fifos_it = fifos.begin();
		    fifos_it != fifos.end();
		    ++fifos_it)
		{
			DvbFifo *fifo = (*fifos_it).second;
			vector<MacFifoElement *> queue = fifo->getQueue();
			vector<MacFifoElement *>::iterator queue_it;

			for(queue_it = queue.begin();
			    queue_it != queue.end();
			    ++queue_it)
			{
				std::vector<NetPacket*> packet_list;
				MacFifoElement* elem = (*queue_it);
				NetPacket *packet = (NetPacket*)elem->getElem();
				tal_id_t tal_id = packet->getDstTalId();
				NetPacket *extension_pkt = NULL;

				list<tal_id_t>::iterator it = std::find(this->is_tal_scpc.begin(),
				                                        this->is_tal_scpc.end(),
				                                        tal_id);
				if(it != this->is_tal_scpc.end() &&
				   this->getCniInputHasChanged(tal_id))
				{
					list_st.push_back(tal_id);
					packet_list.push_back(packet);
					// we could make specific SCPC function
					if(!this->setPacketExtension(this->pkt_hdl,
						                         elem, fifo,
						                         packet_list,
						                         &extension_pkt,
						                         this->mac_id,
						                         tal_id,
						                         "encodeCniExt",
						                         this->super_frame_counter,
						                         true))
					{
						return false;
					}

					LOG(this->log_send_channel, LEVEL_DEBUG,
					    "SF #%d: packet belongs to FIFO #%d\n",
					    this->super_frame_counter, (*fifos_it).first);
					// Delete old packet
					delete packet;
				}
			}
		}
	}

	// try to send empty packet if no packet has been found for a terminal
	for(set<tal_id_t>::const_iterator st_it = this->input_sts->begin();
	    st_it != this->input_sts->end(); ++st_it)
	{
		tal_id_t tal_id = *st_it;
		list<tal_id_t>::iterator it = std::find(list_st.begin(),
		                                        list_st.end(),
		                                        tal_id);
		list<tal_id_t>::iterator it_scpc = std::find(this->is_tal_scpc.begin(),
		                                             this->is_tal_scpc.end(),
		                                             tal_id);

		if(it_scpc != this->is_tal_scpc.end() && it == list_st.end()
		   && this->getCniInputHasChanged(tal_id))
		{
			std::vector<NetPacket*> packet_list;
			NetPacket *extension_pkt = NULL;
			string cat_label;
			map<string, fifos_t>::iterator fifos_it;

			// first get the relevant category for the packet to find appropriate fifo
			if(this->terminal_affectation.find(tal_id) != this->terminal_affectation.end())
			{
				if(this->terminal_affectation.at(tal_id) == NULL)
				{
					LOG(this->log_send_channel, LEVEL_ERROR,
					    "No category associated to terminal %u, "
					    "cannot send CNI for SCPC carriers\n",
					    tal_id);
					return false;
				}
				cat_label = this->terminal_affectation.at(tal_id)->getLabel();
			}
			else
			{
				if(!this->default_category)
				{
					LOG(this->log_send_channel, LEVEL_ERROR,
					    "No default category for terminal %u, "
					    "cannot send CNI for SCPC carriers\n",
					    tal_id);
					return false;
				}
				cat_label = this->default_category->getLabel();
			}
			// find the FIFO associated to the IP QoS (= MAC FIFO id)
			// else use the default id
			fifos_it = this->dvb_fifos.find(cat_label);
			if(fifos_it == this->dvb_fifos.end())
			{
				LOG(this->log_send_channel, LEVEL_ERROR,
				    "No fifo found for this category %s unable to send CNI for SCPC carriers",
				    cat_label.c_str());
				return false;
			}

			// set packet extension to this new empty packet
			if(!this->setPacketExtension(this->pkt_hdl,
				                         NULL,
				                         // highest priority fifo
				                         ((*fifos_it).second)[0],
				                         packet_list,
				                         &extension_pkt,
				                         this->mac_id,
				                         tal_id,
				                         "encodeCniExt",
				                         this->super_frame_counter,
				                         true))
			{
				return false;
			}

			LOG(this->log_send_channel, LEVEL_DEBUG,
			    "SF #%d: adding empty packet into FIFO NM\n",
			    this->super_frame_counter);
		}
	}

	return true;
}


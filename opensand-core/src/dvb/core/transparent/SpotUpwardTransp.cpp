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
 * @file SpotUpwardTransp.cpp
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "SpotUpwardTransp.h"

#include "DamaCtrlRcsLegacy.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Sof.h"

SpotUpwardTransp::SpotUpwardTransp(spot_id_t spot_id,
                                   tal_id_t mac_id,
                                   StFmtSimuList *input_sts,
                                   StFmtSimuList *output_sts):
	SpotUpward(spot_id, mac_id, input_sts, output_sts),
	saloha(NULL),
	is_tal_scpc(),
	input_series(NULL),
	output_series(NULL)
{
}


SpotUpwardTransp::~SpotUpwardTransp()
{
	if(this->saloha)
		delete this->saloha;
	
	if(this->input_series)
		delete this->input_series;
	if(this->output_series)
		delete this->output_series;
}


bool SpotUpwardTransp::onInit(void)
{
	string scheme = RETURN_UP_ENCAP_SCHEME_LIST;

	// get the common parameters
	if(!this->initCommon(scheme.c_str()))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	if(!SpotUpward::onInit())
	{
		return false;
	}

	if(!this->initSeriesGenerator())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the time series generator "
		    "part of the initialisation\n");
		return false;
	}
	
	// initialize the slotted Aloha part
	if(!this->initSlottedAloha())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		return false;
	}

	return true;
}

bool SpotUpwardTransp::initSlottedAloha(void)
{
	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> sa_terminal_affectation;
	TerminalCategorySaloha *sa_default_category;
	ConfigurationList current_spot;
	int lan_scheme_nbr;

	if(!OpenSandConf::getSpot(RETURN_UP_BAND, this->spot_id, 
		                      NO_GW, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value %d and %s with value %d into %s/%s\n",
		    ID, this->spot_id, GW, this->mac_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}
	
	if(!this->initBand<TerminalCategorySaloha>(current_spot,
	                                           RETURN_UP_BAND,
	                                           ALOHA,
	                                           this->ret_up_frame_duration_ms,
	                                           this->satellite_type,
	                                           this->input_modcod_def,
	                                           sa_categories,
	                                           sa_terminal_affectation,
	                                           &sa_default_category,
	                                           this->ret_fmt_groups))
	{
		return false;
	}

	// check if there is Slotted Aloha carriers
	if(sa_categories.size() == 0)
	{
		LOG(this->log_init_channel, LEVEL_DEBUG,
		    "No Slotted Aloha carrier\n");
		return true;
	}

	// TODO possible loss with Slotted Aloha and ROHC or MPEG
	//      (see TODO in TerminalContextSaloha.cpp)
	if(this->pkt_hdl->getName() == "MPEG2-TS")
	{
		LOG(this->log_init_channel, LEVEL_WARNING,
		    "Cannot guarantee no loss with MPEG2-TS and Slotted Aloha "
		    "on return link due to interleaving\n");
	}
	if(!Conf::getNbListItems(Conf::section_map[GLOBAL_SECTION],
		                     LAN_ADAPTATION_SCHEME_LIST,
	                         lan_scheme_nbr))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Section %s, %s missing\n", GLOBAL_SECTION,
		    LAN_ADAPTATION_SCHEME_LIST);
		return false;
	}
	for(int i = 0; i < lan_scheme_nbr; i++)
	{
		string name;
		if(!Conf::getValueInList(Conf::section_map[GLOBAL_SECTION],
			                     LAN_ADAPTATION_SCHEME_LIST,
		                         POSITION, toString(i), PROTO, name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Section %s, invalid value %d for parameter '%s'\n",
			    GLOBAL_SECTION, i, POSITION);
			return false;
		}
		if(name == "ROHC")
		{
			LOG(this->log_init_channel, LEVEL_WARNING,
			    "Cannot guarantee no loss with RoHC and Slotted Aloha "
			    "on return link due to interleaving\n");
		}
	}
	// end TODO

	// Create the Slotted ALoha part
	this->saloha = new SlottedAlohaNcc();
	if(!this->saloha)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create Slotted Aloha\n");
		return false;
	}

	// Initialize the Slotted Aloha parent class
	// Unlike (future) scheduling, Slotted Aloha get all categories because
	// it also handles received frames and in order to know to which
	// category a frame is affected we need to get source terminal ID
	if(!this->saloha->initParent(this->ret_up_frame_duration_ms,
	                             // pkt_hdl is the up_ret one because transparent sat
	                             this->pkt_hdl))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_saloha;
	}

	if(!this->saloha->init(sa_categories,
	                       sa_terminal_affectation,
	                       sa_default_category,
	                       this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_saloha;
	}

	return true;

release_saloha:
	delete this->saloha;
	return false;
}


bool SpotUpwardTransp::initModcodSimu(void)
{
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->output_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS,
	                            &this->input_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the uplink MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->input_modcod_def_scpc))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the uplink MODCOD file\n");
		return false;
	}
	
	if(!this->initModcodSimuFile(RETURN_UP_MODCOD_TIME_SERIES,
	                             this->mac_id, this->spot_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the downlink MODCOD files\n");
		return false;
	}

	// initialize the MODCOD IDs
	if(!this->fmt_simu.goFirstScenarioStep())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MODCOD scheme IDs\n");
		return false;
	}

	// declare the GW as one ST for the MODCOD scenarios
	if(!this->addInputTerminal(this->mac_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    this->mac_id);
		return false;
	}
	if(!this->addOutputTerminal(this->mac_id))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to define the GW as ST with ID %d\n",
		    this->mac_id);
		return false;
	}

	return true;
}


bool SpotUpwardTransp::initSeriesGenerator(void)
{
	string generate;
	ConfigurationList current_gw;
	string input_file;
	string output_file;
	vector<string> path_split;

	if(!this->with_phy_layer)
	{
		return true;
	}

	// Check whether we generate the time series
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   GENERATE_TIME_SERIES_PATH, generate))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, GENERATE_TIME_SERIES_PATH);
		return false;
	}
	if(generate == "none")
	{
		return true;
	}

	// load the time series filenames
	if(!OpenSandConf::getSpot(PHYSICAL_LAYER_SECTION,
	                          this->spot_id, this->mac_id, current_gw))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s', missing spot for id %d and gw %d\n",
		    PHYSICAL_LAYER_SECTION, this->spot_id, this->mac_id);
		return false;
	}

	if(!Conf::getValue(current_gw, RETURN_UP_MODCOD_TIME_SERIES,
	                   input_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s/spot_%d_gw_%d', missing section '%s'\n",
		    PHYSICAL_LAYER_SECTION, this->spot_id, this->mac_id,
		    RETURN_UP_MODCOD_TIME_SERIES);
		return false;
	}

	// extract the filename from path
	tokenize(input_file, path_split, "/");
	input_file = generate + "/" + path_split.back();

	if(!Conf::getValue(current_gw, FORWARD_DOWN_MODCOD_TIME_SERIES,
	                   output_file))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section '%s/spot_%d_gw_%d', missing section '%s'\n",
		    PHYSICAL_LAYER_SECTION, this->spot_id, this->mac_id,
		    FORWARD_DOWN_MODCOD_TIME_SERIES);
		return false;
	}

	// extract the filename from path
	tokenize(output_file, path_split, "/");
	output_file = generate + "/" + path_split.back();

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "generate input and output time series under the files %s and %s\n",
	    input_file.c_str(), output_file.c_str());

	this->input_series = new TimeSeriesGenerator(input_file);
	this->output_series = new TimeSeriesGenerator(output_file);

	return true;
}


bool SpotUpwardTransp::initMode(void)
{
	// initialize the reception standard
	// depending on the satellite type
	this->reception_std = new DvbRcsStd(this->pkt_hdl);

	// If available SCPC carriers, a new packet handler is created at NCC
	// to received BBFrames and to be able to deencapsulate GSE packets.
	if(this->checkIfScpc())
	{
		EncapPlugin::EncapPacketHandler *fwd_pkt_hdl;
		// check that the forward encapsulation scheme is GSE
		// (this should be automatically set by the manager)
		if(!this->initPktHdl(FORWARD_DOWN_ENCAP_SCHEME_LIST,
		                     &fwd_pkt_hdl, false))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed to get forward packet handler\n");
			return false;
		}
		if(fwd_pkt_hdl->getName() != "GSE")
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Forward packet handler is not GSE while there is SCPC channels\n");
			return false;
		}

		if(!this->initPktHdl("GSE",
		                     &this->scpc_pkt_hdl, true))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed to get packet handler for receiving GSE packets\n");
			return false;
		}

		this->reception_std_scpc = new DvbScpcStd(this->scpc_pkt_hdl);
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "NCC is aware that there are SCPC carriers available \n");
	}

	if(!this->reception_std)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the reception standard\n");
		return false;
	}

	return true;
}


bool SpotUpwardTransp::initOutput(void)
{
	// Events
	this->event_logon_req = Output::registerEvent("Spot_%d.DVB.logon_request",
	                                                 this->spot_id);

	if(this->saloha)
	{
		this->log_saloha = Output::registerLog(LEVEL_WARNING, "Spot_%d.Dvb.SlottedAloha",
		                                       this->spot_id);
	}

	// Output probes and stats
	this->probe_gw_l2_from_sat=
		Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
		                           "Spot_%d.Throughputs.L2_from_SAT",
		                            this->spot_id);
	this->l2_from_sat_bytes = 0;

	return true;
}


bool SpotUpwardTransp::handleFrame(DvbFrame *frame, NetBurst **burst)
{
	uint8_t msg_type = frame->getMessageType();
	uint8_t corrupted = frame->isCorrupted();
	PhysicStd *std = this->reception_std;
	
	if(msg_type == MSG_TYPE_BBFRAME)
	{
		// decode the first packet in frame to be able to get source terminal ID
		if(!this->reception_std_scpc)
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Got BBFrame in transparent mode, without SCPC on carrier %u\n",
			    frame->getCarrierId());
			return false;
		}
		std = this->reception_std_scpc;
	}
	// TODO factorize if SCPC modcod handling is the same as for regenerative case
	// Update stats
	this->l2_from_sat_bytes += frame->getPayloadLength();

	if(!std->onRcvFrame(frame, this->mac_id, burst))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle DVB frame or BB frame\n");
		return false;
	}
	NetBurst::iterator pkt_it;
	NetBurst *pkt_burst = (*burst);
	if(pkt_burst)
	{
		for(pkt_it = pkt_burst->begin(); pkt_it != pkt_burst->end(); ++pkt_it)
		{
			const NetPacket *packet = (*pkt_it);
			tal_id_t tal_id = packet->getSrcTalId();
			list<tal_id_t>::iterator it_scpc = std::find(this->is_tal_scpc.begin(), 
			                                             this->is_tal_scpc.end(),
				                                         tal_id);
			if(it_scpc != this->is_tal_scpc.end() &&  
			   packet->getDstTalId() == this->mac_id)
			{
				uint32_t opaque = 0;
				if(!this->scpc_pkt_hdl->getHeaderExtensions(packet,
				                                            "deencodeCniExt",
				                                            &opaque))
				{
					LOG(this->log_receive_channel, LEVEL_ERROR,
					    "error when trying to read header extensions\n");
					return false;
				}
				if(opaque != 0)
				{
					// This is the C/N0 value evaluated by the Terminal 
					// and transmitted via GSE extensions
					// TODO we could make specific SCPC function
					this->setRequiredCniOutput(tal_id, ncntoh(opaque)); 
					break;
				}
			}
		}
	}

	// TODO MODCOD should also be updated correctly for SCPC but at the moment
	//      FMT simulations cannot handle this, fix this once this
	//      will be reworked
	if(std->getType() == "DVB-S2")
	{
		DvbS2Std *s2_std = (DvbS2Std *)std;
		if(!corrupted)
		{
			this->probe_received_modcod->put(s2_std->getReceivedModcod());
		}
		else
		{
			this->probe_rejected_modcod->put(s2_std->getReceivedModcod());
		}
	}

	return true;
}

bool SpotUpwardTransp::updateSeriesGenerator(void)
{
	if(!this->input_series || !this->output_series)
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "Cannot update series\n");
		return false;
	}

	if(!this->input_series->add(this->input_sts->getListSts()))
	{
		return false;
	}

	if(!this->output_series->add(this->output_sts->getListSts()))
	{
		return false;
	}
	return true;
}

void SpotUpwardTransp::handleFrameCni(DvbFrame *dvb_frame)
{
	if(!this->with_phy_layer)
	{
		return;
	}

	double curr_cni = dvb_frame->getCn();
	uint8_t msg_type = dvb_frame->getMessageType();
	tal_id_t tal_id;
	
	switch(msg_type)
	{
		// Cannot check frame type because of currupted frame
		case MSG_TYPE_DVB_BURST:
			{
				// transparent case : update return modcod for terminal
				DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();
				// decode the first packet in frame to be able to
				// get source terminal ID
				if(!this->pkt_hdl->getSrc(frame->getPayload(),
				                          tal_id))
				{
					LOG(this->log_receive_channel, LEVEL_ERROR,
					    "unable to read source terminal ID in"
					    " frame, won't be able to update C/N"
					    " value\n");
					return;
				}
				this->setRequiredCniInput(tal_id, curr_cni);
				break;
			}
		case MSG_TYPE_BBFRAME:
			{	
				// SCPC
				BBFrame *frame = dvb_frame->operator BBFrame*();
				// decode the first packet in frame to be able to
				// get source terminal ID
				if(!this->scpc_pkt_hdl->getSrc(frame->getPayload(),
				                               tal_id))
				{
					LOG(this->log_receive_channel, LEVEL_ERROR,
					    "unable to read source terminal ID in"
					    " frame, won't be able to update C/N"
					    " value\n");
					return;
				}
				// TODO we could make specific SCPC function
				this->setRequiredModcod(tal_id, curr_cni, 
				                        this->input_modcod_def_scpc,
				                        this->input_sts);
				break;
			}
		default:
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Wrong message type %u, this shouldn't happened", msg_type);
			break;
	}

	return;
}


bool SpotUpwardTransp::checkIfScpc()
{
	TerminalCategories<TerminalCategoryDama> scpc_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	fmt_groups_t ret_fmt_groups;
	ConfigurationList current_spot;
	
	if(!OpenSandConf::getSpot(RETURN_UP_BAND, this->spot_id,
		                      NO_GW, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value %d and %s with value %d into %s/%s\n",
		    ID, this->spot_id, GW, this->mac_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         RETURN_UP_BAND,
	                                         SCPC,
	                                         // used for checking, no need to get a relevant value
	                                         5,
	                                         TRANSPARENT,
	                                         // we need S2 modcod definitions
	                                         this->output_modcod_def,
	                                         scpc_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         ret_fmt_groups))
	{
		return false;
	}

	// clear unused fmt_group
	for(fmt_groups_t::iterator it = ret_fmt_groups.begin();
	    it != ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}
	
	if(scpc_categories.size() == 0)
	{
		LOG(this->log_init_channel, LEVEL_INFO,
		    "No SCPC carriers\n");
		return false;
	}
	
	// clear unused category
	for(TerminalCategories<TerminalCategoryDama>::iterator it = scpc_categories.begin();
	    it != scpc_categories.end(); ++it)
	{
		delete (*it).second;
	}

	return true;
}

bool SpotUpwardTransp::onRcvLogonReq(DvbFrame *dvb_frame)
{
	if(!SpotUpward::onRcvLogonReq(dvb_frame))
	{
		return false;
	}
	
	LogonRequest *logon_req = (LogonRequest *)dvb_frame;
	uint16_t mac = logon_req->getMac();
	bool is_scpc = logon_req->getIsScpc();
	if(is_scpc)
	{
		this->is_tal_scpc.push_back(mac);
	}
	
	// Inform SlottedAloha
	if(this->saloha)
	{
		if(!this->saloha->addTerminal(mac))
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Cannot add terminal in Slotted Aloha context\n");
			return false;
		}
	}
	
	return true;
}

bool SpotUpwardTransp::scheduleSaloha(DvbFrame *dvb_frame,
                                list<DvbFrame *>* &ack_frames,
                                NetBurst **sa_burst)
{
	if(!this->saloha)
	{
		return true;
	}
	uint16_t sfn;
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	ack_frames = new list<DvbFrame *>();
	// increase the superframe number and reset
	// counter of frames per superframe
	this->super_frame_counter++;
	if(this->super_frame_counter != sfn)
	{
		LOG(this->log_receive_channel, LEVEL_WARNING,
			"superframe counter (%u) is not the same as in"
			" SoF (%u)\n",
			this->super_frame_counter, sfn);
		this->super_frame_counter = sfn;
	}

	if(!this->saloha->schedule(sa_burst,
	                           *ack_frames,
	                           this->super_frame_counter))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to schedule Slotted Aloha\n");
		delete dvb_frame;
		delete ack_frames;
		return false;
	}

	return true;
}

bool SpotUpwardTransp::handleSlottedAlohaFrame(DvbFrame *frame)
{
	// Update stats
	this->l2_from_sat_bytes += frame->getPayloadLength();

	if(!this->saloha->onRcvFrame(frame))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to handle Slotted Aloha frame\n");
		return false;
	}
	return true;
}

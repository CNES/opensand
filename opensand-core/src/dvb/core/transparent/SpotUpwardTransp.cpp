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
 * @file SpotUpwardTransp.cpp
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "SpotUpwardTransp.h"

#include "DamaCtrlRcsLegacy.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Sof.h"

#include "UnitConverterFixedBitLength.h"
#include "UnitConverterFixedSymbolLength.h"

SpotUpwardTransp::SpotUpwardTransp(spot_id_t spot_id,
                                   tal_id_t mac_id,
                                   StFmtSimuList *input_sts,
                                   StFmtSimuList *output_sts):
	SpotUpward(spot_id, mac_id, input_sts, output_sts),
	saloha(NULL),
	is_tal_scpc()
{
}


SpotUpwardTransp::~SpotUpwardTransp()
{
	if(this->saloha)
		delete this->saloha;
}


bool SpotUpwardTransp::onInit(void)
{
	string scheme = RETURN_UP_ENCAP_SCHEME_LIST;

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

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
	UnitConverter *converter;
	int lan_scheme_nbr;

	if(!OpenSandConf::getSpot(RETURN_UP_BAND,
	                          this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value %d into %s/%s\n",
		    GW, this->mac_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}

	if(!this->initBand<TerminalCategorySaloha>(current_spot,
	                                           RETURN_UP_BAND,
	                                           ALOHA,
	                                           this->ret_up_frame_duration_ms,
	                                           this->rcs_modcod_def,
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
		    "Slotted Aloha NCC Initialization failed.\n");
		goto release_saloha;
	}

	if(this->return_link_std == DVB_RCS2)
	{
		vol_sym_t length_sym = 0;
		if(!Conf::getValue(Conf::section_map[COMMON_SECTION],
		                   RCS2_BURST_LENGTH, length_sym))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get '%s' value", DELAY_BUFFER);
			goto release_saloha;
		}
		converter = new UnitConverterFixedSymbolLength(this->ret_up_frame_duration_ms,
		                                               0,
		                                               length_sym
		                                              );
	}
	else
	{
		converter = new UnitConverterFixedBitLength(this->ret_up_frame_duration_ms,
		                                            0,
		                                            this->pkt_hdl->getFixedLength() << 3
		                                           );
	}

	if(!this->saloha->init(sa_categories,
	                       sa_terminal_affectation,
	                       sa_default_category,
	                       this->spot_id,
	                       converter))
	{
		delete converter;
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the Slotted Aloha NCC\n");
		goto release_saloha;
	}
	delete converter;

	return true;

release_saloha:
	delete this->saloha;
	return false;
}


bool SpotUpwardTransp::initModcodSimu(void)
{
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward link definition MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(this->modcod_def_rcs_type.c_str(),
	                            &this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the return link definition MODCOD file\n");
		return false;
	}

	return true;
}

bool SpotUpwardTransp::initMode(void)
{
	// initialize the reception standard
	// depending on the satellite type
	if(this->return_link_std == DVB_RCS2)
	{
		this->reception_std = new DvbRcs2Std(this->pkt_hdl);
	}
	else
	{
		this->reception_std = new DvbRcsStd(this->pkt_hdl);
	}

	// If available SCPC carriers, a new packet handler is created at NCC
	// to received BBFrames and to be able to deencapsulate GSE packets.
	if(this->checkIfScpc())
	{
		EncapPlugin::EncapPacketHandler *fwd_pkt_hdl;
		vector<string> scpc_encap;

		// check that the forward encapsulation scheme is GSE
		// (this should be automatically set by the manager)
		if(!this->initPktHdl(FORWARD_DOWN_ENCAP_SCHEME_LIST,
		                     &fwd_pkt_hdl))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed to get forward packet handler\n");
			return false;
		}
		if (!OpenSandConf::getScpcEncapStack(this->return_link_std_str,
			                                 scpc_encap) ||
			scpc_encap.size() <= 0)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed to get SCPC encapsulation names\n");
			return false;
		}
		if(fwd_pkt_hdl->getName() != scpc_encap[0])
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Forward packet handler is not %s while there is SCPC channels\n",
			    scpc_encap[0].c_str());
			return false;
		}

		if(!this->initScpcPktHdl(&this->scpc_pkt_hdl))
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

bool SpotUpwardTransp::initAcmLoopMargin(void)
{
	double ret_acm_margin_db;
	double fwd_acm_margin_db;
	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   RETURN_UP_ACM_LOOP_MARGIN,
	                   ret_acm_margin_db))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, RETURN_UP_ACM_LOOP_MARGIN);
		return false;
	}

	if(!Conf::getValue(Conf::section_map[PHYSICAL_LAYER_SECTION],
	                   FORWARD_DOWN_ACM_LOOP_MARGIN,
	                   fwd_acm_margin_db))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    PHYSICAL_LAYER_SECTION, FORWARD_DOWN_ACM_LOOP_MARGIN);
		return false;
	}

	this->input_sts->setAcmLoopMargin(ret_acm_margin_db);
	this->output_sts->setAcmLoopMargin(fwd_acm_margin_db);

	return true;
}

bool SpotUpwardTransp::initOutput(void)
{
  auto output = Output::Get();
	// Events
	this->event_logon_req = output->registerEvent("Spot_%d.DVB.logon_request",
                                                this->spot_id);

	if(this->saloha)
	{
		this->log_saloha = output->registerLog(LEVEL_WARNING,
                                           "Spot_%d.Dvb.SlottedAloha",
		                                       this->spot_id);
	}

	// Output probes and stats
	char probe_name[128];
	snprintf(probe_name, sizeof(probe_name), "Spot_%d.Throughputs.L2_from_SAT", this->spot_id);
	this->probe_gw_l2_from_sat = output->registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_AVG);
	this->l2_from_sat_bytes = 0;

	return true;
}


bool SpotUpwardTransp::handleFrame(DvbFrame *frame, NetBurst **burst)
{
	uint8_t msg_type = frame->getMessageType();
	bool corrupted = frame->isCorrupted();
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
			this->probe_rejected_modcod->put(0);
		}
		else
		{
			this->probe_rejected_modcod->put(s2_std->getReceivedModcod());
			this->probe_received_modcod->put(0);
		}
	}

	return true;
}

void SpotUpwardTransp::handleFrameCni(DvbFrame *dvb_frame)
{
	double curr_cni = dvb_frame->getCn();
	uint8_t msg_type = dvb_frame->getMessageType();
	tal_id_t tal_id;

	switch(msg_type)
	{
		// Cannot check frame type because of currupted frame
		case MSG_TYPE_SAC:
		{
			Sac *sac = (Sac *)dvb_frame;
			tal_id = sac->getTerminalId();
			if(!tal_id)
			{
				LOG(this->log_receive_channel, LEVEL_ERROR,
				    "unable to read source terminal ID in"
				    " frame, won't be able to update C/N"
				    " value\n");
				return;
			}
			break;
		}
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
			break;
		}
		default:
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Wrong message type %u, this shouldn't happened", msg_type);
			return;
	}
	this->setRequiredCniInput(tal_id, curr_cni);
}


bool SpotUpwardTransp::checkIfScpc()
{
	TerminalCategories<TerminalCategoryDama> scpc_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	fmt_groups_t ret_fmt_groups;
	ConfigurationList current_spot;

	if(!OpenSandConf::getSpot(RETURN_UP_BAND,
	                          this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no attribute %s with value %d into %s/%s\n",
		    GW, this->mac_id, RETURN_UP_BAND, SPOT_LIST);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         RETURN_UP_BAND,
	                                         SCPC,
	                                         // used for checking, no need to get a relevant value
	                                         5,
	                                         // we need S2 modcod definitions
	                                         this->s2_modcod_def,
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

	if(!(this->input_sts->isStPresent(mac) && this->output_sts->isStPresent(mac)))
	{
		if(!this->addOutputTerminal(mac, this->s2_modcod_def))
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "failed to handle FMT for ST %u, "
			    "won't send logon response\n", mac);
			return false;
		}
	}

	if(is_scpc)
	{
		this->is_tal_scpc.push_back(mac);
		// handle ST for FMT simulation
		if(!(this->input_sts->isStPresent(mac) && this->output_sts->isStPresent(mac)))
		{
			// ST was not registered yet
			if(!this->addInputTerminal(mac, this->s2_modcod_def))
			{
				LOG(this->log_receive_channel, LEVEL_ERROR,
				    "failed to handle FMT for ST %u, "
				    "won't send logon response\n", mac);
				return false;
			}
		}
	}
	else
	{
		// handle ST for FMT simulation
		if(!(this->input_sts->isStPresent(mac) && this->output_sts->isStPresent(mac)))
		{
			// ST was not registered yet
			if(!this->addInputTerminal(mac, this->rcs_modcod_def))
			{
				LOG(this->log_receive_channel, LEVEL_ERROR,
				    "failed to handle FMT for ST %u, "
				    "won't send logon response\n", mac);
				return false;
			}
		}
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

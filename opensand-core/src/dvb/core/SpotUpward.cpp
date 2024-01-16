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
 * @file SpotUpward.cpp
 * @brief Upward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include <sstream>

#include "SpotUpward.h"
#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Sof.h"
#include "Sac.h"
#include "Logon.h"
#include "PhysicStd.h"
#include "NetBurst.h"
#include "SlottedAlohaNcc.h"
#include "TerminalCategoryDama.h"
#include "UnitConverterFixedSymbolLength.h"
#include "OpenSandModelConf.h"

#include <opensand_output/OutputEvent.h>


SpotUpward::SpotUpward(spot_id_t spot_id,
                       tal_id_t mac_id,
                       StackPlugin *upper_encap,
                       std::shared_ptr<StFmtSimuList> input_sts,
                       std::shared_ptr<StFmtSimuList> output_sts):
	DvbChannel{upper_encap, [spot_id](){std::stringstream stream; stream << "gw" << spot_id << ".upward"; return stream.str();}()},
	DvbFmt{},
	spot_id{spot_id},
	mac_id{mac_id},
	saloha{nullptr},
	reception_std{nullptr},
	reception_std_scpc{nullptr},
	scpc_pkt_hdl{nullptr},
	ret_fmt_groups{},
	is_tal_scpc{},
	probe_gw_l2_from_sat{nullptr},
	probe_received_modcod{nullptr},
	probe_rejected_modcod{nullptr},
	log_saloha{nullptr},
	event_logon_req{nullptr}
{
	this->super_frame_counter = 0;
	this->input_sts = input_sts;
	this->output_sts = output_sts;
}


SpotUpward::~SpotUpward()
{
	this->ret_fmt_groups.clear();
}


void SpotUpward::generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane)
{
	SlottedAlohaNcc::generateConfiguration(disable_ctrl_plane);
}


bool SpotUpward::onInit()
{
	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// get the common parameters
	if(!this->initCommon(EncapSchemeList::RETURN_UP))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	// Get and open the files
	if(!this->initModcodSimu())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the files part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initAcmLoopMargin())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the ACM loop margin  part of the initialisation\n");
		return false;
	}

	if(!this->initMode())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	// synchronized with SoF
	this->initStatsTimer(this->ret_up_frame_duration);

	if(!this->initOutput())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		goto error_mode;
	}

	// initialize the slotted Aloha part
	if(!this->initSlottedAloha())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		goto error_mode;
	}

	// everything went fine
	return true;

error_mode:
	this->reception_std = nullptr;
	return false;

}


bool SpotUpward::initSlottedAloha()
{
	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> sa_terminal_affectation;
	std::shared_ptr<TerminalCategorySaloha> sa_default_category;
	vol_sym_t length_sym = 0;

	auto Conf = OpenSandModelConf::Get();
	
	// Skip if the control plane is disabled
	bool ctrl_plane_disabled;
	Conf->getControlPlaneDisabled(ctrl_plane_disabled);
	if (ctrl_plane_disabled)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Control plane disabled: skipping slotted aloha initialization");
		return true;
	}

	OpenSandModelConf::spot current_spot;
	if (!Conf->getSpotReturnCarriers(this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no spot definition attached to the gateway %d\n",
		    this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategorySaloha>(current_spot,
	                                           "return up frequency plan",
	                                           AccessType::ALOHA,
	                                           this->ret_up_frame_duration,
	                                           this->rcs_modcod_def,
	                                           sa_categories,
	                                           sa_terminal_affectation,
	                                           sa_default_category,
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

	auto encap = Conf->getProfileData()->getComponent("encapsulation");
	for(auto& item : encap->getList("lan_adaptation_schemes")->getItems())
	{
		std::string protocol_name;
		auto lan_adaptation_scheme = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);
		if(!OpenSandModelConf::extractParameterData(lan_adaptation_scheme->getParameter("protocol"), protocol_name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "LAN Adaptation Scheme in global section "
				"is missing a protocol name\n");
			return false;
		}

		if(protocol_name == "ROHC")
		{
			LOG(this->log_init_channel, LEVEL_WARNING,
			    "Cannot guarantee no loss with RoHC and Slotted Aloha "
			    "on return link due to interleaving\n");
		}
	}
	// end TODO

	// Create the Slotted ALoha part
	std::unique_ptr<SlottedAlohaNcc> saloha;
	try
	{
		saloha = std::make_unique<SlottedAlohaNcc>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create Slotted Aloha\n");
		return false;
	}

	// Initialize the Slotted Aloha parent class
	// Unlike (future) scheduling, Slotted Aloha get all categories because
	// it also handles received frames and in order to know to which
	// category a frame is affected we need to get source terminal ID
	if(!saloha->initParent(this->ret_up_frame_duration,
	                       // pkt_hdl is the up_ret one because transparent sat
	                       this->pkt_hdl))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Slotted Aloha NCC Initialization failed.\n");
		return false;
	}

	if(!Conf->getRcs2BurstLength(length_sym))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot get 'RCS2 Burst Length' value");
		return false;
	}
	UnitConverterFixedSymbolLength converter{this->ret_up_frame_duration, 0, length_sym};

	if(!saloha->init(sa_categories,
	                 sa_terminal_affectation,
	                 sa_default_category,
	                 this->spot_id,
	                 converter))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the Slotted Aloha NCC\n");
		return false;
	}

	this->saloha = std::move(saloha);
	return true;
}


bool SpotUpward::initModcodSimu()
{
	if(!this->initModcodDefFile(MODCOD_DEF_S2, this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward link definition MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS2,
	                            this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the return link definition MODCOD file\n");
		return false;
	}

	return true;
}


bool SpotUpward::initMode()
{
	// initialize the reception standard
	// depending on the satellite type
	this->reception_std = std::make_unique<DvbRcs2Std>(this->pkt_hdl);

	// If available SCPC carriers, a new packet handler is created at NCC
	// to received BBFrames and to be able to deencapsulate GSE packets.
	if(this->checkIfScpc())
	{
		if(!this->initPktHdl(EncapSchemeList::RETURN_SCPC, this->scpc_pkt_hdl, this->scpc_ctx))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed to get packet handler for receiving GSE packets\n");
			return false;
		}

		this->reception_std_scpc = std::make_unique<DvbScpcStd>(this->scpc_pkt_hdl);
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


bool SpotUpward::initAcmLoopMargin()
{
	double ret_acm_margin_db;
	double fwd_acm_margin_db;
	auto Conf = OpenSandModelConf::Get();

	if(!Conf->getReturnAcmLoopMargin(ret_acm_margin_db))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Section Advanced Links Settings, Return link ACM loop margin missing\n");
		return false;
	}

	if(!Conf->getForwardAcmLoopMargin(fwd_acm_margin_db))
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Section Advanced Links Settings, Forward link ACM loop margin missing\n");
		return false;
	}

	this->input_sts->setAcmLoopMargin(ret_acm_margin_db);
	this->output_sts->setAcmLoopMargin(fwd_acm_margin_db);

	return true;
}


bool SpotUpward::initOutput()
{
	auto output = Output::Get();

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	std::string prefix = generateProbePrefix(spot_id, Component::gateway, is_sat);

	// Events
	this->event_logon_req = output->registerEvent(Format("Spot_%d.DVB.logon_request", this->spot_id));

	if(this->saloha)
	{
		this->log_saloha = output->registerLog(LEVEL_WARNING, Format("Spot_%d.Dvb.SlottedAloha", this->spot_id));
	}

	// Output probes and stats
	this->probe_gw_l2_from_sat = output->registerProbe<int>(prefix + "Throughputs.L2_from_SAT",
	                                                        "Kbits/s", true, SAMPLE_AVG);
	this->l2_from_sat_bytes = 0;

	return true;
}


bool SpotUpward::checkIfScpc()
{
	TerminalCategories<TerminalCategoryDama> scpc_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	std::shared_ptr<TerminalCategoryDama> default_category;
	fmt_groups_t ret_fmt_groups;

	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotReturnCarriers(this->spot_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no spot definition attached to the gateway %d\n",
		    this->spot_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "return up frequency plan",
	                                         AccessType::SCPC,
	                                         // used for checking, no need to get a relevant value
	                                         time_ms_t(5),
	                                         // we need S2 modcod definitions
	                                         this->s2_modcod_def,
	                                         scpc_categories,
	                                         terminal_affectation,
	                                         default_category,
	                                         ret_fmt_groups))
	{
		return false;
	}

	if(scpc_categories.size() == 0)
	{
		LOG(this->log_init_channel, LEVEL_INFO,
		    "No SCPC carriers\n");
		return false;
	}

	return true;
}


void SpotUpward::setFilterTalId(tal_id_t filter)
{
	this->DvbChannel::setFilterTalId(filter);

	for (auto &&context: this->scpc_ctx)
	{
		context->setFilterTalId(filter);
	}
}


bool SpotUpward::handleFrame(Rt::Ptr<DvbFrame> frame, Rt::Ptr<NetBurst> &burst)
{
	EmulatedMessageType msg_type = frame->getMessageType();
	bool corrupted = frame->isCorrupted();
	bool isScpc = false;
	PhysicStd *std = this->reception_std.get();

	if(msg_type == EmulatedMessageType::BbFrame)
	{
		if(!this->reception_std_scpc)
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Got BBFrame in transparent mode, without SCPC on carrier %u\n",
			    frame->getCarrierId());
			return false;
		}
		std = this->reception_std_scpc.get();
		isScpc = true;
	}
	// Update stats
	this->l2_from_sat_bytes += frame->getPayloadLength();

	if(!std->onRcvFrame(std::move(frame), this->mac_id, burst))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle DVB frame or BB frame\n");
		return false;
	}

	if(burst)
	{
		for (auto&& packet : *burst)
		{
			tal_id_t tal_id = packet->getSrcTalId();
			auto it_scpc = std::find(this->is_tal_scpc.begin(),
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
		auto received_modcod = this->reception_std_scpc->getReceivedModcod();
		if(!corrupted)
		{
			this->probe_received_modcod->put(received_modcod);
			this->probe_rejected_modcod->put(0);
		}
		else
		{
			this->probe_rejected_modcod->put(received_modcod);
			this->probe_received_modcod->put(0);
		}
	}

	// check burst validity
	if(!burst)
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "burst is not valid\n");
		return false;
	}

	auto nb_bursts = burst->size();
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "message contains a burst of %d %s packet(s)\n",
	    nb_bursts, burst->name());

	auto &contexts = isScpc ? this->scpc_ctx : this->ctx;

	// iterate on all the deencapsulation contexts to get the ip packets
	for(auto&& context : contexts)
	{
		burst = context->deencapsulate(std::move(burst));
		if(!burst)
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "deencapsulation failed in %s context\n",
			    context->getName());
			return false;
		}
		auto burst_size = burst->size();
		LOG(this->log_receive_channel, LEVEL_INFO,
		    "%d %s packet => %zu %s packet(s)\n",
		    nb_bursts, context->getName(),
		    burst_size, burst->name());
		nb_bursts = burst_size;
	}

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "burst of deencapsulated packets sent to the upper layer\n");
	return true;
}


void SpotUpward::handleFrameCni(DvbFrame& dvb_frame)
{
	double curr_cni = dvb_frame.getCn();
	EmulatedMessageType msg_type = dvb_frame.getMessageType();
	tal_id_t tal_id;

	switch(msg_type)
	{
		// Cannot check frame type because of currupted frame
		case EmulatedMessageType::Sac:
		{
			Sac& sac = dvb_frame_upcast<Sac>(dvb_frame);
			tal_id = sac.getTerminalId();
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
		case EmulatedMessageType::DvbBurst:
		{
			// transparent case : update return modcod for terminal
			DvbRcsFrame& frame = dvb_frame_upcast<DvbRcsFrame>(dvb_frame);
			// decode the first packet in frame to be able to
			// get source terminal ID
			if(!this->pkt_hdl->getSrc(frame.getPayload(), tal_id))
			{
				LOG(this->log_receive_channel, LEVEL_ERROR,
				    "unable to read source terminal ID in"
				    " frame, won't be able to update C/N"
				    " value\n");
				return;
			}
			break;
		}
		case EmulatedMessageType::BbFrame:
		{
			// SCPC
			BBFrame& frame = dvb_frame_upcast<BBFrame>(dvb_frame);
			// decode the first packet in frame to be able to
			// get source terminal ID
			if(!this->scpc_pkt_hdl->getSrc(frame.getPayload(), tal_id))
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


bool SpotUpward::onRcvLogonReq(DvbFrame& dvb_frame)
{
	LogonRequest& logon_req = dvb_frame_upcast<LogonRequest>(dvb_frame);
	uint16_t mac = logon_req.getMac();

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "Logon request from ST%u on spot %u\n", mac, this->spot_id);

	// refuse to register a ST with same MAC ID as the NCC
	// or if it's a gw
	if(OpenSandModelConf::Get()->isGw(mac) or mac == this->mac_id)
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "a ST wants to register with the MAC ID of the NCC "
		    "(%d), reject its request!\n", mac);
		return false;
	}

	// send the corresponding event
	event_logon_req->sendEvent("Logon request received from ST%u on spot %u",
	                           mac, this->spot_id);

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

	if(logon_req.getIsScpc())
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
		LOG(this->log_receive_channel, LEVEL_INFO,
		    "Registered ST%u on spot %u as an SCPC terminal",
		    mac, this->spot_id);
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
		LOG(this->log_receive_channel, LEVEL_INFO,
		    "Registered ST%u on spot %u as a regular terminal",
		    mac, this->spot_id);
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


void SpotUpward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}
	this->probe_gw_l2_from_sat->put(
		time_ms_t(this->l2_from_sat_bytes * 8) / this->stats_period_ms);
	this->l2_from_sat_bytes = 0;

	// Send probes
	Output::Get()->sendProbes();
}


bool SpotUpward::scheduleSaloha(Rt::Ptr<DvbFrame> dvb_frame,
                                Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> &ack_frames,
                                Rt::Ptr<NetBurst> &sa_burst)
{
	if(!this->saloha)
	{
		return true;
	}

	if (dvb_frame)
	{
		uint16_t sfn;
		auto sof = dvb_frame_upcast<Sof>(std::move(dvb_frame));
		sfn = sof->getSuperFrameNumber();

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
	}

	ack_frames = Rt::make_ptr<std::list<Rt::Ptr<DvbFrame>>>();
	if(!this->saloha->schedule(sa_burst,
	                           *ack_frames,
	                           this->super_frame_counter))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to schedule Slotted Aloha\n");
		return false;
	}

	return true;
}


bool SpotUpward::handleSlottedAlohaFrame(Rt::Ptr<DvbFrame> frame)
{
	// Update stats
	this->l2_from_sat_bytes += frame->getPayloadLength();

	if(!this->saloha->onRcvFrame(std::move(frame)))
	{
		LOG(this->log_saloha, LEVEL_ERROR,
		    "failed to handle Slotted Aloha frame\n");
		return false;
	}
	return true;
}


bool SpotUpward::handleSac(DvbFrame &dvb_frame)
{
	Sac& sac = dvb_frame_upcast<Sac>(dvb_frame);

	// transparent : the C/N0 of forward link
	double cni = sac.getCni();
	tal_id_t tal_id = sac.getTerminalId();
	this->setRequiredCniOutput(tal_id, cni);
	LOG(this->log_receive_channel, LEVEL_INFO,
	    "handle received SAC from terminal %u with cni %f\n",
	    tal_id, cni);
	
	return true;
}



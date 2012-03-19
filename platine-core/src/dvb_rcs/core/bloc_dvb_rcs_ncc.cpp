/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file bloc_dvb_rcs_ncc.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/times.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>

#include "bloc_dvb_rcs_ncc.h"

#include "lib_dama_ctrl_stub.h"
#include "lib_dama_ctrl_yes.h"
#include "lib_dama_ctrl_legacy.h"
#include "lib_dama_ctrl_uor.h"

#include "msg_dvb_rcs.h"
#include "DvbRcsStd.h"
#include "DvbS2Std.h"

#include "AtmCell.h"
#include "MpegPacket.h"
#include "GsePacket.h"

// environment plane
#include "platine_env_plane/EnvironmentAgent_e.h"
extern T_ENV_AGENT EnvAgent;

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS_NCC
#include "platine_conf/uti_debug.h"


/**
 * Constructor
 */
BlocDVBRcsNcc::BlocDVBRcsNcc(mgl_blocmgr *blocmgr,
                             mgl_id fatherid,
                             const char *name):
                             BlocDvb(blocmgr, fatherid, name),
                             NccPepInterface(),
                             complete_dvb_frames()
{
	this->init_ok = false;

	this->macId = DVB_GW_MAC_ID;

	// DAMA controller
	this->m_pDamaCtrl = NULL;

	// carrier IDs
	this->m_carrierIdDvbCtrl = -1;
	this->m_carrierIdSOF = -1;
	this->m_carrierIdData = -1;

	// superframes and frames
	this->super_frame_counter = -1;
	this->frame_counter = 0;
	this->m_frameTimer = -1;

	// DVB-RCS/S2 emulation
	this->emissionStd = NULL;
	this->receptionStd = NULL;
	this->scenario_timer = -1;

	/* NGN network / Policy Enforcement Point (PEP) */
	this->pep_cmd_apply_timer = -1;
	this->pepAllocDelay = -1;
}


/**
 * Destructor
 */
BlocDVBRcsNcc::~BlocDVBRcsNcc()
{
	if(this->m_pDamaCtrl != NULL)
		delete this->m_pDamaCtrl;
	if(this->emissionStd != NULL)
		delete this->emissionStd;
	if(this->receptionStd != NULL)
		delete this->receptionStd;

	this->complete_dvb_frames.clear();

	if(this->m_bbframe != NULL)
		delete this->m_bbframe;
}


/**
 * @brief The event handler
 *
 * @param event  the received event to handle
 * @return       mgl_ok if the event was correctly handled, mgl_ko otherwise
 */
mgl_status BlocDVBRcsNcc::onEvent(mgl_event *event)
{
	const char *FUNCNAME = DBG_PREFIX "[onEvent]";
	mgl_status status = mgl_ko;
	int ret;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->init_ok)
		{
			UTI_ERROR("%s bloc has already been initialized, ignore init event\n",
			          FUNCNAME);
		}
		else if(this->onInit() < 0)
		{
			UTI_ERROR("%s bloc initialization failed\n", FUNCNAME);
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT);
		}
		else
		{
			this->init_ok = true;
			status = mgl_ok;
		}
	}
	else if(!this->init_ok)
	{
		UTI_ERROR("DVB-RCS SAT bloc not initialized, "
		          "ignore non-init event\n");
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer() &&
		   MGL_EVENT_MSG_IS_TYPE(event, msg_dvb))
		{
			// messages from lower layer: dvb frames
			T_DVB_META *dvb_meta;
			long carrier_id;
			unsigned char *frame;
			int l_len;

			dvb_meta = (T_DVB_META *) MGL_EVENT_MSG_GET_BODY(event);
			carrier_id = dvb_meta->carrier_id;
			frame = (unsigned char *) dvb_meta->hdr;
			l_len = MGL_EVENT_MSG_GET_BODYLEN(event);

			UTI_DEBUG("[onEvent] DVB frame received\n");
			if(this->onRcvDVBFrame(frame, l_len) < 0)
			{
				status = mgl_ko;
			}
			else
			{
				status = mgl_ok;
			}
			// TODO: release frame ?
			g_memory_pool_dvb_rcs.release((char *) dvb_meta);
		}
		else if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getUpperLayer() &&
		        MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
		{
			NetBurst *burst;
			NetBurst::iterator pkt_it;

			burst = (NetBurst *) MGL_EVENT_MSG_GET_BODY(event);

			UTI_DEBUG("SF#%ld: encapsulation burst received "
			          "(%d packet(s))\n", this->super_frame_counter,
			          burst->length());

			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end();
			    pkt_it++)
			{
				UTI_DEBUG("SF#%ld: store one encapsulation "
				          "packet\n", this->super_frame_counter);

				if(this->emissionStd->onRcvEncapPacket(*pkt_it,
				                                       &this->data_dvb_fifo,
				                                       getCurrentTime(),
				                                       0) < 0)
				{
					// a problem occured => trace it but
					// carry on simulation
					UTI_ERROR("SF#%ld: unable to store received "
					          "encapsulation packet "
					          "(see previous errors)\n",
					          this->super_frame_counter);
				}

				(*pkt_it)->addTrace(HERE());

				UTI_DEBUG("SF#%ld: encapsulation packet is "
				          "successfully stored\n",
				          this->super_frame_counter);
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;

			status = mgl_ok;
		}
		else
		{
			UTI_ERROR("SF#%ld: unknown message event received\n",
			          this->super_frame_counter);
		}
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		status = mgl_ok;

		// receive the frame Timer event
		UTI_DEBUG_L3("timer event received\n");

		if(MGL_EVENT_TIMER_IS_TIMER(event, this->m_frameTimer))
		{
			status = mgl_ok;

			// Set the timer again for SOF
			// must be very careful with the set timer call !
			//     - at the beginnig of the treatment -> more precise frame
			//       duration, but in case of overrun the frame is lost
			//     - at the end of the treatment -> the frame duration is
			//       theorical frame duration + treatment duration !!!
			this->setTimer(this->m_frameTimer, this->frame_duration);

			// increment counter of frames per superframe
			this->frame_counter++;

			// if we reached the end of a superframe and the
			// beginning of a new one, send SOF and run allocation
			// algorithms (DAMA)
			if(this->frame_counter == this->frames_per_superframe)
			{
				// increase the superframe number and reset
				// counter of frames per superframe
				this->super_frame_counter++;
				this->frame_counter = 0;

				// send Start Of Frame (SOF)
				this->sendSOF();

				// run the allocation algorithms (DAMA)
				this->m_pDamaCtrl->runOnSuperFrameChange(this->super_frame_counter);

				// send TBTP computed by DAMA
				this->sendTBTP();
			}

			// schedule encapsulation packets
			if(this->emissionStd->scheduleEncapPackets(&this->data_dvb_fifo,
			                                           this->getCurrentTime(),
			                                           &this->complete_dvb_frames) != 0)
			{
				UTI_ERROR("failed to schedule encapsulation "
				          "packets stored in DVB FIFO\n");
				status = mgl_ko;
			}

			if(status != mgl_ko &&
			   this->sendBursts(&this->complete_dvb_frames, this->m_carrierIdData) != 0)
			{
				UTI_ERROR("failed to build and send DVB/BB frames\n");
				status = mgl_ko;
			}
		}
		else if(MGL_EVENT_TIMER_IS_TIMER(event, this->scenario_timer))
		{
			// it's time to update MODCOD and DRA scheme IDs
			UTI_DEBUG_L3("MODCOD/DRA scenario timer received\n");

			// set the timer again
			this->setTimer(this->scenario_timer, this->dvb_scenario_refresh);

			if(!this->emissionStd->goNextStScenarioStep())
			{
				UTI_ERROR("SF#%ld: failed to update MODCOD "
				          "or DRA scheme IDs\n",
				          this->super_frame_counter);
			}
			else
			{
				UTI_DEBUG_L3("SF#%ld: MODCOD and DRA scheme IDs "
				             "successfully updated\n",
				             this->super_frame_counter);
				status = mgl_ok;
			}
		}
		else if(MGL_EVENT_TIMER_IS_TIMER(event, this->pep_cmd_apply_timer))
		{
			// it is time to apply the command sent by the external
			// PEP component

			PepRequest *pep_request;

			UTI_INFO("apply PEP requests now\n");
			while((pep_request = this->getNextPepRequest()) != NULL)
			{
				if(m_pDamaCtrl->applyPepCommand(pep_request))
				{
					UTI_INFO("PEP request successfully "
					         "applied in DAMA\n");
				}
				else
				{
					UTI_ERROR("failed to apply PEP request "
					          "in DAMA\n");
				}
			}

			status = mgl_ok;
		}
		else
		{
			UTI_ERROR("%s unknown timer event received\n", FUNCNAME);
		}
	}
	else if(MGL_EVENT_IS_FD(event))
	{
		if(MGL_EVENT_FD_GET_FD(event) == this->getPepListenSocket())
		{
			// event received on PEP listen socket
			UTI_INFO("event received on PEP listen socket\n");

			// create the client socket to receive messages
			ret = acceptPepConnection();
			if(ret == 0)
			{
				UTI_INFO("NCC is now connected to PEP\n");
				// add a fd to handle events on the client socket
				this->addFd(this->getPepClientSocket());
			}
			else if(ret == -1)
			{
				UTI_NOTICE("failed to accept new connection "
				           "request from PEP\n");
			}
			else if(ret == -2)
			{
				UTI_NOTICE("one PEP already connected: "
				           "reject new connection request\n");
			}
			else
			{
				UTI_ERROR("unknown status %d from "
				          "acceptPepConnection()\n", ret);
			}

			status = mgl_ok;
		}
		else if(MGL_EVENT_FD_GET_FD(event) == this->getPepClientSocket())
		{
			// event received on PEP client socket
			UTI_INFO("event received on PEP client socket\n");

			// read the message sent by PEP or delete socket
			// if connection is dead
			if(this->readPepMessage() == true)
			{
				// we have received a set of commands from the
				// PEP component, let's apply the resources
				// allocations/releases they contain

				// set delay for applying the commands
				if(this->getPepRequestType() == PEP_REQUEST_ALLOCATION)
				{
					this->setTimer(this->pep_cmd_apply_timer,
					               pepAllocDelay);
					UTI_INFO("PEP Allocation request, apply a %dms delay\n", pepAllocDelay);
				}
				else if(this->getPepRequestType() == PEP_REQUEST_RELEASE)
				{
					this->setTimer(this->pep_cmd_apply_timer, 0);
					UTI_INFO("PEP Release request, no delay to apply\n");
				}
				else
				{
					UTI_ERROR("cannot determine request type!\n");
				}
			}
			else
			{
				UTI_NOTICE("network problem encountered with PEP, "
				           "connection was therefore closed\n");
				this->removeFd(this->getPepClientSocket());
			}

			status = mgl_ok;
		}
	}
	else
	{
		UTI_ERROR("%s unknown event received\n", FUNCNAME);
		status = mgl_ko;
	}

	return status;
}


/**
 * Read configuration when receive the init event
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsNcc::onInit()
{
	const char *FUNCNAME = DBG_PREFIX "[onInit]";
	int ret;
	long simu_column_num;
	mgl_msg *link_up_msg;
	T_LINK_UP *link_is_up;

	// get the common parameters
	if(!this->initCommon())
	{
		UTI_ERROR("failed to complete the common part of the initialisation");
		goto error;
	}

	// initialize the timers
	ret = this->initTimers();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the timers part of the "
		          "initialisation");
		goto error;
	}

	ret = this->initMode();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the mode part of the "
		          "initialisation");
		goto error;
	}

	// read the uplink encapsulation packet length
	ret = this->initEncap();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the encapsulation part of the "
		          "initialisation");
		goto error_mode;
	}

	// Get the carrier Ids
	ret = this->initCarrierIds();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the carrier IDs part of the "
		          "initialisation");
		goto error_mode;
	}

	// Get and open the files
	ret = this->initFiles();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the files part of the "
		          "initialisation");
		goto error_mode;
	}

	// get and launch the dama algorithm
	ret = initDama();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the DAMA part of the "
		          "initialisation");
		goto error_mode;
	}

	ret = initFifo();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the FIFO part of the "
		          "initialisation");
		goto release_dama;
	}

	// Set #sf and launch frame timer
	this->super_frame_counter = 0;
	setTimer(m_frameTimer, this->frame_duration);

	// Launch the timer in order to retrieve the modcods
	setTimer(this->scenario_timer, this->dvb_scenario_refresh);

	// get the column number for GW in MODCOD/DRA simulation files
	if(!globalConfig.getValueInList(DVB_SIMU_COL, COLUMN_LIST,
	                                TAL_ID, toString(DVB_GW_MAC_ID),
                                    COLUMN_NBR, simu_column_num))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_SIMU_COL, COLUMN_LIST);
		goto release_dama;
	}
	if(simu_column_num <= 0 || simu_column_num > NB_MAX_ST)
	{
		UTI_ERROR("section '%s': invalid value %ld for parameter "
		          "'%s'\n", DVB_SIMU_COL, simu_column_num,
		          COLUMN_NBR);
		goto release_dama;
	}

	// declare the GW as one ST for the MODCOD/DRA scenarios
	if(!this->emissionStd->addSatelliteTerminal(DVB_GW_MAC_ID,
	                                            simu_column_num))
	{
		UTI_ERROR("failed to define the GW as ST with ID %ld\n",
		          DVB_GW_MAC_ID);
		goto release_dama;
	}

	// allocate memory for the BB frame if DVB-S2 standard is used
	if(this->emissionStd->type() == "DVB-S2" ||
	   this->receptionStd->type() == "DVB-S2")
	{
		this->m_bbframe = new std::map<int, T_DVB_BBFRAME *>;
		if(this->m_bbframe == NULL)
		{
			UTI_ERROR("%s SF#%ld: failed to allocate memory for the BB "
			          "frame\n", FUNCNAME, this->super_frame_counter);
			goto release_dama;
		}
	}

	// create and send a "link is up" message to upper layer
	link_is_up = new T_LINK_UP;
	if(link_is_up == NULL)
	{
		UTI_ERROR("%s SF#%ld: failed to allocate memory for link_is_up "
		          "message\n", FUNCNAME, this->super_frame_counter);
		goto release_dama;
	}
	link_is_up->group_id = 0;
	link_is_up->tal_id = DVB_GW_MAC_ID;

	link_up_msg = newMsgWithBodyPtr(msg_link_up, link_is_up, sizeof(T_LINK_UP));
	if(link_up_msg == NULL)
	{
		UTI_ERROR("%s SF#%ld Failed to allocate a mgl msg.\n", FUNCNAME,
		          this->super_frame_counter);
		goto release_dama;
	}

	this->sendMsgTo(this->getUpperLayer(), link_up_msg);
	UTI_DEBUG_L3("%s SF#%ld Link is up msg sent to upper layer\n", FUNCNAME,
	             this->super_frame_counter);

	// listen for connections from external PEP components
	if(this->listenForPepConnections() != true)
	{
		UTI_ERROR("failed to listen for PEP connections\n");
		goto release_dama;
	}
	this->addFd(this->getPepListenSocket());

	// everything went fine
	return 0;

release_dama:
	delete m_pDamaCtrl;
error_mode:
	// TODO: release the emission and reception standards here
error:
	return -1;
}


/**
 * Read configuration for the different timers
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsNcc::initTimers()
{
	int val;

	// read the pep allocation delay
	if(!globalConfig.getValue(NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          NCC_SECTION_PEP, DVB_NCC_ALLOC_DELAY);
		goto error;
	}
	this->pepAllocDelay = val;
	UTI_INFO("pepAllocDelay set to %d ms\n", this->pepAllocDelay);

	return 0;

error:
	return -1;
}


/**
 * @brief Initialize the transmission mode
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsNcc::initMode()
{
	// initialize the emission and reception standards depending
	// on the satellite type
	if(this->satellite_type == TRANSPARENT_SATELLITE)
	{
		this->emissionStd = new DvbS2Std();
		this->receptionStd = new DvbRcsStd();
		// set the terminal ID in emission and reception standards
		// to -1 because the GW should handle all the packets in
		// transparent mode
		this->receptionStd->setTalId(-1);
		this->emissionStd->setTalId(-1);
	}
	else if(this->satellite_type == REGENERATIVE_SATELLITE)
	{
		this->emissionStd = new DvbRcsStd();
		this->receptionStd = new DvbS2Std();
		// set the terminal ID in emission and reception standards
		this->receptionStd->setTalId(DVB_GW_MAC_ID);
		this->emissionStd->setTalId(DVB_GW_MAC_ID);
	}
	else
	{
		UTI_ERROR("section '%s': unknown value '%s' for parameter "
		          "'%s'\n", GLOBAL_SECTION, this->satellite_type.c_str(),
		          SATELLITE_TYPE);
		goto error;

	}
	if(this->emissionStd == NULL)
	{
		UTI_ERROR("failed to create the emission standard\n");
		goto release_standards;
	}
	if(this->receptionStd == NULL)
	{
		UTI_ERROR("failed to create the reception standard\n");
		goto release_standards;
	}

	// set frame duration in emission standard
	this->emissionStd->setFrameDuration(this->frame_duration);

	return 0;

release_standards:
	if(this->emissionStd != NULL)
	  delete this->emissionStd;
	if(this->receptionStd != NULL)
	  delete this->receptionStd;
error:
	return -1;
}


/**
 * @brief Read configuration for the encapsulation protocol
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsNcc::initEncap()
{
    string out_encap_scheme;
	int encap_packet_type = PKT_TYPE_INVALID;

	// read the satellite type
	if(this->satellite_type == REGENERATIVE_SATELLITE)
	{
		out_encap_scheme = this->up_return_encap_scheme;
	}
	else
	{
		out_encap_scheme = this->down_forward_encap_scheme;
	}
	UTI_INFO("output encapsulation scheme = %s\n",
	         out_encap_scheme.c_str());

	// if the DVB-RCS standard is used for emission, the frame length
	// is constant and the maximum number of packets per frame can
	// be computed from the length of a packet
	if(this->emissionStd->type() == "DVB-RCS")
	{
		if(out_encap_scheme == ENCAP_ATM_AAL5 ||
		   out_encap_scheme == ENCAP_ATM_AAL5_ROHC)
		{
			// DVB-RCS frames encapsulate ATM cells
			encap_packet_type = PKT_TYPE_ATM;
		}
		else if(out_encap_scheme == ENCAP_MPEG_ULE ||
		        out_encap_scheme == ENCAP_MPEG_ULE_ROHC)
		{
			// DVB-RCS frames encapsulate MPEG packets
			encap_packet_type = PKT_TYPE_MPEG;
		}
		else
		{
			UTI_ERROR("bad value for output encapsulation scheme "
			          "with emission standard %s, check the value "
			          "of parameter of encapsulation schemes in section "
                      "'%s'\n", this->emissionStd->type().c_str(),
			          GLOBAL_SECTION);
 			goto error;
		}
	}
	else if(this->emissionStd->type() == "DVB-S2")
	{
		if(out_encap_scheme == ENCAP_MPEG_ULE ||
		   out_encap_scheme == ENCAP_MPEG_ULE_ROHC)
		{
			// DVB-RCS frames encapsulate MPEG packets
			encap_packet_type = PKT_TYPE_MPEG;
		}
		else if(out_encap_scheme == ENCAP_GSE ||
		        out_encap_scheme == ENCAP_GSE_ROHC)
		{
			// DVB-RCS frames encapsulate GSE packets
			encap_packet_type = PKT_TYPE_GSE;
		}
		else
		{
			UTI_ERROR("bad value for output encapsulation scheme "
			          "with emission standard %s, check the value "
			          "of parameter of encapsulation schemes in section "
                      "'%s'\n", this->emissionStd->type().c_str(),
			          GLOBAL_SECTION);
			goto error;
		}
	}
	else
	{
		UTI_ERROR("bad emission standard %s\n",
		          this->emissionStd->type().c_str());
		goto error;
	}

	// set the encapsulation packet type for emission standard
	this->emissionStd->setEncapPacketType(encap_packet_type);

	return 0;

error:
	return -1;
}


/**
 * Read configuration for the carrier IDs
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsNcc::initCarrierIds()
{
	int val;

	// Get the carrier Id m_carrierIdDvbCtrl
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_CTRL_CAR, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_CTRL_CAR);
		goto error;
	}
	this->m_carrierIdDvbCtrl = val;
	UTI_INFO("carrierIdDvbCtrl set to %ld\n", this->m_carrierIdDvbCtrl);

	// Get the carrier Id m_carrierIdSOF
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SOF_CAR, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_SOF_CAR);
		goto error;
	}
	this->m_carrierIdSOF = val;
	UTI_INFO("carrierIdSOF set to %ld\n", this->m_carrierIdSOF);

	// Get the carrier Id m_carrierIdData
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_DATA_CAR, val))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_DATA_CAR);
	goto error;
	}
	this->m_carrierIdData = val;
	this->data_dvb_fifo.setId(m_carrierIdData);
	UTI_INFO("carrierIdData set to %ld\n", this->m_carrierIdData);

	return 0;

error:
	return -1;
}


/**
 * @brief Read configuration for the different files and open them
 *
 * @return  0 in case of succeed, -1 otherwise
 */
int BlocDVBRcsNcc::initFiles()
{
	if(this->emissionStd->type() == "DVB-S2")
	{
		if(this->initDraFiles() != 0)
		{
			UTI_ERROR("failed to initialize the DRA scheme files\n");
			goto error;
		}
	}

	if(this->emissionStd->type() == "DVB-S2")
	{
		if(this->initModcodFiles() != 0)
		{
			UTI_ERROR("failed to initialize the MODCOD files\n");
			goto error;
		}
	}

	// initialize the MODCOD and DRA scheme IDs
	if(!this->emissionStd->goNextStScenarioStep())
	{
		UTI_ERROR("failed to initialize MODCOD or DRA scheme IDs\n");
		goto error;
	}

	return 0;

error:
	return -1;
}

/**
 * @brief Read configuration for the DRA scheme definition/simulation files
 *
 * @return  -1 if failed, 0 if succeed
 */
int BlocDVBRcsNcc::initDraFiles()
{
	std::string dra_def_file;
	std::string dra_simu_file;

	// build the path to the DRA scheme definition file
	dra_def_file = MODCOD_DRA_PATH;
	dra_def_file += "/";
	dra_def_file += this->dvb_scenario;
	dra_def_file += "/def_dra_scheme.txt";

	if(access(dra_def_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
				  dra_def_file.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("DRA scheme definition file = '%s'\n", dra_def_file.c_str());

	// load all the DRA scheme definitions from file
	if(!dynamic_cast<DvbS2Std *>( this->emissionStd)->loadDraSchemeDefinitionFile(dra_def_file))
	{
		goto error;
	}

	// build the path to the DRA scheme simulation file
	dra_simu_file += MODCOD_DRA_PATH;
	dra_simu_file += "/";
	dra_simu_file += this->dvb_scenario;
	dra_simu_file += "/sim_dra_scheme.txt";

	if(access(dra_simu_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
				  dra_simu_file.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("DRA scheme simulation file = '%s'\n", dra_simu_file.c_str());

	// associate the simulation file with the list of STs
	if(!dynamic_cast<DvbS2Std *>(this->emissionStd)->loadDraSchemeSimulationFile(dra_simu_file))
	{
		goto error;
	}

	return 0;

error:
	return -1;
}


/**
 * Read configuration for the DAMA algorithm
 *
 * @return  -1 if failed, 0 if succeed
 */
int BlocDVBRcsNcc::initDama()
{
	string up_return_encap_proto;
	int st_encap_packet_length;
	int ret;

	/* select the specified DAMA algorithm */
	if(this->dama_algo == "Legacy")
	{
		UTI_INFO("creating Legacy DAMA controller\n");
		this->m_pDamaCtrl = new DvbRcsDamaCtrlLegacy();

	}
	else if(this->dama_algo == "UoR")
	{
		UTI_INFO("creating UoR DAMA controller\n");
		this->m_pDamaCtrl = new DvbRcsDamaCtrlUoR();
	}
	else if(this->dama_algo == "Yes")
	{
		UTI_INFO("creating Yes DAMA controller\n");
		this->m_pDamaCtrl = new DvbRcsDamaCtrlYes();
	}
	else if(this->dama_algo == "Stub" || this->dama_algo == "None")
	{
		UTI_INFO("creating Stub DAMA controller\n");
		this->m_pDamaCtrl = new DvbRcsDamaCtrlStub();
	}
	else
	{
		UTI_ERROR("section '%s': bad value for parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_NCC_DAMA_ALGO);
		goto error;
	}

	if(this->m_pDamaCtrl == NULL)
	{
		UTI_ERROR("failed to create the DAMA controller\n");
		goto error;
	}

	// retrieve the output encapsulation scheme
	if(!globalConfig.getValue(GLOBAL_SECTION, UP_RETURN_ENCAP_SCHEME,
	                          up_return_encap_proto))
	{
		UTI_ERROR("section '%s': bad value for parameter '%s'\n",
		          DVB_NCC_SECTION, UP_RETURN_ENCAP_SCHEME);
		goto release_dama;
	}

	if(up_return_encap_proto == ENCAP_ATM_AAL5 ||
	   up_return_encap_proto == ENCAP_ATM_AAL5_ROHC)
	{
		// return link: DVB-RCS frames encapsulate ATM cells
		st_encap_packet_length = AtmCell::length();
	}
	else if(up_return_encap_proto == ENCAP_MPEG_ULE ||
	        up_return_encap_proto == ENCAP_MPEG_ULE_ROHC)
	{
		// return link: DVB-RCS frames encapsulate MPEG packets
		st_encap_packet_length = MpegPacket::length();
	}
	else
	{
		UTI_ERROR("bad value for st output encapsulation scheme\n");
		goto release_dama;
	}

	// initialize the DAMA controller
	if(this->emissionStd->type() == "DVB-S2")
	{
		ret = this->m_pDamaCtrl->init(
				m_carrierIdDvbCtrl,
				this->frame_duration,
				this->frames_per_superframe,
				st_encap_packet_length,
				dynamic_cast<DvbS2Std *>(this->emissionStd)->getDraSchemeDefinitions());
	}
	else
	{
		ret = this->m_pDamaCtrl->init(m_carrierIdDvbCtrl,
		                              this->frame_duration,
		                              this->frames_per_superframe,
		                              st_encap_packet_length,
		                              0);
	}
	if(ret != 0)
	{
		UTI_ERROR("failed to initialize the DAMA controller\n");
		goto release_dama;
	}

	return 0;

release_dama:
	delete this->m_pDamaCtrl;
error:
	return -1;
}


/**
 * @brief Read configuration for the FIFO
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsNcc::initFifo()
{
	int val;

	// retrieve and set FIFO size
	if(!globalConfig.getValue(DVB_NCC_SECTION, DVB_SIZE_FIFO, val))
	{
		UTI_ERROR("section '%s': bad value for parameter '%s'\n",
		          DVB_NCC_SECTION, DVB_SIZE_FIFO);
		goto error;
	}
	this->data_dvb_fifo.init(val);

	return 0;

error:
	return -1;
}



/******************* EVENT MANAGEMENT *********************/


int BlocDVBRcsNcc::onRcvDVBFrame(unsigned char *data, int len)
{
	const char *FUNCNAME = DBG_PREFIX "[onRcvDVBFrame]";
	T_DVB_HDR *dvb_hdr;

	// get DVB header
	dvb_hdr = (T_DVB_HDR *) data;

	switch(dvb_hdr->msg_type)
	{
		// ATM/MPEG/GSE burst
		case MSG_TYPE_DVB_BURST:
		case MSG_TYPE_BBFRAME:
		{
			// ignore BB frames in transparent scenario
			// (this is required because the GW may receive BB frames
			//  in transparent scenario due to carrier emulation)

			NetBurst *burst;

			if(this->receptionStd->type() == "DVB-RCS" &&
			   dvb_hdr->msg_type == MSG_TYPE_BBFRAME)
			{
				UTI_DEBUG("ignore received BB frame in transparent scenario\n");
				goto drop;
			}
			if(this->receptionStd->onRcvFrame(data, len, dvb_hdr->msg_type,
			                                  this->macId, &burst) < 0)
			{
				UTI_ERROR("failed to handle ATM/MPEG DVB frame "
				          "or BB frame\n");
				goto error;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}
		}
		break;

		case MSG_TYPE_CR:
		{
			T_DVB_SAC_CR_INFO * cr_info;
			unsigned int dra_id;

			UTI_DEBUG_L3("handle received Capacity Request (CR)\n");

			// retrieve the current DRA scheme for the ST
			cr_info = (T_DVB_SAC_CR_INFO *) (data + sizeof(T_DVB_HDR));
			dra_id = this->emissionStd->getStCurrentDraSchemeId(cr_info->logon_id);

			if(this->m_pDamaCtrl->hereIsCR(data, len, dra_id) != 0)
			{
				UTI_ERROR("failed to handle Capacity Request "
				          "(CR) frame\n");
				goto error;
			}
			g_memory_pool_dvb_rcs.release((char *) data);
		}
		break;

		case MSG_TYPE_SACT:
			UTI_DEBUG_L3("%s SACT\n", FUNCNAME);
			m_pDamaCtrl->hereIsSACT(data, len);
			g_memory_pool_dvb_rcs.release((char *) data);
			break;

		case MSG_TYPE_SESSION_LOGON_REQ:
			UTI_DEBUG("%s Logon Req\n", FUNCNAME);
			onRcvLogonReq(data, len);
			break;

		case MSG_TYPE_SESSION_LOGOFF:
			UTI_DEBUG_L3("%s Logoff Req\n", FUNCNAME);
			onRcvLogoffReq(data, len);
			break;

		case MSG_TYPE_TBTP:
		case MSG_TYPE_SESSION_LOGON_RESP:
		case MSG_TYPE_SOF:
			// nothing to do in this case
			UTI_DEBUG_L3("ignore TBTP, logon response or SOF frame "
			             "(type = %ld)\n", dvb_hdr->msg_type);
			g_memory_pool_dvb_rcs.release((char *) data);
			break;

		default:
			UTI_ERROR("unknown type (%ld) of DVB frame\n",
			          dvb_hdr->msg_type);
			g_memory_pool_dvb_rcs.release((char *) data);
			break;
	}

	return 0;

drop:
	g_memory_pool_dvb_rcs.release((char *) data);
	return 0;

error:
	UTI_ERROR("Treatments failed at SF# %ld\n",
	          this->super_frame_counter);
	return -1;
}


/**
 * Send a start of frame
 */
void BlocDVBRcsNcc::sendSOF()
{
	unsigned char *lp_ptr;
	long l_size;
	T_DVB_HDR *lp_hdr;
	T_DVB_SOF *lp_sof;


	// Get a dvb frame
	lp_ptr = (unsigned char *) g_memory_pool_dvb_rcs.get(HERE());
	if(!lp_ptr)
	{
		UTI_ERROR("[sendSOF] Failed to get memory from pool dvb_rcs\n");
		return;
	}

	// Set DVB header
	l_size = sizeof(T_DVB_SOF);
	lp_hdr = (T_DVB_HDR *) lp_ptr;
	lp_hdr->msg_length = l_size;
	lp_hdr->msg_type = MSG_TYPE_SOF;

	// Set frame number
	lp_sof = (T_DVB_SOF *) lp_ptr;
	lp_sof->frame_nr = this->super_frame_counter;

	// Send it
	if(!this->sendDvbFrame((T_DVB_HDR *) lp_ptr, m_carrierIdSOF))
	{
		UTI_ERROR("[sendSOF] Failed to call sendDvbFrame()\n");
		g_memory_pool_dvb_rcs.release((char *) lp_ptr);
		return;
	}

	UTI_DEBUG_L3("SF%ld: SOF sent\n", this->super_frame_counter);
}


void BlocDVBRcsNcc::sendTBTP()
{
	unsigned char *lp_ptr;
	long carrier_id;
	long l_size;
	int ret;

	// Get a dvb frame
	lp_ptr = (unsigned char *) g_memory_pool_dvb_rcs.get(HERE());
	if(!lp_ptr)
	{
		UTI_ERROR("[sendTBTP] Failed to get memory from pool dvb_rcs\n");
		return;
	}

	l_size = MSG_DVB_RCS_SIZE_MAX;	// maximum size is the buffer size
	// Set DVB body
	ret = m_pDamaCtrl->buildTBTP(lp_ptr, l_size);
	if(ret < 0)
	{
		UTI_DEBUG_L3("[sendTBTP] Dama didn't build TBTP,"
		             "releasing buffer.\n");
		g_memory_pool_dvb_rcs.release((char *) lp_ptr);
		return;
	};

	// Send it
	carrier_id = m_pDamaCtrl->getCarrierId();
	l_size = ((T_DVB_TBTP *) lp_ptr)->hdr.msg_length;    // real size now
	if(!this->sendDvbFrame((T_DVB_HDR *) lp_ptr, carrier_id))
	{
		UTI_ERROR("[sendTBTP] Failed to send TBTP\n");
		g_memory_pool_dvb_rcs.release((char *) lp_ptr);
		return;
	}

	UTI_DEBUG_L3("SF%ld: TBTP sent\n", this->super_frame_counter);
}


void BlocDVBRcsNcc::onRcvLogonReq(unsigned char *ip_buf, int l_len)
{
	T_DVB_LOGON_REQ *lp_logon_req;
	T_DVB_LOGON_RESP *lp_logon_resp;
	int l_size;
	unsigned int dra_id;
	std::list<long>::iterator list_it;

	lp_logon_req = (T_DVB_LOGON_REQ *) ip_buf;
	UTI_DEBUG("[onRcvLogonReq] Logon request from %d\n", lp_logon_req->mac);

	// get context for this mac address
	// (could receive multiple logon request for same mac due to delay)

	// Sanity check of the buffer
	if(lp_logon_req->hdr.msg_type != MSG_TYPE_SESSION_LOGON_REQ)
	{
		UTI_ERROR("wrong packet data type (%ld)\n",
		          lp_logon_req->hdr.msg_type);
		goto release;
	}

	// Sanity check of the length of the buffer
	if(lp_logon_req->hdr.msg_length > l_len)
	{
		UTI_ERROR("buffer len (%d) < msg_length (%ld)\n",
		          l_len, lp_logon_req->hdr.msg_length);
		goto release;
	}

	// refuse to register a ST with same MAC ID as the NCC
	if(lp_logon_req->mac == this->macId)
	{
		UTI_ERROR("a ST wants to register with the MAC ID of the NCC "
		          "(%d), reject its request!\n", lp_logon_req->mac);
		goto release;
	}

	// send the corresponding event
	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, lp_logon_req->mac, 0,
	                    C_EVENT_LOGIN_RECEIVED);

	// register the new ST
	if(this->emissionStd->doSatelliteTerminalExist(lp_logon_req->mac))
	{
		// ST already registered once
		UTI_ERROR("request to register ST with ID %d that is already "
		          "registered, resend logon response\n",
		          lp_logon_req->mac);
	}
	else
	{
		// ST was not registered yet
		UTI_INFO("register ST with MAC ID %d\n", lp_logon_req->mac);
		if(!this->emissionStd->addSatelliteTerminal(lp_logon_req->mac,
		                                            lp_logon_req->nb_row))
		{
			UTI_ERROR("failed to register ST with MAC ID %d\n",
			          lp_logon_req->mac);
		}
	}

	// Get a dvb frame
	lp_logon_resp = (T_DVB_LOGON_RESP *) g_memory_pool_dvb_rcs.get(HERE());
	if(!lp_logon_resp)
	{
		UTI_ERROR("[onRcvLogonReq] Failed to get memory"
		          "from pool dvb_rcs\n");
		goto release;
	}

	// Inform the Dama controler (for its own context)
	dra_id = this->emissionStd->getStCurrentDraSchemeId(lp_logon_req->mac);
	if(m_pDamaCtrl->hereIsLogonReq((unsigned char*)lp_logon_req, l_len, dra_id) == 0)
	{
		// Set DVB header
		l_size = sizeof(T_DVB_LOGON_RESP);
		lp_logon_resp->hdr.msg_length = l_size;
		lp_logon_resp->hdr.msg_type = MSG_TYPE_SESSION_LOGON_RESP;
		lp_logon_resp->mac = lp_logon_req->mac;
		lp_logon_resp->nb_row = lp_logon_req->nb_row;
		lp_logon_resp->group_id = 0;
		lp_logon_resp->logon_id = lp_logon_req->mac;
		lp_logon_resp->return_vci = 0;
		lp_logon_resp->return_vpi = 0;
		lp_logon_resp->traffic_burst_type = 0;

		// Send it
		if(!sendDvbFrame((T_DVB_HDR *) lp_logon_resp, m_carrierIdDvbCtrl))
		{
			UTI_ERROR("[onRcvLogonReq] Failed send message\n");
			goto release;
		}


		UTI_DEBUG_L3("SF%ld: logon response sent to lower layer\n",
		             this->super_frame_counter);


		// send the corresponding event
		ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, lp_logon_req->mac, 0,
		                    C_EVENT_LOGIN_RESPONSE);

	}

release:
	g_memory_pool_dvb_rcs.release((char *) ip_buf);
}

void BlocDVBRcsNcc::onRcvLogoffReq(unsigned char *ip_buf, int l_len)
{
	T_DVB_LOGOFF *lp_logoff;
	std::list<long>::iterator list_it;

	lp_logoff = (T_DVB_LOGOFF *) ip_buf;

	// Packet type sanity check
	if(lp_logoff->hdr.msg_type != MSG_TYPE_SESSION_LOGOFF)
	{
		UTI_ERROR("wrong dvb packet type (%ld)\n",
		          lp_logoff->hdr.msg_type);
		goto release;
	}

	// Length sanity check
	if(lp_logoff->hdr.msg_length > l_len)
	{
		UTI_ERROR("pkt length (%ld) > buffer len (%d)\n",
		          lp_logoff->hdr.msg_length, l_len);
		goto release;
	}

	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->emissionStd->deleteSatelliteTerminal(lp_logoff->mac))
	{
		UTI_ERROR("failed to delete the ST with ID %d\n",
		          lp_logoff->mac);
		goto release;
	}

	m_pDamaCtrl->hereIsLogoff(ip_buf, l_len);
	UTI_DEBUG_L3("SF%ld: logoff request from %d\n",
	             this->super_frame_counter, lp_logoff->mac);

release:
	g_memory_pool_dvb_rcs.release((char *) ip_buf);
}

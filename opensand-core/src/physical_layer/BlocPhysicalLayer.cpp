/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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
 * @file     BlocPhysicalLayer.cpp
 * @brief    PhysicalLayer bloc
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 */

#include "BlocPhysicalLayer.h"

#include "msg_dvb_rcs.h"
#include "Channel.h"


#include <opensand_conf/conf.h>
#include <opensand_env_plane/EnvironmentAgent_e.h>

#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>


extern T_ENV_AGENT EnvAgent;


BlocPhysicalLayer::BlocPhysicalLayer(mgl_blocmgr *blocmgr,
                                     mgl_id fatherid,
                                     const char *name,
                                     component_t type,
                                     PluginUtils utils):
	mgl_bloc(blocmgr, fatherid, name),
	utils(utils)
{   
	const char *FUNCNAME = "[BlocPhysicalLayer][Constructor]";

	UTI_DEBUG("%s Basic DVB physical layer created\n", FUNCNAME);
	this->init_ok = false;
    
	// Timer id in bloc_physical layer for channel(s)
	// attenuation update
	this->channel_timer = -1;
    
	// Period of channel(s) attenuation update (ms)
	this->granularity = 0; 
    
	this->channel_uplink = NULL;
	this->channel_downlink = NULL;

	// Type of Terminal (st, gw or st) 
	this->component_type = type;
}


BlocPhysicalLayer::~BlocPhysicalLayer()
{
	UTI_DEBUG("DVB physical layer destructor\n");
	if(this->channel_uplink)
	{
		delete this->channel_uplink;
	}
	if(this->channel_downlink)
	{
		delete this->channel_downlink;
	}
}

bool BlocPhysicalLayer::initTimers()
{

	// first set the timer immediately in order to initialize parameters
	this->setTimer(this->channel_timer, 0);
	return true;

}

bool BlocPhysicalLayer::onInit()
{   
	// function name for debug
	const char *FUNCNAME = "[BlocPhysicalLayer][onInit]";

	// Initialization of physical layer objects for up and downlink
	ConfigurationList model_list;

	// Intermediate variables for Config file reading
	string attenuation_type; 
	string nominal_type;
	string minimal_type = "";
	string error_type = "";
	vector<string> links;

	// configure both up and down link
	links.push_back("up");
	links.push_back("down");

	// satellite type: regenerative or transparent ?
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          this->satellite_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_INFO("satellite type = %s\n", this->satellite_type.c_str());
	
	// satellite type: regenerative or transparent ?
	if(!globalConfig.getValue(PHYSICAL_LAYER_SECTION, GRANULARITY,
	                          this->granularity))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          PHYSICAL_LAYER_SECTION, GRANULARITY);
		goto error;
	}
	UTI_INFO("granularity = %d\n", this->granularity);

	// Retrieving model table entries
	if(!globalConfig.getListItems(PHYSICAL_LAYER_SECTION, MODEL_LIST,
	                              model_list))
	{
		UTI_ERROR("section '%s, %s': missing physical layer modules "
		          "configuration\n", PHYSICAL_LAYER_SECTION, MODEL_LIST);
		goto error;
	}

	for(vector<string>::iterator it = links.begin(); it != links.end(); ++it)
	{
		AttenuationModelPlugin *attenuation = NULL;
		NominalConditionPlugin *nominal = NULL;
		MinimalConditionPlugin *minimal = NULL;
		ErrorInsertionPlugin *error = NULL;

		// Initiate Attenuation model 
		if(!globalConfig.getValueInList(model_list,
		                                LINK,
		                                *it, ATTENUATION_MODEL_TYPE,
		                                attenuation_type))
		{
			UTI_ERROR("%s %slink %s cannot be parsed in %s, %s section\n",
			          FUNCNAME, (*it).c_str(), ATTENUATION_MODEL_TYPE,
			          PHYSICAL_LAYER_SECTION, MODEL_LIST);
			goto error;
		}
		// Initiate Nominal conditions 
		if(!globalConfig.getValueInList(model_list,
		                                LINK,
		                                *it, NOMINAL_CONDITION_TYPE,
		                                nominal_type))
		{
			UTI_ERROR("%s %slink %s cannot be parsed in %s, %s section\n",
			          FUNCNAME, (*it).c_str(), NOMINAL_CONDITION_TYPE,
			          PHYSICAL_LAYER_SECTION, MODEL_LIST);
			goto error;
		}
		
		if(*it == "down")
		{
			// Initiate Minimal conditions only for downlink
			if(!globalConfig.getValueInList(model_list,
			                                LINK,
			                                *it, MINIMAL_CONDITION_TYPE,
			                                minimal_type))
			{
				UTI_ERROR("%s %slink %s cannot be parsed in %s, %s section\n",
				          FUNCNAME, (*it).c_str(), MINIMAL_CONDITION_TYPE,
				          PHYSICAL_LAYER_SECTION, MODEL_LIST);
				goto error;
			}

			// Initiate Error Insertion only for downlink
			if(!globalConfig.getValueInList(model_list,
			                                LINK,
			                                *it, ERROR_INSERTION_TYPE,
			                                error_type))
			{
				UTI_ERROR("%s %slink %s cannot be parsed in %s, %s section\n",
				          FUNCNAME, (*it).c_str(), ERROR_INSERTION_TYPE,
				          PHYSICAL_LAYER_SECTION, MODEL_LIST);
				goto error;
			}
		}

		/* get all the plugins */
		if(!this->utils.getPhysicalLayerPlugins(attenuation_type,
		                                        nominal_type,
		                                        minimal_type,
		                                        error_type,
		                                        &attenuation,
		                                        &nominal,
		                                        &minimal,
		                                        &error))
		{
			UTI_ERROR("error when getting physical layer plugins");
			goto error;
		}
		if(!attenuation->init(this->granularity, *it))
		{
			UTI_ERROR("cannot initialize attenuation model plugin %s",
			           attenuation_type.c_str());
			goto error;
		}
		if(!nominal->init(*it))
		{
			UTI_ERROR("cannot initialize nominal condition plugin %s",
			           nominal_type.c_str());
			goto error;
		}

		if(*it == "down")
		{
			UTI_INFO("%slink: attenuation model = %s, nominal condition type = %s, "
			         "minimal condition type = %s, error insertion type = %s",
			         (*it).c_str(), attenuation_type.c_str(), nominal_type.c_str(),
			         minimal_type.c_str(), error_type.c_str());
		}
		else
		{
			UTI_INFO("%slink: attenuation model = %s, nominal condition type = %s",
			         (*it).c_str(), attenuation_type.c_str(), nominal_type.c_str());
		}

		// Initiate the Channels
		if(*it == "up")
		{
			this->channel_uplink = new Channel(*it,
			                                   attenuation,
			                                   nominal,
			                                   minimal,
			                                   error);
		}
		else
		{
			if(!minimal->init())
			{
				UTI_ERROR("cannot initialize minimal condition plugin %s",
				          minimal_type.c_str());
				goto error;
			}
			
			if(!error->init())
			{
				UTI_ERROR("cannot initialize error insertion plugin %s",
				          error_type.c_str());
				goto error;
			}

			this->channel_downlink = new Channel(*it,
			                                     attenuation,
			                                     nominal,
			                                     minimal,
			                                     error);
		}
	}

	// Read the frame duration, the super frame duration
	// and the second duration
	if(!this->initTimers())
	{
		UTI_ERROR("failed to complete the timers part of the "
		          "initialization");
		goto error;
	}

	return true;

error:
	return false;
}

mgl_status BlocPhysicalLayer::onEvent(mgl_event *event)
{
	const char *FUNCNAME = "[BlocPhysicalLayer] [onEvent]"; 
	mgl_status status = mgl_ok; // whether the event is successfully
	// handled or not

	if(MGL_EVENT_IS_INIT(event))
	{
		UTI_DEBUG("%s init event received\n", FUNCNAME);
		// Initialization event
		if(this->init_ok)
		{
			UTI_ERROR("%s bloc has already been initialized, "
			          "ignore init event\n", FUNCNAME);
		}
		else if(!this->onInit())
		{
			UTI_ERROR("%s bloc initialization failed\n", FUNCNAME);
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT_COMPO);
			status = mgl_ko;
		}
		else
		{
			this->init_ok = true;
			ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_INIT, 4, 0, 12);
		}
	}
	else if(!this->init_ok)
	{
		UTI_ERROR("%s bloc not initialized, ignore non-init events\n",
		          FUNCNAME);
		status = mgl_ko;
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		// message event: forward DVB frames from lower block to upper block
		// and from upper block to lower block
		T_DVB_META *dvb_meta; // Header structure for Meta data (carrier_id)
		long l_len;           // Length of the received packet

		// retrieve data (ie. the DVB frame) from the message
		dvb_meta = (T_DVB_META *) MGL_EVENT_MSG_GET_BODY(event);
		l_len = MGL_EVENT_MSG_GET_BODYLEN(event);

		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getUpperLayer() &&
		   MGL_EVENT_MSG_IS_TYPE(event, msg_dvb))
		{
			// DVB message received from the upper block

			// forward the DVB frame to the lower block
			status = this->forwardMetaFrame(this->getLowerLayer(),
			                                dvb_meta, l_len);
		}
		else if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer() &&
		        MGL_EVENT_MSG_IS_TYPE(event, msg_dvb))
		{
			// DVB message received from the lower block

			// forward the DVB frame to the upper block
			status = this->forwardMetaFrame(this->getUpperLayer(),
			                                dvb_meta, l_len);
		}
		else
		{
			UTI_ERROR("%s message received, but not handled\n", FUNCNAME);
			status = mgl_ko;
		}
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		//Event handler for Channel(s) state update
		if(MGL_EVENT_TIMER_IS_TIMER(event, this->channel_timer))
		{
			status = mgl_ok;
			UTI_DEBUG_L3("channel timer expired\n");
			if(!this->channel_uplink->updateChannel() or
			   !this->channel_downlink->updateChannel())
			{
				status = mgl_ko;
				UTI_ERROR("one of both channels updating failed, do not "
				          "update channels anymore\n");
			}
			else
			{
				setTimer(this->channel_timer, this->granularity);
			}
		}   
	}
	else
	{
		UTI_ERROR("%s event received, but not handled\n", FUNCNAME);
		status = mgl_ko;
	}

	return status;
}


mgl_status BlocPhysicalLayer::forwardMetaFrame(mgl_id dest_block,
                                               T_DVB_META *dvb_meta,
                                               long l_len)
{
	const char *FUNCNAME = "[BlocPhysicalLayer] [forward_dvb_frame]";
	mgl_msg *msg;
	long len_modif = l_len; // Length of the final resulting frame.
	                        // If physical layer is OFF: frame length
	                        // keeps the inital length

	//CASE 1: TERMINAL(ST or GW) 
	if(this->component_type != satellite)
	{
		if(dest_block == this->getLowerLayer())
		{   
			//Location of T_DVB_PHY at the end of the frame (Note:(char *)
			//         used to point/address individual bytes) 
			T_DVB_PHY *physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
			                                  + dvb_meta->hdr->msg_length);

			// Case of outgoing msg: Mark the msg with the C/N of Channel 
			this->channel_uplink->addSegmentCN(physical_parameters);

			// Length of the resulting msg including the PHY trailer 
			len_modif = l_len + sizeof(T_DVB_PHY);

			UTI_DEBUG_L3("SEND: Insert Uplink C/N = %f dB, CarrierId = %ld, " 
			             "PktLength = %ld, MsgLength = %ld \n",
			             physical_parameters->cn_previous,
			             dvb_meta->carrier_id,len_modif,
			             dvb_meta->hdr->msg_length);
		}
		else if(dest_block == this->getUpperLayer())
		{
			// Update of the Threshold CN if Minimal Condition
			// Mde is Modcod dependent 
			if(!this->channel_downlink->updateMinimalCondition(dvb_meta->hdr))
			{
				// debug because it will be very verbose
				UTI_DEBUG("Error in Update of Minimal Condition\n");
				goto error;
			}

			//Location of T_DVB_PHY at the end of the frame 
			T_DVB_PHY * physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
			                                         + dvb_meta->hdr->msg_length);

			// Length of the new msg including the PHY trailer 
			len_modif= l_len - sizeof(T_DVB_PHY);

			UTI_DEBUG_L3("RECEIVE: Previous C/N  = %f dB - CarrierId = %ld " 
			             "PktLength = %ld MsgLength = %ld \n",
			             physical_parameters->cn_previous,
			             dvb_meta->carrier_id,len_modif,
			             dvb_meta->hdr->msg_length);

			// Checking if the received frame must be affected by errors 
			if(this->channel_downlink->isToBeModifiedPacket(physical_parameters->cn_previous))
			{
				// Insertion of errors if necessary                
				this->channel_downlink->modifyPacket(dvb_meta,len_modif); 
			}
		}
	}
	//CASE 2: SATELLITE
	else
	{
		if(this->satellite_type == TRANSPARENT_SATELLITE &&
		   dest_block == this->getLowerLayer())
		{
			//Location of T_DVB_PHY at the end of the frame (Note:(char *)
			//         used to point/address individual bytes) 
			T_DVB_PHY * physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
			                                         + dvb_meta->hdr->msg_length);

			UTI_DEBUG_L3("Satellite Transparent: Insert Uplink C/N = %f dB, "
			             "CarrierId = %ld PktLength = %ld, MsgLength = %ld\n",
			             physical_parameters->cn_previous,
			             dvb_meta->carrier_id, len_modif,
			             dvb_meta->hdr->msg_length);

			// Modify the C/N value of the message due to satellite segment influence 
			this->channel_uplink->modifySegmentCN(physical_parameters);
		}
		// TODO try to factorize with st and gw
		else if(this->satellite_type == REGENERATIVE_SATELLITE)
		{
			//TODO: Dropping and Error Insertion
			//Redundant code: same as Transparent case for STs and GW

			if(dest_block == this->getLowerLayer())
			{   
				//Location of T_DVB_PHY at the end of the frame (Note: (char *)
				//         used to point/address individual bytes) 
				T_DVB_PHY * physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
				                                         + dvb_meta->hdr->msg_length);

				// Case of outgoing msg: Mark the msg with the C/N of Channel 
				this->channel_uplink->addSegmentCN(physical_parameters);

				// Length of the resulting msg including the PHY trailer 
				len_modif = l_len + sizeof(T_DVB_PHY);

				UTI_DEBUG_L3("SEND: Insert [Satellite - ground terminal] "
				             "C/N  = %f dB, CarrierId = %ld " 
				             "PktLength = %ld MsgLength = %ld \n",
				             physical_parameters->cn_previous,
				             dvb_meta->carrier_id,len_modif,
				             dvb_meta->hdr->msg_length);
			}
			else if(dest_block == this->getUpperLayer())
			{
				//Location of T_DVB_PHY at the end of the frame 
				T_DVB_PHY * physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
				                                        + dvb_meta->hdr->msg_length);

				// Length of the new msg including the PHY trailer 
				len_modif= l_len - sizeof(T_DVB_PHY);

				UTI_DEBUG_L3("RECEIVE: Previous C/N  = %f dB - CarrierId = %ld " 
				             "PktLength = %ld MsgLength = %ld \n",
				             physical_parameters->cn_previous,
				             dvb_meta->carrier_id,len_modif,
				             dvb_meta->hdr->msg_length);

				// Checking if the received frame must be affected by errors 
				if(this->channel_downlink->isToBeModifiedPacket(physical_parameters->cn_previous))
				{
					// Insertion of errors if necessary                
					this->channel_downlink->modifyPacket(dvb_meta,len_modif); 
				}
			}
		}
	} 
    
	// create a new message for the lower layer
	msg = this->newMsgWithBodyPtr(msg_dvb, dvb_meta, len_modif);
	if(!msg)
	{
		// cannot create the message, so release memory used by the DVB frame
		UTI_ERROR("%s cannot create a DVB message\n", FUNCNAME);
		g_memory_pool_dvb_rcs.release((char *) dvb_meta);
		goto error;
	}
	// message successfully created, send the message to lower block
	this->sendMsgTo(dest_block, msg);

	return mgl_ok;
error:
	return mgl_ko;
}



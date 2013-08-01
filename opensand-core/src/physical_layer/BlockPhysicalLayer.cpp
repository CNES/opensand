/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file     BlockPhysicalLayer.cpp
 * @brief    PhysicalLayer bloc
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "BlockPhysicalLayer.h"

#include "Plugin.h"
#include "msg_dvb_rcs.h"
#include "PhyChannel.h"


#include <opensand_conf/conf.h>


// output events
Event *BlockPhysicalLayer::error_init = NULL;
Event *BlockPhysicalLayer::init_done = NULL;

BlockPhysicalLayer::BlockPhysicalLayer(const string &name,
                                       component_t component_type):
	Block(name),
	component_type(component_type)
{
	UTI_DEBUG("Basic DVB physical layer created\n");
	// TODO we need a mutex here because some parameters may be used in upward and downward
	this->enableChannelMutex();

	error_init = Output::registerEvent("BlockPhysicalLayer::init", LEVEL_ERROR);
	init_done = Output::registerEvent("BlockPhysicalLayer::init_done", LEVEL_INFO);
}


BlockPhysicalLayer::~BlockPhysicalLayer()
{
	UTI_DEBUG("DVB physical layer destructor\n");
}


bool BlockPhysicalLayer::onInit(void)
{
	((Chan *)this->downward)->component_type = this->component_type;
	((Chan *)this->upward)->component_type = this->component_type;

	return true;
}

bool Chan::initChan(const string &link)
{
	// Initialization of physical layer objects for up and downlink
	ConfigurationList model_list;

	ostringstream name;

	// Intermediate variables for Config file reading
	string sat_type;
	string attenuation_type;
	string nominal_type;
	string minimal_type = "";
	string error_type = "";

	AttenuationModelPlugin *attenuation = NULL;
	NominalConditionPlugin *nominal = NULL;
	MinimalConditionPlugin *minimal = NULL;
	ErrorInsertionPlugin *error = NULL;

	// satellite type: regenerative or transparent ?
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE, sat_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_INFO("satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	// get granularity
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

	// Initiate Attenuation model
	if(!globalConfig.getValueInList(model_list,
	                                LINK,
	                                link, ATTENUATION_MODEL_TYPE,
	                                attenuation_type))
	{
		UTI_ERROR("%slink %s cannot be parsed in %s, %s section\n",
		          link.c_str(), ATTENUATION_MODEL_TYPE,
		          PHYSICAL_LAYER_SECTION, MODEL_LIST);
		goto error;
	}
	// Initiate Nominal conditions
	if(!globalConfig.getValueInList(model_list,
	                                LINK,
	                                link, NOMINAL_CONDITION_TYPE,
	                                nominal_type))
	{
		UTI_ERROR("%slink %s cannot be parsed in %s, %s section\n",
		          link.c_str(), NOMINAL_CONDITION_TYPE,
		          PHYSICAL_LAYER_SECTION, MODEL_LIST);
		goto error;
	}

	if(link == "down")
	{
		// Initiate Minimal conditions only for downlink
		if(!globalConfig.getValueInList(model_list,
		                                LINK,
		                                link, MINIMAL_CONDITION_TYPE,
		                                minimal_type))
		{
			UTI_ERROR("%slink %s cannot be parsed in %s, %s section\n",
			          link.c_str(), MINIMAL_CONDITION_TYPE,
			          PHYSICAL_LAYER_SECTION, MODEL_LIST);
			goto error;
		}

		// Initiate Error Insertion only for downlink
		if(!globalConfig.getValueInList(model_list,
		                                LINK,
		                                link, ERROR_INSERTION_TYPE,
		                                error_type))
		{
			UTI_ERROR("%slink %s cannot be parsed in %s, %s section\n",
			          link.c_str(), ERROR_INSERTION_TYPE,
			          PHYSICAL_LAYER_SECTION, MODEL_LIST);
			goto error;
		}
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins(attenuation_type,
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
	if(!attenuation->init(this->granularity, link))
	{
		UTI_ERROR("cannot initialize attenuation model plugin %s",
		           attenuation_type.c_str());
		goto error;
	}
	if(!nominal->init(link))
	{
		UTI_ERROR("cannot initialize nominal condition plugin %s",
		           nominal_type.c_str());
		goto error;
	}

	if(link == "down")
	{
		UTI_INFO("%slink: attenuation model = %s, nominal condition type = %s, "
		         "minimal condition type = %s, error insertion type = %s",
		         link.c_str(), attenuation_type.c_str(), nominal_type.c_str(),
		         minimal_type.c_str(), error_type.c_str());

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
	}
	else
	{
		UTI_INFO("%slink: attenuation model = %s, nominal condition type = %s",
		         link.c_str(), attenuation_type.c_str(), nominal_type.c_str());
	}

	name << "attenuation_" << link;
	this->att_timer = this->addTimerEvent(name.str(), this->granularity);

	return true;

error:
	return false;
}

bool BlockPhysicalLayer::onEvent(const RtEvent *const event,
                                 Chan *chan)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// message event: forward DVB frames from upper block to lower block
			T_DVB_META *dvb_meta; // Header structure for Meta data (carrier_id)
			long l_len;           // Length of the received packet

			// retrieve data (ie. the DVB frame) from the message
			dvb_meta = (T_DVB_META *)((MessageEvent *)event)->getData();
			l_len = ((MessageEvent *)event)->getLength();

			// forward the DVB frame to the lower block
			return chan->forwardMetaFrame(dvb_meta, l_len);
		}
		break;

		case evt_timer:
			if(*event != chan->att_timer)
			{
				return false;
			}
			//Event handler for Channel(s) state update
			UTI_DEBUG_L3("channel timer expired\n");
			if(!chan->update())
			{
				UTI_ERROR("one of both channels updating failed, do not "
				          "update channels anymore\n");
				return false;
			}
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockPhysicalLayer::onDownwardEvent(const RtEvent *const event)
{
	return this->onEvent(event, (Chan *)this->downward);
}

bool BlockPhysicalLayer::onUpwardEvent(const RtEvent *const event)
{
	return this->onEvent(event, (Chan *)this->upward);
}


// TODO check block initialization with up and down
bool BlockPhysicalLayer::PhyUpward::onInit(void)
{
	// link = 'down' because upward channel corresponds to downlink
	return this->initChan("down");
}

bool BlockPhysicalLayer::PhyDownward::onInit(void)
{
	// link = 'down' because upward channel corresponds to downlink
	return this->initChan("up");
}


bool BlockPhysicalLayer::PhyUpward::forwardMetaFrame(T_DVB_META *dvb_meta,
                                                     long l_len)
{
	long len_modif = l_len; // Length of the final resulting frame.
	                        // If physical layer is OFF: frame length
	                        // keeps the inital length
	T_DVB_PHY *physical_parameters;

	if(this->satellite_type != REGENERATIVE)
	{
		return true;
	}
	//TODO: Dropping and Error Insertion for sat

	if(this->component_type != satellite)
	{
		// Update of the Threshold CN if Minimal Condition
		// Mde is Modcod dependent
		if(!this->updateMinimalCondition(dvb_meta->hdr))
		{
			// debug because it will be very verbose
			UTI_DEBUG("Error in Update of Minimal Condition\n");
			goto error;
		}
	}

	//Location of T_DVB_PHY at the end of the frame
	physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
	                                         + dvb_meta->hdr->msg_length);

	// Length of the new msg including the PHY trailer
	len_modif= l_len - sizeof(T_DVB_PHY);

	UTI_DEBUG_L3("RECEIVE: Previous C/N  = %f dB - CarrierId = %u "
	             "PktLength = %ld MsgLength = %u\n",
	             physical_parameters->cn_previous,
	             dvb_meta->carrier_id, len_modif,
	             dvb_meta->hdr->msg_length);

	// Checking if the received frame must be affected by errors
	if(this->isToBeModifiedPacket(physical_parameters->cn_previous))
	{
		// Insertion of errors if necessary
		this->modifyPacket(dvb_meta,len_modif);
	}

	// message successfully created, send the message to upper block
	if(!this->enqueueMessage((void **)&dvb_meta, len_modif))
	{
		UTI_ERROR("failed to send burst of packets to upper layer\n");
		goto error;
	}

	return true;
error:
	return false;
}



bool BlockPhysicalLayer::PhyDownward::forwardMetaFrame(T_DVB_META *dvb_meta,
                                                       long l_len)
{
	long len_modif = l_len; // Length of the final resulting frame.
	                        // If physical layer is OFF: frame length
	                        // keeps the inital length

	//CASE 1: TERMINAL(ST or GW)
	if(this->component_type != satellite or this->satellite_type == REGENERATIVE)
	{
		//Location of T_DVB_PHY at the end of the frame (Note:(char *)
		//         used to point/address individual bytes)
		T_DVB_PHY *physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
		                                  + dvb_meta->hdr->msg_length);

		// Case of outgoing msg: Mark the msg with the C/N of Channel
		this->addSegmentCN(physical_parameters);

		// Length of the resulting msg including the PHY trailer
		len_modif = l_len + sizeof(T_DVB_PHY);

		UTI_DEBUG_L3("SEND: Insert Uplink C/N = %f dB, CarrierId = %u, "
		             "PktLength = %ld, MsgLength = %u\n",
		             physical_parameters->cn_previous,
		             dvb_meta->carrier_id,len_modif,
		             dvb_meta->hdr->msg_length);
	}
	//CASE 2: SATELLITE
	else if(this->satellite_type == TRANSPARENT)
	{
		//Location of T_DVB_PHY at the end of the frame (Note:(char *)
		//         used to point/address individual bytes)
		T_DVB_PHY * physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr
		                                         + dvb_meta->hdr->msg_length);

		UTI_DEBUG_L3("Satellite Transparent: Insert Uplink C/N = %f dB, "
		             "CarrierId = %u PktLength = %ld, MsgLength = %u\n",
		             physical_parameters->cn_previous,
		             dvb_meta->carrier_id, len_modif,
		             dvb_meta->hdr->msg_length);

		// Modify the C/N value of the message due to satellite segment influence
		this->modifySegmentCN(physical_parameters);
	}

	// message successfully created, send the message to lower block
	if(!this->enqueueMessage((void **)&dvb_meta, len_modif))
	{
		UTI_ERROR("failed to send burst of packets to lower layer\n");
		goto error;
	}

	return true;
error:
	return false;
}



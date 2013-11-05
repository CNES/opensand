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
 * @file     BlockPhysicalLayerSat.cpp
 * @brief    PhysicalLayer bloc
 * @author   Santiago PENA <santiago.penaluque@cnes.fr>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PREFIX
#define DBG_PACKAGE PKG_PHY_LAYER
#include <opensand_conf/uti_debug.h>

#include "BlockPhysicalLayerSat.h"

#include "Plugin.h"
#include "OpenSandFrames.h"
#include "PhyChannel.h"


#include <opensand_conf/conf.h>


// output events
Event *BlockPhysicalLayerSat::error_init = NULL;
Event *BlockPhysicalLayerSat::init_done = NULL;

BlockPhysicalLayerSat::BlockPhysicalLayerSat(const string &name):
	Block(name)
{
	error_init = Output::registerEvent("BlockPhysicalLayerSat::init", LEVEL_ERROR);
	init_done = Output::registerEvent("BlockPhysicalLayerSat::init_done", LEVEL_INFO);
}


BlockPhysicalLayerSat::~BlockPhysicalLayerSat()
{
}


bool BlockPhysicalLayerSat::onInit(void)
{
	return true;
}

bool BlockPhysicalLayerSat::onEvent(const RtEvent *const event,
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

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockPhysicalLayerSat::onDownwardEvent(const RtEvent *const event)
{
	return this->onEvent(event, (Chan *)this->downward);
}

bool BlockPhysicalLayerSat::onUpwardEvent(const RtEvent *const event)
{
	return this->onEvent(event, (Chan *)this->upward);
}

bool BlockPhysicalLayerSat::PhyUpward::onInit(void)
{
	ostringstream name;
	string link("down"); // we are on downlink

	// Intermediate variables for Config file reading
	string sat_type;
	string minimal_type;
	string error_type;

	// satellite type
	if(!globalConfig.getValue(GLOBAL_SECTION, SATELLITE_TYPE,
	                          sat_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, SATELLITE_TYPE);
		goto error;
	}
	UTI_INFO("satellite type = %s\n", sat_type.c_str());
	this->satellite_type = strToSatType(sat_type);

	// Initiate Minimal conditions
	if(!globalConfig.getValue(PHYSICAL_LAYER_SECTION,
	                          MINIMAL_CONDITION_TYPE,
	                          minimal_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          PHYSICAL_LAYER_SECTION, MINIMAL_CONDITION_TYPE);
		goto error;
	}

	// Initiate Error Insertion
	if(!globalConfig.getValue(PHYSICAL_LAYER_SECTION,
	                          ERROR_INSERTION_TYPE,
	                          error_type))
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          PHYSICAL_LAYER_SECTION, ERROR_INSERTION_TYPE);
		goto error;
	}

	/* get all the plugins */
	if(!Plugin::getPhysicalLayerPlugins("",
	                                    minimal_type,
	                                    error_type,
	                                    &this->attenuation_model,
	                                    &this->minimal_condition,
	                                    &this->error_insertion))
	{
		UTI_ERROR("error when getting physical layer plugins");
		goto error;
	}
	
	UTI_INFO("uplink: minimal condition type = %s, error insertion type = %s",
	         minimal_type.c_str(), error_type.c_str());

	if(!this->minimal_condition->init())
	{
		UTI_ERROR("cannot initialize minimal condition plugin %s",
		          minimal_type.c_str());
		goto error;
	}

	if(!this->error_insertion->init())
	{
		UTI_ERROR("cannot initialize error insertion plugin %s",
		          error_type.c_str());
		goto error;
	}

	return true;

error:
	return false;
}


bool BlockPhysicalLayerSat::PhyUpward::forwardMetaFrame(T_DVB_META *dvb_meta,
                                                        size_t len)
{
	size_t len_modif = len; // Length of the final resulting frame.
	double cn_total;
	T_DVB_PHY *physical_parameters;

	if(this->satellite_type == TRANSPARENT ||
	   (((T_DVB_HDR *)dvb_meta->hdr)->msg_type != MSG_TYPE_BBFRAME &&
	    ((T_DVB_HDR *)dvb_meta->hdr)->msg_type != MSG_TYPE_DVB_BURST))
	{
		// nothing to do in transparent mode
		// do not handle signalisation
		goto forward;
	}

	// Update of the Threshold CN if Minimal Condition
	// Mde is Modcod dependent
	if(!this->updateMinimalCondition(dvb_meta->hdr))
	{
		// debug because it will be very verbose
		UTI_DEBUG("Error in Update of Minimal Condition\n");
		goto error;
	}

	// Location of T_DVB_PHY at the end of the frame
	physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr +
	                                    dvb_meta->hdr->msg_length);

	// Length of the new msg without the PHY trailer
	len_modif = len - sizeof(T_DVB_PHY);

	UTI_DEBUG_L3("RECEIVE: Previous C/N  = %f dB - CarrierId = %u "
	             "PktLength = %ld MsgLength = %u\n",
	             physical_parameters->cn_previous,
	             dvb_meta->carrier_id, len_modif,
	             dvb_meta->hdr->msg_length);

	cn_total = physical_parameters->cn_previous;
	UTI_DEBUG("Total C/N: %f\n", cn_total);
	// Checking if the received frame must be affected by errors
	if(this->isToBeModifiedPacket(cn_total))
	{
		// Insertion of errors if necessary
		this->modifyPacket(dvb_meta, len_modif);
	}

forward:
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


bool BlockPhysicalLayerSat::PhyDownward::forwardMetaFrame(T_DVB_META *dvb_meta,
                                                          size_t len)
{
	size_t len_modif = len; // Length of the final resulting frame.
	T_DVB_PHY *physical_parameters;

	if(this->satellite_type == TRANSPARENT ||
	   (((T_DVB_HDR *)dvb_meta->hdr)->msg_type != MSG_TYPE_BBFRAME &&
	    ((T_DVB_HDR *)dvb_meta->hdr)->msg_type != MSG_TYPE_DVB_BURST))
	{
		// nothing to do in transparent mode
		// do not handle signalisation
		goto forward;
	}

	//Location of T_DVB_PHY at the end of the frame (Note:(char *)
	//         used to point/address individual bytes)
	physical_parameters = (T_DVB_PHY *)((char *)dvb_meta->hdr +
	                                    dvb_meta->hdr->msg_length);
	// we only need physical parameters for factorization on the receving side
	// that is the same for transparent mode
	// we use -1 for unused C/N
	physical_parameters->cn_previous = -1;

	// Length of the resulting msg including the PHY trailer
	len_modif = len + sizeof(T_DVB_PHY);

	UTI_DEBUG_L3("SEND: Insert Fake Uplink C/N = %fdB, CarrierId = %u, "
	             "PktLength = %ld, MsgLength = %u\n",
	             physical_parameters->cn_previous,
	             dvb_meta->carrier_id,len_modif,
	             dvb_meta->hdr->msg_length);

forward:
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



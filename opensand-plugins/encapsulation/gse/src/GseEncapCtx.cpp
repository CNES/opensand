/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file GseEncapCtx.cpp
 * @brief GSE encapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "GseEncapCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "opensand_conf/uti_debug.h"


GseEncapCtx::GseEncapCtx(GseIdentifier *identifier, uint16_t spot_id)
{
	this->src_tal_id = identifier->getSrcTalId();
	this->dst_tal_id = identifier->getDstTalId();
	this->qos = identifier->getQos();
	this->is_full = false;
	this->vfrag = NULL;
	this->protocol = 0;
	this->name = "unknown";
	this->dest_spot = spot_id;
}

GseEncapCtx::~GseEncapCtx()
{
	if(this->vfrag != NULL)
	{
		gse_free_vfrag(&(this->vfrag));
	}
}

gse_status_t GseEncapCtx::add(NetPacket *packet)
{
	gse_status_t status = GSE_STATUS_OK;

	size_t previous_length = 0;

	// Check is context already contains data
	if(this->vfrag == NULL)
	{
		status = gse_create_vfrag(&this->vfrag,
		                          GSE_MAX_PACKET_LENGTH,
		                          GSE_MAX_HEADER_LENGTH,
		                          GSE_MAX_TRAILER_LENGTH);
		if(status != GSE_STATUS_OK)
		{
			goto error;
		}
		this->protocol = packet->getType();
		this->name = packet->getName();
	}
	else if(this->isFull())
	{
		status = GSE_STATUS_DATA_TOO_LONG;
		UTI_ERROR("failed to encapsulate packet because its size "
		          "is greater that GSE fragment free space\n");
		goto error;
	}
	else
	{
		previous_length = gse_get_vfrag_length(this->vfrag);
	}

	memcpy(gse_get_vfrag_start(this->vfrag) + previous_length,
	       (unsigned char *)packet->getData().c_str(),
	       packet->getTotalLength());
	// Update the virtual fragment length
	status = gse_set_vfrag_length(this->vfrag, previous_length +
	                              packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("failed to set the new vfrag length\n");
		goto error;
	}

	// if there is not enough space in buffer for another packet
	// set is_full to true
	if((GSE_MAX_PACKET_LENGTH -
	   gse_get_vfrag_length(this->vfrag)) < packet->getTotalLength())
	{
		this->is_full = true;
	}

error:
	return status;
}

gse_vfrag_t *GseEncapCtx::data()
{
	return this->vfrag;
}

size_t GseEncapCtx::length()
{
	if(this->vfrag != NULL)
	{
		return gse_get_vfrag_length(this->vfrag);
	}
	else
	{
		return 0;
	}
}

bool GseEncapCtx::isFull()
{
	return this->is_full;
}

uint8_t GseEncapCtx::getSrcTalId()
{
	return this->src_tal_id;
}

uint8_t GseEncapCtx::getDstTalId()
{
	return this->dst_tal_id;
}

uint8_t GseEncapCtx::getQos()
{
	return this->qos;
}

uint16_t GseEncapCtx::getProtocol()
{
	return this->protocol;
}

std::string GseEncapCtx::getPacketName()
{
	return this->name;
}

uint16_t GseEncapCtx::getDestSpot()
{
	return this->dest_spot;
}


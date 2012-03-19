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
 * @file GseEncapCtx.cpp
 * @brief GSE encapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include <GseEncapCtx.h>

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


GseEncapCtx::GseEncapCtx(GseIdentifier *identifier)
{
	this->_tal_id = identifier->talId();
	this->_mac_id = identifier->macId();
	this->_qos = identifier->qos();
	this->_is_full = false;
	this->_vfrag = NULL;
	this->_protocol = 0;
	this->_name = "unknown";
}

GseEncapCtx::~GseEncapCtx()
{
	if(this->_vfrag != NULL)
	{
		gse_free_vfrag(&(this->_vfrag));
	}
}

gse_status_t GseEncapCtx::add(NetPacket *packet)
{
	gse_status_t status = GSE_STATUS_OK;

	size_t previous_length = 0;

	// Check is context already contains data
	if(this->_vfrag == NULL)
	{
		status = gse_create_vfrag(&this->_vfrag,
		                          GSE_MAX_PACKET_LENGTH,
		                          GSE_MAX_HEADER_LENGTH,
		                          GSE_MAX_TRAILER_LENGTH);
		if(status != GSE_STATUS_OK)
		{
			goto error;
		}
		this->_protocol = packet->type();
		this->_name = packet->name();
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
		previous_length = gse_get_vfrag_length(this->_vfrag);
	}

	memcpy(gse_get_vfrag_start(this->_vfrag) + previous_length,
	       (unsigned char *)packet->data().c_str(),
	       packet->totalLength());
	// Update the virtual fragment length
	status = gse_set_vfrag_length(this->_vfrag, previous_length +
	                              packet->totalLength());
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("failed to set the new vfrag length\n");
		goto error;
	}

	// if there is not enough space in buffer for another packet
	// set is_full to true
	if((GSE_MAX_PACKET_LENGTH -
	   gse_get_vfrag_length(this->_vfrag)) < packet->totalLength())
	{
		this->_is_full = true;
	}

error:
	return status;
}

gse_vfrag_t *GseEncapCtx::data()
{
	return this->_vfrag;
}

size_t GseEncapCtx::length()
{
	if(this->_vfrag != NULL)
	{
		return gse_get_vfrag_length(this->_vfrag);
	}
	else
	{
		return 0;
	}
}

bool GseEncapCtx::isFull()
{
	return this->_is_full;
}

long GseEncapCtx::talId()
{
	return this->_tal_id;
}

unsigned long GseEncapCtx::macId()
{
	return this->_mac_id;
}

int GseEncapCtx::qos()
{
	return this->_qos;
}

uint16_t GseEncapCtx::protocol()
{
	return this->_protocol;
}

std::string GseEncapCtx::packetName()
{
	return this->_name;
}

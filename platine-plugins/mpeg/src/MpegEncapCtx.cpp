/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
 * @file MpegEncapCtx.cpp
 * @brief MPEG encapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <MpegEncapCtx.h>


MpegEncapCtx::MpegEncapCtx(uint16_t pid, uint16_t spot_id)
{
	this->_frame = new Data();
	this->_pid = pid;
	this->_cc = 0;
	this->_dst_spot = spot_id;

	this->initFrame();
}

MpegEncapCtx::~MpegEncapCtx()
{
	if(this->_frame != NULL)
		delete this->_frame;
}

void MpegEncapCtx::initFrame()
{
	this->_frame->clear();
	this->_frame->append(1, 0x47);
	this->_frame->append(1, (this->_pid >> 8) & 0x1f);
	this->_frame->append(1, (this->_pid >> 0) & 0xff);
	this->_frame->append(1, 0x10 | (this->_cc & 0x0F));
}

void MpegEncapCtx::reset()
{
	this->_cc = (this->_cc + 1) & 0x0f;
	this->initFrame();
}

Data * MpegEncapCtx::frame()
{
	return this->_frame;
}

void MpegEncapCtx::add(Data *data, unsigned int offset,
                       unsigned int length)
{
	this->_frame->append(*data, offset, length);
}

unsigned int MpegEncapCtx::length()
{
	return this->_frame->length();
}

unsigned int MpegEncapCtx::left()
{
	return TS_PACKETSIZE - this->length();
}

uint8_t MpegEncapCtx::sync()
{
	return this->_frame->at(0);
}

uint16_t MpegEncapCtx::pid()
{
	return this->_pid;
}

uint8_t MpegEncapCtx::cc()
{
	return this->_cc;
}

bool MpegEncapCtx::pusi()
{
	return (this->_frame->at(1) & 0x40) != 0;
}

void MpegEncapCtx::setPusi()
{
	this->_frame->replace(1, 1, 1, this->_frame->at(1) | 0x40);
}

void MpegEncapCtx::addPP()
{
	if(this->length() > TS_HEADERSIZE)
	{
		// partially filled MPEG2-TS packet
		this->_frame->insert(TS_HEADERSIZE, 1, this->length() - TS_HEADERSIZE);
	}
	else
	{
		// empty MPEG2-TS packet
		this->_frame->append(1, 0x00);
	}
}

void MpegEncapCtx::padding()
{
	if(this->left() > 0)
		this->_frame->append(this->left(), 0xff);
}

uint16_t MpegEncapCtx::getDstSpot()
{
	return this->_dst_spot;
}


/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file MpegDeencapCtx.cpp
 * @brief MPEG2-TS desencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <MpegDeencapCtx.h>

#include <opensand_output/Output.h>


MpegDeencapCtx::MpegDeencapCtx(uint16_t pid, uint16_t spot_id): _data()
{
	this->_pid = pid;
	this->_cc = 0;
	this->_need_pusi = true;
	this->_dest_spot = spot_id;
	this->log = Output::registerLog(LEVEL_WARNING,
	                                "Encap.MPEG");
}

MpegDeencapCtx::~MpegDeencapCtx()
{
}

void MpegDeencapCtx::reset()
{
	this->_data.clear();
}

unsigned int MpegDeencapCtx::length()
{
	return this->_data.size();
}

uint16_t MpegDeencapCtx::pid()
{
	return this->_pid;
}

uint8_t MpegDeencapCtx::cc()
{
	return this->_cc;
}

void MpegDeencapCtx::incCc()
{
	this->_cc = (this->_cc + 1) & 0x0f;
}

void MpegDeencapCtx::setCc(uint8_t cc)
{
	this->_cc = cc & 0x0f;
}

bool MpegDeencapCtx::need_pusi()
{
	return this->_need_pusi;
}

void MpegDeencapCtx::set_need_pusi(bool flag)
{
	this->_need_pusi = flag;
}

unsigned int MpegDeencapCtx::sndu_len()
{
	return this->_sndu_len;
}

void MpegDeencapCtx::set_sndu_len(unsigned int len)
{
	this->_sndu_len = len;
}

void MpegDeencapCtx::add(const Data &data, unsigned int offset, unsigned int length)
{
	this->_data.append(data, offset, length);
}

Data MpegDeencapCtx::data()
{
	return this->_data;
}

uint16_t MpegDeencapCtx::getDestSpot()
{
	return this->_dest_spot;
}


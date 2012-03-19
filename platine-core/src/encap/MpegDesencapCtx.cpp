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
 * @file MpegDesencapCtx.cpp
 * @brief MPEG2-TS desencapsulation context
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <MpegDesencapCtx.h>


MpegDesencapCtx::MpegDesencapCtx(uint16_t pid): _data()
{
	this->_pid = pid;
	this->_cc = 0;
	this->_need_pusi = true;
}

MpegDesencapCtx::~MpegDesencapCtx()
{
}

void MpegDesencapCtx::reset()
{
	this->_data.clear();
}

unsigned int MpegDesencapCtx::length()
{
	return this->_data.size();
}

uint16_t MpegDesencapCtx::pid()
{
	return this->_pid;
}

uint8_t MpegDesencapCtx::cc()
{
	return this->_cc;
}

void MpegDesencapCtx::incCc()
{
	this->_cc = (this->_cc + 1) & 0x0f;
}

void MpegDesencapCtx::setCc(uint8_t cc)
{
	this->_cc = cc & 0x0f;
}

bool MpegDesencapCtx::need_pusi()
{
	return this->_need_pusi;
}

void MpegDesencapCtx::set_need_pusi(bool flag)
{
	this->_need_pusi = flag;
}

unsigned int MpegDesencapCtx::sndu_len()
{
	return this->_sndu_len;
}

void MpegDesencapCtx::set_sndu_len(unsigned int len)
{
	this->_sndu_len = len;
}

void MpegDesencapCtx::add(unsigned char *data, unsigned int length)
{
	this->_data.append(data, length);
}

Data MpegDesencapCtx::data()
{
	return this->_data;
}

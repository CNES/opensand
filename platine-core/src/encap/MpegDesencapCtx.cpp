/**
 * @file MpegDesencapCtx.cpp
 * @brief MPEG2-TS desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
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


/**
 * @file MpegEncapCtx.cpp
 * @brief MPEG encapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include <MpegEncapCtx.h>


MpegEncapCtx::MpegEncapCtx(uint16_t pid)
{
	this->_frame = new Data();
	this->_pid = pid;
	this->_cc = 0;

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

void MpegEncapCtx::add(Data *data, unsigned int offset, unsigned int length)
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


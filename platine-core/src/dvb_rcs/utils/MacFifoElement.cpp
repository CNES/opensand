/**
 * @file MacFifoElement.cpp
 * @brief Fifo Element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "MacFifoElement.h"

// debug
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"

// Be careful, the maximum FIFO size sum should be smaller than the number of elements
// in the memory pool (it is the worst case)
// TODO configuration parameter for that or compute it with FIFO sizes
mgl_memory_pool MacFifoElement::mempool(sizeof(MacFifoElement), 100000,  "fifo_element");


MacFifoElement::MacFifoElement(unsigned char *data, unsigned int length,
                               long tick_in, long tick_out)
{
	this->_type = 0;
	this->_data = data;
	this->_length = length;
	this->_tick_in = tick_in;
	this->_tick_out = tick_out;
}

MacFifoElement::MacFifoElement(NetPacket *packet,
                               long tick_in, long tick_out)
{
	this->_type = 1;
	this->_packet = packet;
	this->_tick_in = tick_in;
	this->_tick_out = tick_out;
}


MacFifoElement::~MacFifoElement()
{
}

void MacFifoElement::addTrace(std::string name_function)
{
	mempool.add_function(name_function, (char *) this);
}


unsigned char *MacFifoElement::getData()
{
	return this->_data;
}

unsigned int MacFifoElement::getDataLength()
{
	return this->_length;
}

void MacFifoElement::setPacket(NetPacket *packet)
{
	this->_packet = packet;
}

NetPacket *MacFifoElement::getPacket()
{
	return this->_packet;
}

long MacFifoElement::getType()
{
	return this->_type;
}

long MacFifoElement::getTickIn()
{
	return this->_tick_in;
}

long MacFifoElement::getTickOut()
{
	return this->_tick_out;
}

/**
 * @file PhysicStd.cpp
 * @brief Generic Physical Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#include "PhysicStd.h"

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "platine_conf/uti_debug.h"


PhysicStd::PhysicStd(std::string type):
	_type(type),
	satellite_terminals()
{
	this->encapPacketType = PKT_TYPE_INVALID;
	this->frameDuration = 0;
	this->remainingCredit = 0;
	this->bandwidth = 0;
	this->tal_id = -1;
}


PhysicStd::~PhysicStd()
{
}


std::string PhysicStd::type()
{
	return this->_type;
}


int PhysicStd::onRcvEncapPacket(NetPacket *packet,
                                dvb_fifo *fifo,
                                long current_time,
                                int fifo_delay)
{
	MacFifoElement *elem;

	// create a new satellite cell to store the packet
	elem = new MacFifoElement(packet, current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		UTI_ERROR("memory pool FIFO element error, drop packet\n");
		goto error;
	}

	// append the new satellite cell in the ST FIFO of the appropriate
	// satellite spot
	if(fifo->append(elem) < 0)
	{
		UTI_ERROR("FIFO is full: drop packet\n");
		goto release_elem;
	}

	UTI_DEBUG("encapsulation packet %s stored in FIFO "
	          "(tick_in = %ld, tick_out = %ld)\n",
						packet->name().c_str(),
	          elem->getTickIn(), elem->getTickOut());

	return 0;

release_elem:
	delete elem;
error:
	delete packet;
	return -1;
}

int PhysicStd::onForwardFrame(dvb_fifo *data_fifo,
                              unsigned char *frame,
                              unsigned int length,
                              long current_time,
                              int fifo_delay)
{
	MacFifoElement *elem;

	// sanity check
	if(frame == NULL || length <= 0)
	{
		UTI_ERROR("invalid DVB burst to forward to carrier ID %d\n",
		          data_fifo->getId());
		goto error;
	}

	// get a room with timestamp in fifo
	elem = new MacFifoElement(frame, length, current_time, current_time + fifo_delay);
	if(elem == NULL)
	{
		UTI_ERROR("memory pool FIFO element error, drop packet\n");
		goto error;
	}

	// fill the delayed queue
	if(data_fifo->append(elem) < 0)
	{
		UTI_ERROR("fifo full, drop the DVB frame\n");
		goto release_elem;
	}

	UTI_DEBUG("DVB/BB frame stored in FIFO for carrier ID %d "
	          "(tick_in = %ld, tick_out = %ld)\n", data_fifo->getId(),
	          elem->getTickIn(), elem->getTickOut());

	return 0;

release_elem:
	delete elem;
error:
	g_memory_pool_dvb_rcs.release((char *) frame);
	return -1;
}

void PhysicStd::setEncapPacketType(int encap_packet_type)
{
	this->encapPacketType = encap_packet_type;
}

void PhysicStd::setFrameDuration(int frame_duration)
{
	this->frameDuration = frame_duration;
}

void PhysicStd::setBandwidth(int bandwidth)
{
	this->bandwidth = bandwidth;
}

bool PhysicStd::doSatelliteTerminalExist(long id)
{
	return this->satellite_terminals.do_exist(id);
}

bool PhysicStd::deleteSatelliteTerminal(long id)
{
	return this->satellite_terminals.del(id);
}

bool PhysicStd::addSatelliteTerminal(long id,
                unsigned long simu_column_num)
{
	return this->satellite_terminals.add(id, simu_column_num);
}

bool PhysicStd::goNextStScenarioStep()
{
	return this->satellite_terminals.goNextScenarioStep();
}

unsigned int PhysicStd::getStCurrentDraSchemeId(long id)
{
	return this->satellite_terminals.getCurrentDraSchemeId(id);
}

int PhysicStd::getRealModcod()
{
	return this->realModcod;
}

int PhysicStd::getReceivedModcod()
{
	return this->receivedModcod;
}

void PhysicStd::setTalId(long tal_id)
{
	this->tal_id = tal_id;
}

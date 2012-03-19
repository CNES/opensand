/**
 * @file SatSpot.cpp
 * @brief This bloc implements a satellite spots
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

// Logging configuration
#define DBG_PACKAGE PKG_DVB_RCS_SAT
#include "platine_conf/uti_debug.h"

#include "SatSpot.h"
#include "msg_dvb_rcs.h"
#include "MacFifoElement.h"

// Declaring SatSpot necessary ctor and dtor
SatSpot::SatSpot():
	m_ctrlFifo(),
	m_logonFifo(),
	m_dataOutGwFifo(),
	m_dataOutStFifo(),
	complete_dvb_frames()
{
	this->m_spotId = -1;
	this->m_dataInId = -1;

	// for statistics purpose
	m_dataStat.previous_tick = times(NULL);
	m_dataStat.sum_data = 0;
}

SatSpot::~SatSpot()
{
	MacFifoElement *elem;

	this->complete_dvb_frames.clear();

	// clear logon fifo
	while((elem = (MacFifoElement *) this->m_logonFifo.remove()) != NULL)
	{
		g_memory_pool_dvb_rcs.release((char *)elem->getData());
		delete elem;
	}

	// clear control fifo
	while((elem = (MacFifoElement *) this->m_ctrlFifo.remove()) != NULL)
	{
		g_memory_pool_dvb_rcs.release((char *)elem->getData());
		delete elem;
	}

	// clear data OUT ST fifo
	while((elem = (MacFifoElement *) this->m_dataOutStFifo.remove()) != NULL)
	{
		delete ((NetPacket *) elem->getPacket());
		delete elem;
	}

	// clear data OUT GW fifo
	while((elem = (MacFifoElement *) this->m_dataOutGwFifo.remove()) != NULL)
	{
		delete ((NetPacket *) elem->getPacket());
		delete elem;
	}
}


int SatSpot::init(long spotId, long logId, long ctrlId,
                  long dataInId, long dataOutStId, long dataOutGwId)
{
	m_spotId = spotId;

	// initialize MAC FIFOs
#define FIFO_SIZE 5000
	m_logonFifo.init(FIFO_SIZE);
	m_logonFifo.setId(logId);
	m_ctrlFifo.init(FIFO_SIZE);
	m_ctrlFifo.setId(ctrlId);
	m_dataInId = dataInId;
	m_dataOutStFifo.init(FIFO_SIZE);
	m_dataOutStFifo.setId(dataOutStId);
	m_dataOutGwFifo.init(FIFO_SIZE);
	m_dataOutGwFifo.setId(dataOutGwId);

	return 0;
}

long SatSpot::getSpotId()
{
	return this->m_spotId;
}


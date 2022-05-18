#include "RtChannelMux.h"
#include "RtFifo.h"


RtChannelMux::RtChannelMux(const std::string &name, const std::string &type):
	RtChannelBase{name, type},
	previous_fifos{},
	next_fifo{nullptr}
{
}


bool RtChannelMux::initPreviousFifo()
{
	bool success = true;
	for (auto&& fifo : this->previous_fifos)
		success = success && this->initSingleFifo(fifo);

	return success;
}

bool RtChannelMux::enqueueMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->next_fifo, data, size, type);
}


void RtChannelMux::addPreviousFifo(std::shared_ptr<RtFifo> &fifo)
{
	this->previous_fifos.push_back(fifo);
}


void RtChannelMux::setNextFifo(std::shared_ptr<RtFifo> &fifo)
{
	this->next_fifo = fifo;
}

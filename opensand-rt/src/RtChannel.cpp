#include "RtChannel.h"
#include "RtFifo.h"


RtChannel::RtChannel(const std::string &name, const std::string &type):
	RtChannelBase{name, type},
	previous_fifo{nullptr},
	next_fifo{nullptr}
{
}


bool RtChannel::initPreviousFifo()
{
  return this->initSingleFifo(this->previous_fifo);
}


bool RtChannel::enqueueMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->next_fifo, data, size, type);
}


void RtChannel::setPreviousFifo(std::shared_ptr<RtFifo> &fifo)
{
	this->previous_fifo = fifo;
};


void RtChannel::setNextFifo(std::shared_ptr<RtFifo> &fifo)
{
	this->next_fifo = fifo;
};

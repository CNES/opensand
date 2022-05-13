#include "RtChannel.h"
#include "RtFifo.h"


RtChannel::~RtChannel()
{
	if (this->previous_fifo)
	{
		delete this->previous_fifo;
	}
}


bool RtChannel::initPreviousFifo()
{
	if (this->previous_fifo)
	{
		if (!this->previous_fifo->init())
		{
			this->reportError(true, "cannot initialize previous fifo\n");
			return false;
		}
		if (!this->addMessageEvent(this->previous_fifo))
		{
			this->reportError(true, "cannot create previous message event\n");
			return false;
		}
	}
	return true;
}


bool RtChannel::enqueueMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->next_fifo, data, size, type);
}


void RtChannel::setPreviousFifo(RtFifo *fifo)
{
	this->previous_fifo = fifo;
};


void RtChannel::setNextFifo(RtFifo *fifo)
{
	this->next_fifo = fifo;
};

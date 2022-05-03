#include "RtChannelMux.h"
#include "RtFifo.h"

RtChannelMux::~RtChannelMux()
{
	for (RtFifo *fifo: previous_fifos)
	{
		if (fifo)
		{
			delete fifo;
		}
	}
}

bool RtChannelMux::initPreviousFifo()
{
	for (RtFifo *fifo: previous_fifos)
		if (fifo)
		{
			if (!fifo->init())
			{
				this->reportError(true, "cannot initialize previous fifo\n");
				return false;
			}
			if (!this->addMessageEvent(fifo))
			{
				this->reportError(true, "cannot create previous message event\n");
				return false;
			}
		}
	return true;
}

bool RtChannelMux::enqueueMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->next_fifo, data, size, type);
}

void RtChannelMux::addPreviousFifo(RtFifo *fifo)
{
	this->previous_fifos.push_back(fifo);
};

void RtChannelMux::setNextFifo(RtFifo *fifo)
{
	this->next_fifo = fifo;
};

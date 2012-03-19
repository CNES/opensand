/**
 * @file MacFifoElement.h
 * @brief Fifo element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef FIFO_ELEMENT_H
#define FIFO_ELEMENT_H

#include <platine_margouilla/mgl_memorypool.h>

#include <syslog.h>

#include "NetPacket.h"
#include "config.h"

/**
 * @class MacFifoElement
 * @brief Fifo element
 */
class MacFifoElement
{
 protected:

	/// 0 if the element contains a DVB frame,
	/// 1 if the element contains a NetPacket 
	long _type;

	/// The data to store in the FIFO (if type = 0)
	unsigned char *_data;
	/// The length of data
	unsigned int _length;
	/// The data to store in the FIFO (if type = 1)
	NetPacket *_packet;

	/// The arrival time of packet in FIFO (in ms)
	long _tick_in;
	/// The minimal time the packet will output the FIFO (in ms)
	long _tick_out;

	/// Pool of memory for fifo element
	static mgl_memory_pool mempool;

 public:

#if MEMORY_POOL
	inline void *operator new(size_t size) throw()
	{
		if((int) size > MacFifoElement::mempool._memBlocSize)
		{
			syslog(LOG_ERR, "too much memory asked: %u bytes "
			       "while only %ld is available", size,
			       MacFifoElement::mempool._memBlocSize);
			return NULL;
		}
		else
		{
			return MacFifoElement::mempool.get("MacFifoElement::new", size);
		}
	}

	inline void *operator new[](size_t size) throw()
	{
		if((int) size > MacFifoElement::mempool._memBlocSize)
		{
			syslog(LOG_ERR, "too much memory asked: %u bytes "
			       "while only %ld is available", size,
			       MacFifoElement::mempool._memBlocSize);
			return NULL;
		}
		else
		{
			return MacFifoElement::mempool.get("MacFifoElement::new[]", size);
		}
	}

	inline void operator delete(void *p) throw()
	{
		mempool.release((char *) p);
	}

	inline void operator delete[](void *p) throw()
	{
		mempool.release((char *) p);
	}
#endif


 public:

	/**
	 * Build a fifo element
	 * @param data     The data to store in the FIFO
	 * @param length   The length of the data
	 * @param tick_in  The arrival time of packet in FIFO (in ms)
	 * @param tick_out The minimal time the packet will output the FIFO (in ms)
	 */
	MacFifoElement(unsigned char *data, unsigned int length,
	               long tick_in, long tick_out);

	/**
	 * Build a fifo element
	 * @param packet   The data to store in the FIFO
	 * @param tick_in  The arrival time of packet in FIFO (in ms)
	 * @param tick_out The minimal time the packet will output the FIFO (in ms)
	 */
	MacFifoElement(NetPacket *packet,
	               long tick_in, long tick_out);

	/**
	 * Destroy the fifo element
	 */
	~MacFifoElement();

	/**
	 * Add trace in memory pool in order to debug memory allocation
	 */
	void addTrace(std::string name_function);

	/**
	 * Get the data
	 * @return The data
	 */
	unsigned char *getData();


	/**
	 * Get the length of data
	 * @return The length of data
	 */
	unsigned int getDataLength();


	/**
	 * Set the packet
	 *
	 * @param packet The new packet
	 */
	void setPacket(NetPacket *packet);


	/**
	 * Get the packet
	 * @return The packet
	 */
	NetPacket *getPacket();


	/**
	 * Get the type of data in FIFO element
	 * @return The type of data in FIFO element
	 */
	long getType();

	/**
	 * Get the arrival time of packet in FIFO (in ms)
	 * @return The arrival time of packet in FIFO
	 */
	long getTickIn();

	/**
	 * Get the minimal time the packet will output the FIFO (in ms)
	 * @return The minimal time the packet will output the FIFO
	 */
	long getTickOut();

};

#endif


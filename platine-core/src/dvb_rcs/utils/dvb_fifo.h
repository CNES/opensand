/**
 * @file dvb_fifo.h
 * @brief FIFO queue containing MAC ATM cells
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef DVD_FIFO_H
#define DVD_FIFO_H

#include "platine_margouilla/mgl_fifo.h"

#include <linux/param.h>
#include <sys/times.h>


// RT and NRT fifo types are used for Uor, stub, simple DAMA agent
// --> they indicate MAC QoS
#define DVB_FIFO_NRT 0
#define DVB_FIFO_RT 1

// EF, AF, BE fifo type are used fo Esa DAMA agent:
// they indicated the MAC QOS, which is equivalent in this case to Diffserv
// IP QoS
// NM and SIG fifo id used for Network Managment and Signalisation
#define DVB_FIFO_NM 0
#define DVB_FIFO_EF 1
#define DVB_FIFO_SIG 2
#define DVB_FIFO_AF 3
#define DVB_FIFO_BE 4

// each FIFO is associated to either none, RBDC or VBDC capacity request type
#define DVB_FIFO_CR_NONE 0
#define DVB_FIFO_CR_RBDC 1
#define DVB_FIFO_CR_VBDC 2



/// DVB fifo statistics context
typedef struct _MacFifoStatContext
{
	unsigned long currentPkNb; // current pk number
	unsigned long inPkNb;      // number of pk inserted during period
	unsigned long outPkNb;     // number of pk extracted during period
} MacFifoStatContext;


/**
 * @class dvb_fifo
 * @brief Defines a DVB fifo
 *
 * Manages a DVB fifo, for queuing, statistics, ...
 */
class dvb_fifo: public mgl_fifo
{
 public:

	dvb_fifo();
	dvb_fifo(int id, int kind);
	dvb_fifo(int id, int kind, int pvc);
	virtual ~dvb_fifo();

	int getKind();
	void setKind(int kind);

	void setPvc(int pvc);
	int getPvc();

	void setCrType(int crType);
	int getCrType();

	int getId();
	void setId(int id);

	long getFilled();
	long getFilledWithNoReset();
	void resetFilled(int crType);

	void setCapacity(long capacity);
	long getCapacity();

	bool allowed();
	void setThreshold(long thr);
	long getMaxSize();

	int init(long i_size);
	long append(void *ptr_data);
	void *remove();
	int flush();

	void getStatsCxt(MacFifoStatContext & statInfo);

 protected:

	int m_id;         ///< the fifo ID
	int m_kind;       ///< the QoS kind of the fifo : RT, NRT etc.. or EF, AF,
	                  ///< BE, ...
	int m_pvc;        ///< the MAC PVC associated to the FIFO.
	int m_crType;     ///< the associated Capacity Request
	long m_filled;    ///< the number of cells that filled the fifo since
	                  ///< previous check
	long m_capacity;  ///< the allocated number of cells for the current
	                  ///< superframe
	long m_size;      ///< the maximum size for that FIFO
	long m_threshold; ///< the computed threshold that should block upper
	                  ///< transmission
	MacFifoStatContext m_statContext; ///< statistics context used by MAC layer

	// Collect statistics on occupation rate à la RED
	// it sis computed like this:
	//    avg_queue_size = (1 - alpha)*avg_queue_size
	//                      +   (alpha)*current_size
	// with alpha being usual CISCO RED parameter
	// alpha = 2^{-x} we have
	// If we were n times idle between two interval of time we precomute n times
	//    avg_queue_size -= avg_queue_size >> x
	// then 1 times
	//    avg_queue_size += current_size >> x
	//
	long avg_queue_size;
	long current_size;
	clock_t previous_tick;
	clock_t this_tick;
	clock_t notice_tick;
	// NB: avg_queue_size is computed but not used yet
};

#endif

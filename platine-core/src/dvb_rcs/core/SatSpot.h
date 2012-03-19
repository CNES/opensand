/**
 * @file SatSpot.h
 * @brief This bloc implements satellite spots
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef SAT_SPOT_H
#define SAT_SPOT_H


#include <sys/times.h>
#include <map>

using namespace std;

#include "dvb_fifo.h"
#include "DvbFrame.h"


typedef struct sat_statBloc
{
	long sum_data;
	clock_t previous_tick;
} sat_StatBloc;


/**
 * @class SatSpot
 * @brief A DVB-RCS/S2 spot for the satellite emulator
 *
 */
class SatSpot
{

 private:

	long m_spotId;            ///< Internal identifier of a spot

 public: // TODO: all the variables below should be private

	long m_dataInId;          ///< the input carrier ID for the spot

	dvb_fifo m_ctrlFifo;      ///<  Fifo associated with Control carrier
	dvb_fifo m_logonFifo;     ///<  Fifo associated with Logons
	dvb_fifo m_dataOutGwFifo; ///<  Fifo associated with Data for the GW
	dvb_fifo m_dataOutStFifo; ///<  Fifo associated with Data for the ST

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	list<DvbFrame *> complete_dvb_frames;

	sat_StatBloc m_dataStat; ///< Used only with data FIFO, other are useless

 public:

	SatSpot();
	~SatSpot();

	int init(long spotId, long logId, long ctrlId,
	         long dataInId, long dataOutStId, long dataOutGwId);

	long getSpotId();

};


/// The map of satellite spots
typedef map<long, SatSpot *> SpotMap;

#endif

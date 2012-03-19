/**
 * @file bloc_dvb.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#include <string.h>
#include <errno.h>

#include "bloc_dvb.h"
#include "platine_conf/conf.h"
#include "DvbS2Std.h"

#define DBG_PREFIX
#define DBG_PACKAGE PKG_DVB_RCS
#include "platine_conf/uti_debug.h"


/**
 * Constructor
 */
BlocDvb::BlocDvb(mgl_blocmgr *blocmgr,
                 mgl_id fatherid,
                 const char *name):
	mgl_bloc(blocmgr, fatherid, name)
{
}

/**
 * Destructor
 */
BlocDvb::~BlocDvb()
{
}


/**
 * @brief Read configuration for the MODCOD definition/simulation files
 *
 * Always run this function after initEncap !
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDvb::initModcodFiles()
{
	std::string strConfig;
	std::string modcod_def_file;
	std::string modcod_simu_file;
	int bandwidth;

	// retrieve the simulation scenario
	if(globalConfig.getStringValue(GLOBAL_SECTION, DVB_SCENARIO, strConfig) < 0)
	{
		UTI_ERROR("section '%s', missing parameter '%s'\n",
		          GLOBAL_SECTION, DVB_SCENARIO);
		goto error;
	}

	if(strConfig != "individual" && strConfig != "collective")
	{
		UTI_ERROR("invalid value '%s' for config key '%s' in section '%s'\n",
		          strConfig.c_str(), DVB_SCENARIO, GLOBAL_SECTION);
		goto error;
	}

	// build the path to the modcod definition file
	modcod_def_file += MODCOD_DRA_PATH;
	modcod_def_file += "/";
	modcod_def_file += strConfig;
	modcod_def_file += "/def_modcods.txt";

	if(access(modcod_def_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
				  modcod_def_file.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod definition file = '%s'\n", modcod_def_file.c_str());

	// load all the MODCOD definitions from file
	if(!dynamic_cast<DvbS2Std *>(this->emissionStd)->loadModcodDefinitionFile(modcod_def_file))
	{
		goto error;
	}

	// build the path to the modcod simulation file
	modcod_simu_file += MODCOD_DRA_PATH;
	modcod_simu_file += "/";
	modcod_simu_file += strConfig;
	modcod_simu_file += "/sim_modcods.txt";

	if(access(modcod_def_file.c_str(), R_OK) < 0)
	{
		UTI_ERROR("cannot access '%s' file (%s)\n",
				  modcod_simu_file.c_str(), strerror(errno));
		goto error;
	}
	UTI_INFO("modcod simulation file = '%s'\n", modcod_simu_file.c_str());

	// associate the simulation file with the list of STs
	if(!dynamic_cast<DvbS2Std *>(this->emissionStd)->loadModcodSimulationFile(modcod_simu_file))
	{
		goto error;
	}

	// Get the value of the bandwidth
	if(globalConfig.getIntegerValue(GLOBAL_SECTION, BANDWIDTH,
	                                bandwidth) < 0)
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, BANDWIDTH);
		goto error;
	}

	// Set the bandwidth value for DVB-S2 emission standard
	if(this->emissionStd->type() == "DVB-S2")
	{
		this->emissionStd->setBandwidth(bandwidth);
	}

	return 0;

error:
	return -1;
}


/**
 * Send the complete DVB frames created
 * by \ref scheduleEncapPackets or
 * \ ref DvbRcsDamaAgent::globalSchedule for Terminal
 *
 * @param complete_frames the list of complete DVB frames
 * @param carrier_id      the ID of the carrier where to send the frames
 * @return 0 if successful, -1 otherwise
 */
int BlocDvb::sendBursts(std::list<DvbFrame *> *complete_frames,
                        long carrier_id)
{
	std::list<DvbFrame *>::iterator frame;
	int retval = 0;

	// send all complete DVB-RCS frames
	UTI_DEBUG_L3("send all %u complete DVB-RCS frames...\n",
	             complete_frames->size());
	for(frame = complete_frames->begin();
	    frame != complete_frames->end();
	    frame = complete_frames->erase(frame))
	{
		// Send DVB frames to lower layer
		if(!this->sendDvbFrame(dynamic_cast<DvbFrame *>(*frame), carrier_id))
		{
			retval = -1;
			continue;
		}

		// DVB frame is now sent, so delete its content
		// and remove it from the list (done in the for() statement)
		delete (*frame);
		UTI_DEBUG("complete DVB frame sent to carrier %ld\n", carrier_id);
	}

	return retval;
}

/**
 * @brief Send message to lower layer with the given DVB frame
 *
 * @param frame       the DVB frame to put in the message
 * @param carrier_id  the carrier ID used to send the message
 * @return            true on success, false otherwise
 */
bool BlocDvb::sendDvbFrame(DvbFrame *frame, long carrier_id)
{
	unsigned char *dvb_frame;
	unsigned int dvb_length;

	if(frame->totalLength() <= 0)
	{
		UTI_ERROR("empty frame, header and payload are not present\n");
		goto error;
	}

	if(frame->getNumPackets() <= 0)
	{
		UTI_ERROR("empty frame, header is present but not payload\n");
		goto error;
	}

	// get memory for a DVB frame
	dvb_frame = (unsigned char *) g_memory_pool_dvb_rcs.get(HERE());
	if(dvb_frame == NULL)
	{
		UTI_ERROR("cannot get memory for DVB frame\n");
		goto error;
	}

	// copy the DVB frame
	dvb_length = frame->totalLength();
	memcpy(dvb_frame, frame->data().c_str(), dvb_length);

	if (!this->sendDvbFrame((T_DVB_HDR *) dvb_frame, carrier_id))
	{
		UTI_ERROR("failed to send message\n");
		goto release_dvb_frame;
	}

	UTI_DEBUG("end of message sending\n");

	return true;

release_dvb_frame:
	g_memory_pool_dvb_rcs.release((char *) dvb_frame);
error:
	return false;
}


/**
 * @brief Create a margouilla message with the given DVB frame
 *        and send it to lower layer
 *
 * @param dvb_frame     the DVB frame
 * @param carrier_id    the DVB carrier Id
 * @return              true on success, false otherwise
 */
bool BlocDvb::sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id)
{
	T_DVB_META *dvb_meta; // encapsulates the DVB Frame in a structure
	mgl_msg *msg; // Margouilla message to send to lower layer

	dvb_meta = (T_DVB_META *) g_memory_pool_dvb_rcs.get(HERE());
	dvb_meta->carrier_id = carrier_id;
	dvb_meta->hdr = dvb_frame;
	
	// create the Margouilla message with burst as data
	msg = this->newMsgWithBodyPtr(msg_dvb,
	                              dvb_meta, dvb_frame->msg_length);
	if(msg == NULL)
	{
		UTI_ERROR("failed to create message to send DVB frame, drop the frame\n");
		return false;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) < 0)
	{
		UTI_ERROR("failed to send DVB frame to lower layer\n");
		return false;
	}
	UTI_DEBUG("DVB frame sent to the lower layer\n");

	return true;
}


/**
 * @brief Create a margouilla message with the given burst
 *        and sned it to upper layer
 *
 * @param burst the burst of encapsulated packets
 * @return      0 on success, -1 on error
 */

int BlocDvb::SendNewMsgToUpperLayer(NetBurst *burst)
{
	mgl_msg *msg; // Margouilla message to send to upper layer

	// create the Margouilla message with burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst,
	                              burst, sizeof(burst));
	if(msg == NULL)
	{
		UTI_ERROR("failed to create message to send burst, drop the burst\n");
		goto release_burst;
	}

	// send the message to the upper layer
	if(this->sendMsgTo(this->getUpperLayer(), msg) < 0)
	{
		UTI_ERROR("failed to send burst of packets to upper layer\n");
		goto release_burst;
	}
	UTI_DEBUG("burst sent to the upper layer\n");

	return 0;

release_burst:
	delete burst;
	return -1;
}

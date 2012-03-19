/**
 * @file bloc_dvb_rcs_tal.cpp
 *
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal, compatible
 *        with ESA Dama agent
 *
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "bloc_dvb_rcs_tal.h"
#include <sstream>
#include <assert.h>

#include "DvbRcsStd.h"
#include "DvbS2Std.h"

// environment plane
extern T_ENV_AGENT EnvAgent;

// logs configuration
#define DBG_PACKAGE PKG_DVB_RCS_TAL
#include "platine_conf/uti_debug.h"
#define DVB_DBG_PREFIX "[Tal]"

int BlocDVBRcsTal::qos_server_sock = -1;


/**
 * constructor use mgl_bloc default constructor
 * @see mgl_bloc::mgl_bloc()
 */
BlocDVBRcsTal::BlocDVBRcsTal(mgl_blocmgr *blocmgr, mgl_id fatherid, const char *name):
	BlocDvb(blocmgr, fatherid, name),
	complete_dvb_frames(),
	qos_server_host()
{
	this->init_ok = false;

	// MAC ID and registration with NCC
	this->macId = 0;
	this->_state = state_initializing;
	this->m_logonTimer = -1;

	// DAMA
	this->m_pDamaAgent = NULL;

	// carrier IDs
	this->m_carrierIdDvbCtrl = -1;
	this->m_carrierIdLogon = -1;
	this->m_carrierIdData = -1;

	// superframes and frames
	this->super_frame_counter = -1;
	this->frame_counter = -1;
	this->frames_per_superframe = -1;
	this->m_frameTimer = -1;
	this->frame_duration = -1;

	// DVB-RCS/S2 emulation
	this->emissionStd = NULL;
	this->receptionStd = NULL;
	this->m_bbframe_dropped_rate = 0;
	this->m_bbframe_dropped = 0;
	this->m_bbframe_received = 0;

	// DVB FIFOs
	this->dvb_fifos_number = -1;
	this->dvb_fifos = NULL;
	this->m_defaultFifoIndex = -1;
	this->m_nbPvc = -1;

	// misc
	this->out_encap_packet_length = -1;
	this->out_encap_packet_type = MSG_TYPE_ERROR;
	this->in_encap_packet_length = -1;
	this->m_obrPeriod = -1;
	this->m_obrSlotFrame = -1;
	this->m_fixedBandwidth = -1;
	this->m_totalAvailAlloc = -1;

	// statistics
	this->m_statCounters.ulOutgoingCells = NULL;
	this->m_statCounters.ulIncomingCells = NULL;
	this->m_statContext.ulOutgoingThroughput = NULL;
	this->m_statContext.ulIncomingThroughput = NULL;

	// QoS Server
	this->qos_server_sock = -1;
}


/**
 * Destructor
 */
BlocDVBRcsTal::~BlocDVBRcsTal()
{
	if(this->m_pDamaAgent != NULL)
	{
		delete this->m_pDamaAgent;
	}

	if(this->dvb_fifos != NULL && this->dvb_fifos_number != 0)
	{
		deletePackets();
		delete[] this->dvb_fifos;
	}

	if(this->m_statCounters.ulOutgoingCells != NULL)
		delete[] this->m_statCounters.ulOutgoingCells;
	if(this->m_statCounters.ulIncomingCells != NULL)
		delete[] this->m_statCounters.ulIncomingCells;
	if(this->m_statContext.ulOutgoingThroughput != NULL)
		delete[] this->m_statContext.ulOutgoingThroughput;
	if(this->m_statContext.ulIncomingThroughput != NULL)
		delete[] this->m_statContext.ulIncomingThroughput;

	// close QoS Server socket if it was opened
	if(this->qos_server_sock != -1)
	{
		close(this->qos_server_sock);
	}

	// release the reception and emission DVB standards
	if(this->emissionStd != NULL)
	{
		delete this->emissionStd;
	}
	if(this->receptionStd != NULL)
	{
		delete this->receptionStd;
	}

	this->complete_dvb_frames.clear();
}


/**
 * @brief The event handler
 *
 * @param event  the received event to handle
 * @return       mgl_ok if the event was correctly handled, mgl_ko otherwise
 */
mgl_status BlocDVBRcsTal::onEvent(mgl_event *event)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onEvent]";
	mgl_status status = mgl_ok;
	int ret = 0;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->init_ok)
		{
			UTI_ERROR("bloc already initialized, ignore init event\n");
		}
		else if(this->onInit() < 0)
		{
			UTI_ERROR("bloc initialization failed\n");
			ENV_AGENT_Error_Send(&EnvAgent, C_ERROR_CRITICAL, 0, 0,
			                     C_ERROR_INIT_COMPO);
		}
		else
		{
			this->init_ok = true;
			status = mgl_ok;
		}
	}
	else if(!this->init_ok)
	{
		UTI_ERROR("DVB-RCS TAL bloc not initialized, "
		          "ignore non-init event\n");
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		// beginning of a new frame
		if(MGL_EVENT_TIMER_IS_TIMER(event, this->m_frameTimer))
		{
			if(this->_state == state_running)
			{
				UTI_DEBUG("%s SF#%ld: send encap bursts on timer basis\n",
				          FUNCNAME, this->super_frame_counter);

				if(this->processOnFrameTick() < 0)
				{
					// exit because the bloc is unable to continue
					fprintf(stderr, "\n%s [processOnFrameTick] treatments at sf %ld, "
					        "frame %ld failed: see log file \n\n",
					        FUNCNAME, this->super_frame_counter,
					        this->frame_counter);
					exit(-1);
				}
			}
		}
		else if(MGL_EVENT_TIMER_IS_TIMER(event, this->m_logonTimer))
		{
			if(this->_state == state_wait_logon_resp)
			{
				// send another logon_req and raise timer
				// only if we are in the good state
				UTI_INFO("still no answer from NCC to the "
				         "logon request we sent for MAC ID %d, "
				         "send a new logon request\n",
				         this->macId);
				this->sendLogonReq();
			}
		}
		else if(MGL_EVENT_TIMER_IS_TIMER(event, this->qos_server_timer))
		{
			// try to re-connect to QoS Server if not already connected
			if(this->qos_server_sock == -1)
			{
				if(!this->connectToQoSServer())
				{
					UTI_DEBUG("%s failed to connect with QoS Server, cannot "
					          "send cross layer information\n", FUNCNAME);
				}
			}

			// check connection status in 5 seconds
			this->setTimer(this->qos_server_timer, 5000);
		}
		else
		{
			UTI_ERROR("%s SF#%ld: unknown timer event received\n",
			          FUNCNAME, this->super_frame_counter);
			status = mgl_ko;
		}
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		if(MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
		{
			// messages from upper layer: burst of encapsulation packets
			NetBurst *burst;
			NetBurst::iterator pkt_it;
			int fifo_id;
			std::string message;
			std::ostringstream oss;
			int i;
			int ret;

			burst = (NetBurst *) MGL_EVENT_MSG_GET_BODY(event);

			UTI_DEBUG("SF#%ld: encapsulation burst received (%d packets)\n",
			          this->super_frame_counter, burst->length());

			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
			{
				UTI_DEBUG_L3("SF#%ld: encapsulation packet has QoS value %d\n",
				             this->super_frame_counter, (*pkt_it)->qos());

				// find the FIFO id (!= FIFO index)
				if((*pkt_it)->qos() == -1)
					fifo_id = this->dvb_fifos[m_defaultFifoIndex].getId();
				else
					fifo_id = (*pkt_it)->qos();

				UTI_DEBUG("SF#%ld: store one encapsulation packet (QoS = %d)\n",
				          this->super_frame_counter, fifo_id);
				(*pkt_it)->addTrace(HERE());

				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				for(i = 0; i < this->dvb_fifos_number; i++)
				{
					if(this->dvb_fifos[i].getId() == fifo_id)
						break;
				}
				if(i == this->dvb_fifos_number)
				{
					UTI_ERROR("SF#%ld: frame %ld: MAC FIFO ID #%d not"
					          "registered => packet dropped\n",
					          this->super_frame_counter,
					          this->frame_counter, fifo_id);
					delete (*pkt_it);
					continue;
				}

				// store the encapsulation packet in the FIFO
				if(this->emissionStd->onRcvEncapPacket(
				   *pkt_it,
				   &(this->dvb_fifos[i]),
				   this->getCurrentTime(),
				   0) < 0)
				{
					// A problem occured. Trace it but carry on simulation
					UTI_ERROR("SF#%ld: frame %ld: unable to "
					          "store received encapsulation "
					          "packet (see previous errors)\n",
					          this->super_frame_counter,
					          this->frame_counter);
				}

				// update incoming counter (if packet is stored or sent)
				m_statCounters.ulIncomingCells[i]++;
			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;

			// Cross layer information: if connected to QoS Server, build XML
			// message and send it
			if(this->qos_server_sock == -1)
			{
				goto not_connected;
			}

 			message = "";
			message.append("<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n");
			message.append("<XMLQoSMessage>\n");
			message.append(" <Sender>");
			message.append("CrossLayer");
			message.append("</Sender>\n");
			message.append(" <Type type=\"CrossLayer\" >\n");
			message.append(" <Infos ");
			for(i = 0; i < this->dvb_fifos_number; i++)
			{
				int nbFreeFrames = this->dvb_fifos[i].getMaxSize() -
				                   this->dvb_fifos[i].getCount();
				int nbFreeBits = nbFreeFrames * this->out_encap_packet_length * 8; // bits
				float macRate = nbFreeBits / this->frame_duration ; // bits/ms or kbits/s
				oss << "File=\"" << (int) macRate << "\" ";
				message.append(oss.str());
				oss.str("");
			}
			message.append("/>");
			message.append(" </Type>\n");
			message.append("</XMLQoSMessage>\n");

			ret = write(this->qos_server_sock, message.c_str(), message.length());
			if(ret == -1)
			{
				UTI_NOTICE("%s failed to send message to QoS Server: %s (%d)\n",
				           FUNCNAME, strerror(errno), errno);
			}

not_connected:
			;
		}
		else if(MGL_EVENT_MSG_IS_TYPE(event, msg_dvb))
		{
			unsigned char *dvb_frame;
			long len;
			T_DVB_META *dvb_meta;
			long carrier_id;

			dvb_meta = (T_DVB_META *) MGL_EVENT_MSG_GET_BODY(event);
			carrier_id = dvb_meta->carrier_id;
			dvb_frame = (unsigned char *) dvb_meta->hdr;
			len = MGL_EVENT_MSG_GET_BODYLEN(event);

			// message from lower layer: DL dvb frame
			UTI_DEBUG_L3("SF#%ld DVB frame received (len %ld)\n",
				     this->super_frame_counter, len);

			ret = onRcvDVBFrame(dvb_frame, len);
			if(ret != 0)
			{
				UTI_DEBUG_L3("SF#%ld: failed to handle received DVB frame\n",
				             this->super_frame_counter);
				// a problem occured, trace is made in onRcvDVBFrame()
				// carry on simulation
				status = mgl_ko;
			}
			// TODO: release frame ?
			g_memory_pool_dvb_rcs.release((char *) dvb_meta);
		}
		else
		{
			UTI_ERROR("SF#%ld: unknown message event received\n",
			          this->super_frame_counter);
			status = mgl_ko;
		}
	}
	else
	{
		UTI_ERROR("SF#%ld: unknown event received\n",
		          this->super_frame_counter);
		status = mgl_ko;
	}

	return status;
}


/**
 * @brief Initialize the transmission mode
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsTal::initMode()
{
	this->emissionStd = new DvbRcsStd();
	if(this->emissionStd == NULL)
	{
		UTI_ERROR("failed to create the emission standard\n");
		goto error;
	}

	this->receptionStd = new DvbS2Std();
	if(this->receptionStd == NULL)
	{
		UTI_ERROR("failed to create the reception standard\n");
		goto release_emission;
	}

	return 0;

release_emission:
	delete(this->emissionStd);
error:
	return -1;
}


/**
 * Read configuration for the encapsulation
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsTal::initEncapsulation()
{
	string out_encap_scheme;
	string in_encap_scheme;

	// read uplink encapsulation packet length from config
	if(globalConfig.getStringValue(GLOBAL_SECTION, OUT_ENCAP_SCHEME,
	                               out_encap_scheme) < 0)
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, OUT_ENCAP_SCHEME);
		goto error;
	}

	// check uplink encapsulation packet length
	if(out_encap_scheme == ENCAP_ATM_AAL5 ||
	   out_encap_scheme == ENCAP_ATM_AAL5_ROHC)
	{
		this->out_encap_packet_length = AtmCell::length();
		this->out_encap_packet_type = PKT_TYPE_ATM;
	}
	else if(out_encap_scheme == ENCAP_MPEG_ULE ||
	        out_encap_scheme == ENCAP_MPEG_ULE_ROHC)
	{
		this->out_encap_packet_length = MpegPacket::length();
		this->out_encap_packet_type = PKT_TYPE_MPEG;
	}
	//TODO only for a DVB-S2 uplink (not implemented yet) !!
#if 0
	else if(out_encap_scheme == ENCAP_GSE)
	{
		this->out_encap_packet_length = GsePacket::length();
		//TODO get real GSE packet length (used for FIFO stats)
		this->out_encap_packet_type = PKT_TYPE_GSE;
	}
#endif
	else
	{
		UTI_ERROR("bad value '%s' for uplink encapsulation "
		          "protocol\n", out_encap_scheme.c_str());
		goto error;
	}
	UTI_INFO("uplink encapsulation packet length = %d bytes\n",
	this->out_encap_packet_length);

	// read downlink encapsulation packet length from config
	if(globalConfig.getStringValue(GLOBAL_SECTION, IN_ENCAP_SCHEME,
	   in_encap_scheme) < 0)
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          GLOBAL_SECTION, IN_ENCAP_SCHEME);
		goto error;
	}

	// set the encapsulation packet type for emission standard
	this->emissionStd->setEncapPacketType(this->out_encap_packet_type);

	// compute downlink encapsulation packet length
	if(in_encap_scheme == ENCAP_MPEG_ATM_AAL5 ||
	   in_encap_scheme == ENCAP_MPEG_ULE ||
	   in_encap_scheme == ENCAP_MPEG_ULE_ROHC ||
	   in_encap_scheme == ENCAP_MPEG_ATM_AAL5_ROHC)
	{
		this->in_encap_packet_length = MpegPacket::length();
	}
	else if(in_encap_scheme == ENCAP_GSE ||
	        in_encap_scheme == ENCAP_GSE_ATM_AAL5 ||
	        in_encap_scheme == ENCAP_GSE_MPEG_ULE ||
	        in_encap_scheme == ENCAP_GSE_ROHC ||
	        in_encap_scheme == ENCAP_GSE_ATM_AAL5_ROHC ||
	        in_encap_scheme == ENCAP_GSE_MPEG_ULE_ROHC)
	{
		this->in_encap_packet_length = 0;//GsePacket::length();
		//TODO get the real packet length (used for FIFO stats)
	}
	else
	{
		UTI_ERROR("bad value '%s' for downlink encapsulation "
		          "protocol\n", in_encap_scheme.c_str());
		goto error;
	}
	UTI_INFO("downlink encapsulation packet length = %d bytes\n",
	          this->in_encap_packet_length);

	return 0;

error:
	return -1;
}


/**
 * Read configuration for the parameters
 *
 * @return -1 if failed, 0 if succeed
 */
int BlocDVBRcsTal::initParameters()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	string strConfig;
	int val;

#define FMT_KEY_MISSING "%s SF#%ld %s missing from section %s\n",FUNCNAME,this->super_frame_counter

	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_RT_BANDWIDTH, val) < 0)
	{
		val = DA_DFLT_RT_BANDWIDTH;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n", FUNCNAME,
		          DVB_RT_BANDWIDTH, val);
		goto error;
	}
	this->m_fixedBandwidth = val;
	UTI_INFO("fixed_bandwidth = %d kbits/s\n", this->m_fixedBandwidth);

	// Get the satellite MAC address
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_MAC_ID, val) < 0)
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_TAL_SECTION, DVB_MAC_ID);
		goto error;
	}
	this->macId = val;

	// Get the number of the row in modcod and dra files
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_SIMU_COL, val) < 0
	   || val > NB_MAX_ST)
	{
		UTI_ERROR("section '%s': missing parameter '%s'\n",
		          DVB_TAL_SECTION, DVB_SIMU_COL);
		goto error;
	}
	this->m_nbRow = val;
	UTI_INFO("nb row = %d\n", this->m_nbRow);

	return 0;

error:
	return -1;
}


/**
 * Read configuration for the frame
 */
void BlocDVBRcsTal::initFrame()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	int val;

#define FMT_KEY_MISSING "%s SF#%ld %s missing from section %s\n",FUNCNAME,this->super_frame_counter

	// nb of frames per superframe
	if(globalConfig.
	   getIntegerValue(DVB_MAC_LAYER_SECTION, DVB_FRMS_PER_SUPER, val) < 0)
	{
		val = DFLT_FRMS_PER_SUPER;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n", FUNCNAME,
		          DVB_FRMS_PER_SUPER, val);
	}
	this->frames_per_superframe = val;

	// Frame duration - in ms
	if(globalConfig.
	   getIntegerValue(DVB_MAC_LAYER_SECTION, DVB_FRM_DURATION, val) < 0)
	{
		val = DFLT_FRM_DURATION;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n", FUNCNAME,
		          DVB_FRM_DURATION, val);
	}
	this->frame_duration = val;
}

/**
 * Read configuration for the carrier ID
 *
 * @return -1 if failed, 0 if succeed
 */
int BlocDVBRcsTal::initCarrierId()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	int val;

#define FMT_KEY_MISSING "%s SF#%ld %s missing from section %s\n",FUNCNAME,this->super_frame_counter

	 // Get the carrier Id m_carrierIdDvbCtrl
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_CAR_ID_CTRL, val) < 0)
	{
		UTI_ERROR(FMT_KEY_MISSING, DVB_CAR_ID_CTRL, DVB_TAL_SECTION);
		return -1;
	}
	this->m_carrierIdDvbCtrl = val;

	// Get the carrier Id m_carrierIdLogon
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_CAR_ID_LOGON, val) < 0)
	{
		UTI_ERROR(FMT_KEY_MISSING, DVB_CAR_ID_LOGON, DVB_TAL_SECTION);
		return -1;
	}
	this->m_carrierIdLogon = val;

	// Get the carrier Id m_carrierIdData
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_CAR_ID_DATA, val) < 0)
	{
		UTI_ERROR(FMT_KEY_MISSING, DVB_CAR_ID_DATA, DVB_TAL_SECTION);
		return -1;
	}
	this->m_carrierIdData = val;

	UTI_INFO("SF#%ld: carrier IDs for Ctrl = %ld, Logon = %ld, Data = %ld\n",
	         this->super_frame_counter, this->m_carrierIdDvbCtrl,
	         this->m_carrierIdLogon, this->m_carrierIdData);

	return 0;

}

/**
 * Read configuration for the MAC FIFOs
 *
 * @return -1 if failed, 0 if succeed
 */
int BlocDVBRcsTal::initMacFifo()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	string strConfig;
	int i;
	int ret;
	int fifoId;
	int fifoSize;
	char fifoKind[10];
	char fifoCrType[10];
	int highestPrioMacFifoIndex;
	int pvc;
	int lastPvc;

#define FMT_KEY_MISSING "%s SF#%ld %s missing from section %s\n",FUNCNAME,this->super_frame_counter

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	this->dvb_fifos_number = globalConfig.getNbListItems(DVB_TAL_SECTION);
	if(this->dvb_fifos_number <= 0)
	{
		UTI_ERROR("invalid number of DVB FIFOs defined in section '%s' "
		          "of configuration file\n", DVB_TAL_SECTION);
		goto error;
	}
	this->dvb_fifos = new dvb_fifo[this->dvb_fifos_number];
	if(this->dvb_fifos == NULL)
	{
		UTI_ERROR("failed to create DVB FIFOs\n");
		goto error;
	}
	UTI_INFO("%d DVB FIFOs defined in section [%s]\n",
	this->dvb_fifos_number, DVB_TAL_SECTION);

	lastPvc = 0;
	for(i = 0; i < this->dvb_fifos_number; i++)
	{
		ret = globalConfig.getListItem(DVB_TAL_SECTION, i + 1, strConfig);
		if(ret < 0)
		{
			UTI_ERROR("%s SF#%ld: cannot get listItem from section '%s'\n",
			          FUNCNAME, this->super_frame_counter, DVB_TAL_SECTION);
			goto err_fifo_release;
		}
		sscanf(strConfig.c_str(), "%d %s %d %d %s", &fifoId, fifoKind,
		       &fifoSize, &pvc, fifoCrType);

		// DVB fifo kind is the MAC QoS. With Esa DAMA it is the same
		// as Diffserv IP QoS: it can be AF, EF, BE
		// NM and SIG filters are for Network Managment and Signalisation
		if(strcmp(fifoKind, "NM") == 0)
			this->dvb_fifos[i].setKind(DVB_FIFO_NM);
		else if(strcmp(fifoKind, "EF") == 0)
			this->dvb_fifos[i].setKind(DVB_FIFO_EF);
		else if(strcmp(fifoKind, "SIG") == 0)
			this->dvb_fifos[i].setKind(DVB_FIFO_SIG);
		else if(strcmp(fifoKind, "AF") == 0)
			this->dvb_fifos[i].setKind(DVB_FIFO_AF);
		else if(strcmp(fifoKind, "BE") == 0)
			this->dvb_fifos[i].setKind(DVB_FIFO_BE);
		else if(strcmp(fifoKind, "RT") == 0 || strcmp(fifoKind, "NRT") == 0)
		{
			UTI_ERROR("%s SF#%ld: kind of fifo not managed by ESA DAMA agent: "
			          "%s\n", FUNCNAME, this->super_frame_counter, fifoKind);
			goto err_fifo_release;
		}
		else
		{
			UTI_ERROR("%s SF#%ld: unknown kind of fifo: %s\n", FUNCNAME,
			          this->super_frame_counter, fifoKind);
			goto err_fifo_release;
		}

		// set PVC id: several FIFOs can be gathered in a single PVC
		// PVC id must be > 0
		if(pvc <= 0)
		{
			UTI_ERROR("%s SF#%ld: PVC %d is not valid (first PVC id is 1)\n",
			          FUNCNAME, this->super_frame_counter, pvc);
			goto err_fifo_release;
		}
		// PVC ids must be in increasing order
		else if(pvc < lastPvc)
		{
			UTI_ERROR("%s SF#%ld: PVC %d is not valid (PVC ids must be in "
			          "increasing order\n", FUNCNAME, this->super_frame_counter,
			          pvc);
			goto err_fifo_release;
		}
		else
		{
			this->dvb_fifos[i].setPvc(pvc);
			lastPvc = pvc;
		}

		// capacity request type associated to the Fifo: NONE, RBDC or VBDC
		if(strcmp(fifoCrType, "RBDC") == 0)
			this->dvb_fifos[i].setCrType(DVB_FIFO_CR_RBDC);
		else if(strcmp(fifoCrType, "VBDC") == 0)
			this->dvb_fifos[i].setCrType(DVB_FIFO_CR_VBDC);
		else if(strcmp(fifoCrType, "NONE") == 0)
		{
			// this will be used by DAMA agent for CR computation
			this->dvb_fifos[i].setCrType(DVB_FIFO_CR_NONE);
		}
		else
		{
			UTI_ERROR("%s SF#%ld: unknown CR type of FIFO: %s\n", FUNCNAME,
			          this->super_frame_counter, fifoCrType);
			goto err_fifo_release;
		}

		// set other DVB FIFOs atributes
		this->dvb_fifos[i].setId(fifoId);
		this->dvb_fifos[i].init(fifoSize);

		UTI_INFO("SF#%ld: Fifo = id %d, kind %d, size %ld, pvc %d, "
		         "CR type %d\n", this->super_frame_counter,
		this->dvb_fifos[i].getId(),
		this->dvb_fifos[i].getKind(),
		this->dvb_fifos[i].getMaxSize(),
		this->dvb_fifos[i].getPvc(),
		this->dvb_fifos[i].getCrType());

	} // end for(queues are now instanciated and initialized)

	// the default FIFO is the last one = the one with the smallest priority
	m_defaultFifoIndex = i - 1;

	// the fifo with the highest priority is the first one
	highestPrioMacFifoIndex = 0;

	// set the number of PVC = the maximum PVC is (first PVC is is 1)
	m_nbPvc = 0;
	for(i = 0; i < this->dvb_fifos_number; i++)
	{
		m_nbPvc = MAX(this->dvb_fifos[i].getPvc(), m_nbPvc);
	}

	// init stats context per QoS
	m_statContext.ulIncomingThroughput = new int[this->dvb_fifos_number];
	if(this->m_statContext.ulIncomingThroughput == NULL)
		goto err_fifo_release;

	m_statContext.ulOutgoingThroughput = new int[this->dvb_fifos_number];
	if(this->m_statContext.ulOutgoingThroughput == NULL)
		goto err_incoming_throughput_release;

	m_statCounters.ulIncomingCells = new int[this->dvb_fifos_number];
	if(this->m_statCounters.ulIncomingCells == NULL)
		goto err_outgoing_throughput_release;

	m_statCounters.ulOutgoingCells = new int[this->dvb_fifos_number];
	if(this->m_statCounters.ulOutgoingCells == NULL)
		goto err_incoming_cells_release;

	resetStatsCxt();

	return 0;

err_incoming_cells_release:
	delete[] this->m_statCounters.ulIncomingCells;
err_outgoing_throughput_release:
	delete[] this->m_statContext.ulOutgoingThroughput;
err_incoming_throughput_release:
	delete[] this->m_statContext.ulIncomingThroughput;
err_fifo_release:
	delete[] this->dvb_fifos;
error:
	return -1;
}


/**
 * Read configuration for the OBR period
 */
void BlocDVBRcsTal::initObr()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	int val;

#define FMT_KEY_MISSING "%s SF#%ld %s missing from section %s\n",FUNCNAME,this->super_frame_counter

	// get the OBR period - in number of frames
	if(globalConfig.getIntegerValue(DVB_TAL_SECTION, DVB_OBR_PERIOD_DATA, val) < 0)
	{
		val = DA_DFLT_OBR_PERIOD_DATA;
		UTI_ERROR("%s Missing %s => taking default value (%d)\n", FUNCNAME,
		          DVB_OBR_PERIOD_DATA, val);
	}
	m_obrPeriod = val;

	// deduce the Obr slot position within the multi-frame, from the mac
	// address and the OBR period
	// ObrSlotFrame= MacAddress 'modulo' Obr Period
	// NB : ObrSlotFrame is within [0, Obr Period -1]
	m_obrSlotFrame = macId % m_obrPeriod;
	UTI_INFO("%s SF#%ld: MAC adress = %d, OBR period = %d, "
	         "OBR slot frame = %d\n", FUNCNAME, this->super_frame_counter,
	         macId, m_obrPeriod, m_obrSlotFrame);
}


/**
 * Read configuration for the DAMA algorithm
 * @return -1 if failed, 0 if succeed
 */
int BlocDVBRcsTal::initDama()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onInit]";
	string strConfig;
	int ret;

#define FMT_KEY_MISSING "%s SF#%ld %s missing from section %s\n",FUNCNAME,this->super_frame_counter

	// get and set the dama algorithm
	if(globalConfig.getStringValue(DVB_TAL_SECTION, DVB_DAMA_ALGO, strConfig) < 0)
	{
		UTI_ERROR("%s SF#%ld Can't get DAMA algorithm name\n", FUNCNAME,
		          this->super_frame_counter);
		goto error;
	}

	if(strConfig == "ESA")
	{
		UTI_INFO("%s SF#%ld: create ESA DAMA agent\n", FUNCNAME,
		         this->super_frame_counter);
		m_pDamaAgent = new DvbRcsDamaAgentEsa();
	}
	else if(strConfig == "UoR")
	{
		UTI_INFO("%s SF#%ld: create UoR DAMA agent\n", FUNCNAME,
		         this->super_frame_counter);
		m_pDamaAgent = new DvbRcsDamaAgentUoR();
	}
	else
	{
		UTI_ERROR("cannot create DAMA agent: algo named '%s' is not "
		          "managed by current MAC layer\n", strConfig.c_str());
		goto error;
	}

	if(this->m_pDamaAgent == NULL)
	{
		UTI_ERROR("failed to create DAMA agent\n");
		goto error;
	}

	// call the onInit() method of the Dama algorithm
	ret = m_pDamaAgent->initComplete(this->dvb_fifos, this->dvb_fifos_number,
	                                 (double) (this->frame_duration / 1000.0),
	                                 m_fixedBandwidth, m_obrPeriod);
	if(ret != 0)
	{
		UTI_ERROR("%s SF#%ld Dama Agent Initialization failed.\n", FUNCNAME,
		          this->super_frame_counter);
		goto err_agent_release;
	}

	return 0;

err_agent_release:
	delete m_pDamaAgent;
error:
	return -1;
}


/**
 * Read configuration for the QoS Server
 * @return -1 if failed, 0 if succeed
 */
int BlocDVBRcsTal::initQoSServer()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[initQoSServer]";
	int ret;

	// QoS Server: read hostname and port from configuration
	ret = globalConfig.getStringValue(SECTION_QOS_AGENT, QOS_SERVER_HOST,
	                                  this->qos_server_host);
	if(ret < 0)
	{
		UTI_INFO("%s section %s, %s missing. QoS Server address defaults to %s.\n",
		         FUNCNAME, SECTION_QOS_AGENT, QOS_SERVER_HOST, QOS_SERVER_DFLT_HOST);
		this->qos_server_host = QOS_SERVER_DFLT_HOST;
	}
	ret = globalConfig.getIntegerValue(SECTION_QOS_AGENT, QOS_SERVER_PORT,
	                                   this->qos_server_port);
	if(ret < 0)
	{
		UTI_INFO("%s section %s, %s missing. QoS Server port defaults to %d.\n",
		         FUNCNAME, SECTION_QOS_AGENT, QOS_SERVER_PORT, QOS_SERVER_DFLT_PORT);
		this->qos_server_port = QOS_SERVER_DFLT_PORT;

		if(this->qos_server_port <= 0 || this->qos_server_port > 0xffff)
		{
			UTI_ERROR("%s default QoS Server port (%d) not valid, break init\n",
			          FUNCNAME, QOS_SERVER_DFLT_PORT);
			goto error;
		}
	}
	else if(this->qos_server_port <= 0 || this->qos_server_port > 0xffff)
	{
		UTI_INFO("%s QoS Server port (%d) not valid, defaults to %d.\n",
		         FUNCNAME, this->qos_server_port, QOS_SERVER_DFLT_PORT);
		this->qos_server_port = QOS_SERVER_DFLT_PORT;
	}

	// QoS Server: catch the SIGFIFO signal that is sent to the process
	// when QoS Server kills the TCP connection
	if(signal(SIGPIPE, BlocDVBRcsTal::closeQosSocket) == SIG_ERR)
	{
		printf("cannot catch signal SIGPIPE\n");
		goto error;
	}

	// QoS Server: try to connect to remote host
	this->connectToQoSServer();

	// QoS Server: check connection status in 5 seconds
	this->setTimer(this->qos_server_timer, 5000);

	return 0;
error:
	return -1;
}


/**
 * @brief Initialize the DVBRCS TAL block
 *
 * @return  0 in case of success, -1 otherwise
 */
int BlocDVBRcsTal::onInit()
{
	int ret;

	// get the transmission mode (transparent/regenerative, delay...)
	ret = this->initMode();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the mode part of the "
		          "initialisation");
		goto error;
	}

	ret = this->initEncapsulation();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the encapsulation part of the "
		          "initialisation");
		goto error;
	}

	ret = this->initParameters();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the 'parameters' part of the "
		          "initialisation");
		goto error;
	}

	this->initFrame();

	ret = this->initCarrierId();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the carrier IDs part of the "
		          "initialisation");
		goto error;
	}

	ret = this->initMacFifo();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the MAC FIFO part of the "
		          "initialisation");
		goto error;
	}

	this->initObr();

	ret = this->initDama();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the DAMA part of the "
		          "initialisation");
		goto error;
	}

	ret = this->initQoSServer();
	if(ret != 0)
	{
		UTI_ERROR("failed to complete the QoS Server part of the "
		          "initialisation");
		goto error;
	}

	// after all of things have been initialized successfully,
	// send a logon request
	UTI_DEBUG("send a logon request with MAC ID %d to NCC\n", this->macId);
	this->_state = state_wait_logon_resp;
	if(this->sendLogonReq() < 0)
	{
		UTI_ERROR("failed to send the logon request to the NCC");
		goto error;
	}

	return 0;

error:
	return -1;
}


// TODO: move to a dedicated class
/**
 * Signal callback called upon SIGFIFO reception.
 *
 * This function is declared as static.
 *
 * @param sig  The signal that called the function
 */
void BlocDVBRcsTal::closeQosSocket(int sig)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[BlocDVBRcsTal::closeQosSocket]";
	UTI_NOTICE("%s TCP connection broken, close socket\n", FUNCNAME);
	close(BlocDVBRcsTal::qos_server_sock);
	BlocDVBRcsTal::qos_server_sock = -1;
}


// TODO: move to a dedicated class
/**
 * Try to connect to the QoS Server
 *
 * The qos_server_host and qos_server_port class variables must be correctly
 * initialized. The qos_server_sock variable should be -1 when calling this
 * function.
 *
 * @return   true if connection is successful, false otherwise
 */
bool BlocDVBRcsTal::connectToQoSServer()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[BlocDVBRcsTal::connectToQoSServer]";
	struct addrinfo hints;
	struct protoent *tcp_proto;
	struct servent *serv;
	struct addrinfo *addresses;
	struct addrinfo *address;
	char straddr[INET6_ADDRSTRLEN];
	int ret;

	if(qos_server_sock != -1)
	{
		UTI_NOTICE("%s already connected to QoS Server, do not call this "
		           "function when already connected\n", FUNCNAME);
		goto skip;
	}

	// set criterias to resolve hostname
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// get TCP protocol number
	tcp_proto = getprotobyname("TCP");
	if(tcp_proto == NULL)
	{
		UTI_NOTICE("%s TCP is not available on the system\n", FUNCNAME);
		goto error;
	}
	hints.ai_protocol = tcp_proto->p_proto;

	// get service name
	serv = getservbyport(htons(this->qos_server_port), "tcp");
	if(serv == NULL)
	{
		UTI_NOTICE("%s service on TCP/%d is not available\n", FUNCNAME,
		           this->qos_server_port);
		goto error;
	}

	// resolve hostname
	ret = getaddrinfo(this->qos_server_host.c_str(), serv->s_name, &hints, &addresses);
	if(ret != 0)
	{
		UTI_NOTICE("%s cannot resolve hostname '%s': %s (%d)\n",
		           FUNCNAME, this->qos_server_host.c_str(), gai_strerror(ret), ret);
		goto error;
	}

	// try to create socket with available addresses
	address = addresses;
	while(address != NULL && this->qos_server_sock == -1)
	{
		bool is_ipv4;
		void *sin_addr;
		const char *retptr;

		is_ipv4 = (address->ai_family == AF_INET);
		if(is_ipv4)
			sin_addr = &((struct sockaddr_in *) address->ai_addr)->sin_addr;
		else // ipv6
			sin_addr = &((struct sockaddr_in6 *) address->ai_addr)->sin6_addr;

		retptr = inet_ntop(address->ai_family, sin_addr, straddr, sizeof(straddr));
		if(retptr != NULL)
		{
			UTI_INFO("%s try IPv%d address %s\n", FUNCNAME, is_ipv4 ? 4 : 6, straddr);
		}
		else
		{
			UTI_INFO("%s try an IPv%d address\n", FUNCNAME, is_ipv4 ? 4 : 6);
		}

		this->qos_server_sock = socket(address->ai_family, address->ai_socktype,
		                               address->ai_protocol);
		if(this->qos_server_sock == -1)
		{
			UTI_NOTICE("%s cannot create socket (%s) with address %s\n",
			           FUNCNAME, strerror(errno), straddr);
			address = address->ai_next;
			continue;
		}

		UTI_INFO("%s socket created for address %s\n", FUNCNAME, straddr);
	}

	if(this->qos_server_sock == -1)
	{
		UTI_NOTICE("%s no valid address found for hostname %s\n", FUNCNAME,
		           this->qos_server_host.c_str());
		goto free_dns;
	}

	UTI_INFO("%s try to connect with QoS Server at %s[%s]:%d\n", FUNCNAME,
	this->qos_server_host.c_str(), straddr, this->qos_server_port);

	// try to connect with the socket
	ret = connect(this->qos_server_sock, address->ai_addr, address->ai_addrlen);
	if(ret == -1)
	{
		UTI_NOTICE("%s connect() failed: %s (%d)\n", FUNCNAME, strerror(errno), errno);
		UTI_NOTICE("%s will retry to connect later\n", FUNCNAME);
		goto close_socket;
	}

	UTI_INFO("%s connected with QoS Server at %s[%s]:%d\n", FUNCNAME,
	         this->qos_server_host.c_str(), straddr, this->qos_server_port);

	// clean allocated addresses
	freeaddrinfo(addresses);

skip:
	return true;

close_socket:
	close(this->qos_server_sock);
	this->qos_server_sock = -1;
free_dns:
	freeaddrinfo(addresses);
error:
	return false;
}


/**
 * This method send a Logon Req message
 *
 * @return -1 if failed, 0 if succeed
 */
int BlocDVBRcsTal::sendLogonReq()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[sendLogonReq]";
	T_DVB_LOGON_REQ *lp_logon_req;
	long l_size;

	// create a new DVB frame
	lp_logon_req = (T_DVB_LOGON_REQ *) g_memory_pool_dvb_rcs.get(HERE());
	if(!lp_logon_req)
	{
		UTI_ERROR("SF#%ld: failed to allocate memory for LOGON "
		          "request\n", this->super_frame_counter);
		goto error;
	}

	// build the DVB header
	l_size = sizeof(T_DVB_LOGON_REQ);
	lp_logon_req->hdr.msg_length = l_size;
	lp_logon_req->hdr.msg_type = MSG_TYPE_SESSION_LOGON_REQ;
	lp_logon_req->mac = macId;
	lp_logon_req->nb_row = m_nbRow;
	lp_logon_req->rt_bandwidth = m_fixedBandwidth;	/* in kbits/s */

	// send the message to the lower layer
	if(!this->sendDvbFrame((T_DVB_HDR *) lp_logon_req, m_carrierIdLogon))
	{
		UTI_ERROR("%s Failed to send Logon Request\n", FUNCNAME);
		goto free_logon_req;
	}
	UTI_DEBUG_L3("%s SF#%ld Logon Req. sent to lower layer\n", FUNCNAME,
	             this->super_frame_counter);

	// try to log on again after some time in case of failure
	this->setTimer(m_logonTimer, 5000);

	// send the corresponding event
	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, macId, 0, C_EVENT_LOGIN_SENT);

	return 0;

free_logon_req:
	g_memory_pool_dvb_rcs.release((char *) lp_logon_req);
error:
	return -1;
}


/**
 * Manage the receipt of the DVB Frames
 * @param ip_buf the data buffer
 * @param i_len the length of the buffer
 * @return -2 if error on TBTP , -1 if another error occurred, 0 if succeed
 */
int BlocDVBRcsTal::onRcvDVBFrame(unsigned char *ip_buf, long i_len)
{
	T_DVB_HDR *hdr;

	g_memory_pool_dvb_rcs.add_function(std::string(__FUNCTION__), (char *) ip_buf);

	// Get msg header
	hdr = (T_DVB_HDR *) ip_buf;

	switch(hdr->msg_type)
	{
		case MSG_TYPE_BBFRAME:
		{
			// keep statistics because data will be released before storing them
			unsigned int data_len = ((T_DVB_BBFRAME *) ip_buf)->dataLength;
			NetBurst *burst;

			if(this->receptionStd->onRcvFrame(ip_buf, i_len, hdr->msg_type,
			                                  this->macId, &burst) < 0)
			{
				UTI_ERROR("failed to handle the reception of "
				          "BB frame (length = %ld)\n", i_len);
				goto error;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}

			// update statistics
			this->m_statCounters.dlOutgoingCells += data_len;

			break;
		}

		case MSG_TYPE_DVB_BURST:
		{
			// keep statistics because data will be released before storing them
			unsigned int nb_cells = 0;
			NetBurst *burst;

			switch(((T_DVB_ENCAP_BURST *) ip_buf)->pkt_type)
			{
				case(PKT_TYPE_ATM):
					nb_cells = (hdr->msg_length - sizeof(T_DVB_HDR)) /
					            AtmCell::length();
					break;

				case(PKT_TYPE_MPEG):
					nb_cells = (hdr->msg_length - sizeof(T_DVB_HDR)) /
					            MpegPacket::length();
					break;

				default:
					UTI_ERROR("bad packet type (%d) in DVB burst\n",
					          ((T_DVB_ENCAP_BURST *) ip_buf)->pkt_type);
			}
		
			if(this->receptionStd->onRcvFrame(ip_buf, i_len, hdr->msg_type,
			                                  this->macId, &burst) < 0)
			{
				UTI_ERROR("failed to handle the reception of "
				          "DVB frame (length = %ld)\n", i_len);
				goto error;
			}
			if(this->SendNewMsgToUpperLayer(burst) < 0)
			{
				UTI_ERROR("failed to send burst to upper layer\n");
				goto error;
			}

			// update statistics
			this->m_statCounters.dlOutgoingCells += nb_cells;
		}
		break;

		// Start of frame (SOF):
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_SOF:
			const char *state_descr;

			if(this->_state == state_running)
				state_descr = "state_running";
			else if(this->_state == state_initializing)
				state_descr = "state_initializing";
			else
				state_descr = "other";

			UTI_DEBUG("SF#%ld: received SOF in state %s\n",
			          this->super_frame_counter, state_descr);

			if(this->_state == state_running)
			{
				if(this->onStartOfFrame(ip_buf, i_len) < 0)
				{
					goto error;
				}
			}
			else
				g_memory_pool_dvb_rcs.release((char *) ip_buf);
			break;

		// TBTP:
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_TBTP:
			if(this->_state == state_running)
			{
				if(this->m_pDamaAgent->hereIsTBTP(ip_buf, i_len) < 0)
				{
					g_memory_pool_dvb_rcs.release((char *) ip_buf);
					goto error_on_TBTP;
				}
			}
			g_memory_pool_dvb_rcs.release((char *) ip_buf);
			break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(this->onRcvLogonResp(ip_buf, i_len) < 0)
			{
				goto error;
			}
			break;

		// messages sent by current or another ST for the NCC --> ignore
		case MSG_TYPE_CR:
		case MSG_TYPE_SESSION_LOGON_REQ:
			g_memory_pool_dvb_rcs.release((char *) ip_buf);
			break;

		default:
			UTI_DEBUG_L3("SF#%ld: unknown type of DVB frame (%ld), ignore\n",
			             this->super_frame_counter, hdr->msg_type);
			g_memory_pool_dvb_rcs.release((char *) ip_buf);
			goto error;
	}

	return 0;

error_on_TBTP:
	fprintf(stderr, "TBTP Treatments failed at SF# %ld, frame %ld: see log "
	        "file\n", this->super_frame_counter, this->frame_counter);
	return -1;

error:
	fprintf(stderr, "Treatments failed at SF# %ld, frame %ld: see log "
	        "file\n", this->super_frame_counter, this->frame_counter);
	return -1;
}

/**
 * Send a capacity request for NRT data
 * @return -1 if failled, 0 if succeed
 */
int BlocDVBRcsTal::sendCR()
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[sendCR]";
	unsigned char *dvb_frame;
	int ret;
	long dvb_size;

	// Get a dvb frame
	dvb_frame = (unsigned char *) g_memory_pool_dvb_rcs.get(HERE());
	if(dvb_frame == 0)
	{
		UTI_ERROR("%s SF#%ld frame %ld: cannot get memory from dvb_rcs "
		          "memory pool\n", FUNCNAME, this->super_frame_counter,
		          this->frame_counter);
		goto error;
	}

	// Set CR body
	ret = this->m_pDamaAgent->buildCR(this->dvb_fifos,
	                                  this->dvb_fifos_number,
	                                  dvb_frame,
	                                  MSG_DVB_RCS_SIZE_MAX);

	// ignore if no CR built
	if(ret != 0)
	{
		g_memory_pool_dvb_rcs.release((char *) dvb_frame);
		UTI_DEBUG_L3("%s SF#%ld frame %ld: DAMA cannot build CR\n", FUNCNAME,
		             this->super_frame_counter, this->frame_counter);
	}
	else // else Send CR
	{
		dvb_size = ((T_DVB_SAC_CR *) dvb_frame)->hdr.msg_length; // real size now

		// Send message
		if(!this->sendDvbFrame((T_DVB_HDR *) dvb_frame, m_carrierIdDvbCtrl))
		{
			UTI_ERROR("%s SF#%ld frame %ld: failed to allocate mgl msg\n",
			          FUNCNAME, this->super_frame_counter, this->frame_counter);
			g_memory_pool_dvb_rcs.release((char *) dvb_frame);
			goto error;
		}

		UTI_DEBUG("%s SF#%ld frame %ld: CR sent\n", FUNCNAME,
		          this->super_frame_counter, this->frame_counter);
	}

	return 0;

error:
	return -1;
}


/**
 * Upon reception of a SoF:
 * - update allocation with TBTP received last superframe (in DAMA agent)
 * - reset timers
 * @param ip_buf points to the dvb_rcs buffer containing SoF
 * @param i_len is the length of *ip_buf
 * @return 0 on success, -1 on failure
 */
int BlocDVBRcsTal::onStartOfFrame(unsigned char *ip_buf, long i_len)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onStartOfFrame]";
	long sfn; // the superframe number piggybacked by SOF packet

	// store the superframe number;
	sfn = ((T_DVB_SOF *) ip_buf)->frame_nr;

	UTI_DEBUG_L3("%s sof reception SFN #%ld super frame nb %ld frame "
	             "counter %ld\n", FUNCNAME, sfn, this->super_frame_counter,
	             this->frame_counter);
	UTI_DEBUG("%s superframe number: %ld", FUNCNAME, sfn);

	// if the NCC crashed, we must reinitiate a logon
	if(sfn < this->super_frame_counter)
	{
		UTI_ERROR("SF#%ld: it seems NCC rebooted => flush buffer & "
		          "resend a logon request\n",
		          this->super_frame_counter);

		if(this->dvb_fifos != NULL)
		{
			deletePackets();
		}
		if(sendLogonReq() < 0)
		{
			goto error;
		}

		this->_state = state_wait_logon_resp;
		this->super_frame_counter = sfn;
		this->frame_counter = -1;
		goto error;
	}

	// as long as the frame is changing, send all probes and event
	ENV_AGENT_Send(&EnvAgent);

	// update the frame numerotation
	this->super_frame_counter = sfn;

	// sync environment plane
	ENV_AGENT_Sync(&EnvAgent, this->super_frame_counter, 0);

	// Inform dama agent
	if(m_pDamaAgent->hereIsSOF(ip_buf, i_len) < 0)
		goto error;

	// There is a risk of unprecise timing so the following hack

	// ---- if we have consumed all frames of previous sf ----
	// ---- (or if it is the first frame)                 ----
	if(this->frame_counter == this->frames_per_superframe ||
	   this->frame_counter == -1)
	{
		UTI_DEBUG("%s SF#%ld frame %ld: all frames from previous SF are "
		          "consumed or it is the first frame\n", FUNCNAME,
		          this->super_frame_counter, this->frame_counter);

		// reset frame counter: it will be init to 1 (1st frame number)
		// at the beginning of processOnFrameTick()
		this->frame_counter = 0;

		// we have consumed all of our frames, we start a new one immediately
 		// this is the first frame of the new superframe
		if(this->processOnFrameTick() < 0)
		{
			// exit because the bloc is unable to continue
			UTI_ERROR("%s treatments at sf %ld, frame %ld failed: "
			          "see log file\n", FUNCNAME, this->super_frame_counter,
			          this->frame_counter);
			fprintf(stderr, "\n%s treatments at sf %ld, frame %ld failed: "
			        "see log file \n\n", FUNCNAME, this->super_frame_counter,
			        this->frame_counter);
			goto error;
		}
	}
	else
	{
		// ----  if we have not consumed all our frames (it is the risk) ----
		// else : frame_counter < frames_per_superframe
		// if we have not consumed all our frames (it is the risk)
		// Then there is, by design, a timer active, we have to leave it
		// as we cannot remove it
		// hence we do only a reassignation of frame_counter (the frame active
		// count now as one frame in our superframe)
		this->frame_counter = 0;
	}

	g_memory_pool_dvb_rcs.release((char *) ip_buf);
	return 0;

error:
	g_memory_pool_dvb_rcs.release((char *) ip_buf);
	return -1;
}


/**
 * When receive a frame tick, send a constant DVB burst size for RT traffic,
 * and a DVB burst for NRT allocated by the DAMA agent
 * @return 0 on success, -1 if failed
 */
int BlocDVBRcsTal::processOnFrameTick()
{
	int ret = 0;
	int globalFrameNumber;

	// update frame counter for current SF - 1st frame within SF is 1 -
	this->frame_counter++;
	UTI_DEBUG("SF#%ld: frame %ld: start processOnFrameTick\n",
	          this->super_frame_counter, this->frame_counter);

	// ------------ arm timer for next frame -----------
	// this is done at the beginning in order not to increase next frame
	// by current frame treatments delay
	if(this->frame_counter < this->frames_per_superframe)
	{
		this->setTimer(this->m_frameTimer,
		               DVB_TIMER_ADJUST(this->frame_duration));
	}

	// ---------- tell the DAMA agent that a new frame begins ----------
	// Inform dama agent, and update total Available Allocation
	// for current frame
	m_totalAvailAlloc = this->m_pDamaAgent->processOnFrameTick();

	// ---------- schedule and send data frames ---------
	// schedule packets extracted from DVB FIFOs according to
	// the algorithm defined in DAMA agent
	ret = this->m_pDamaAgent->globalSchedule(this->dvb_fifos,
	                                         this->dvb_fifos_number,
	                                         m_totalAvailAlloc,
	                                         this->out_encap_packet_type,
	                                         &this->complete_dvb_frames);
	if(ret != 0)
	{
		UTI_ERROR("failed to schedule packets from DVB FIFOs\n");
		goto error;
	}

	// send on the emulated DVB network the DVB frames that contain
	// the encapsulation packets scheduled by the DAMA agent algorithm
	ret = this->sendBursts(&this->complete_dvb_frames, this->m_carrierIdData);
	if(ret != 0)
	{
		UTI_ERROR("failed to send bursts in DVB frames\n");
		goto error;
	}

	// ---------- Capacity Request ----------
	// compute and send Capacity Request ... only if
	// the OBR period has been reached
	globalFrameNumber =
		(this->super_frame_counter - 1) * this->frames_per_superframe
		+ this->frame_counter;
	if((globalFrameNumber % m_obrPeriod) == m_obrSlotFrame)
	{
		if(this->sendCR() < 0)
		{
			UTI_ERROR("failed to send Capacity Request\n");
			goto error;
		}
	}

	// ---------- Statistics ---------
	// trace statistics for current frame
	this->updateStatsOnFrame();
	this->updateStatsOnFrameAndEncap();

	return ret;

error:
	return -1;
}

/**
 * Manage logon response: inform dama and upper layer that the link is now up and running
 * @param ip_buf the pointer to the longon response message
 * @param l_len the lenght of the message
 * @return 0 on success, -1 on failure
 */
int BlocDVBRcsTal::onRcvLogonResp(unsigned char *ip_buf, long l_len)
{
	const char *FUNCNAME = DVB_DBG_PREFIX "[onRcvLogonResp]";
	mgl_msg *lp_msg;
	T_LINK_UP *link_is_up;
	T_DVB_LOGON_RESP *lp_logon_resp;
	std::string name = __FUNCTION__;

	g_memory_pool_dvb_rcs.add_function(name, (char *) ip_buf);
	// Retrieve the Logon Response frame
	lp_logon_resp = (T_DVB_LOGON_RESP *) ip_buf;
	if(lp_logon_resp->mac != macId)
	{
		UTI_DEBUG("%s SF#%ld Loggon_resp for mac=%d, not %d\n", FUNCNAME,
		          this->super_frame_counter, lp_logon_resp->mac, macId);
		goto ok;
	}

	// Remember the id
	m_groupId = lp_logon_resp->group_id;
	m_talId = lp_logon_resp->logon_id;

	// Inform Dama agent
	m_pDamaAgent->hereIsLogonResp(ip_buf, l_len);

	// Send a link is up message to upper layer
	// link_is_up
	link_is_up = new T_LINK_UP;
	if(link_is_up == 0)
	{
		UTI_ERROR("%s SF#%ld Memory allocation error on link_is_up.\n",
		          FUNCNAME, this->super_frame_counter);
		goto error;
	}
	link_is_up->group_id = m_groupId;
	link_is_up->tal_id = m_talId;

	// mgl msg
	lp_msg = this->newMsgWithBodyPtr(msg_link_up, link_is_up, sizeof(T_LINK_UP));
	if(lp_msg == 0)
	{
		UTI_ERROR("%s SF#%ld Failed to allocate a mgl msg.\n", FUNCNAME,
		          this->super_frame_counter);
		delete link_is_up;
		goto error;
	}
	this->sendMsgTo(this->getUpperLayer(), lp_msg);
	UTI_DEBUG_L3("%s SF#%ld Link is up msg sent to upper layer\n", FUNCNAME,
	             this->super_frame_counter);

	// Set the state to "running"
	this->_state = state_running;
	UTI_INFO("SF#%ld: logon succeeded, running as group %ld and logon %ld\n",
	         this->super_frame_counter, this->m_groupId, this->m_talId);

	// send the corresponding event
	ENV_AGENT_Event_Put(&EnvAgent, C_EVENT_SIMU, macId, 0,
	                    C_EVENT_LOGIN_COMPLETE);

	// set the terminal ID in emission and reception standards
	this->receptionStd->setTalId(m_talId);
	this->emissionStd->setTalId(m_talId);

	// set the terminal ID in emission and reception standards
	this->receptionStd->setTalId(m_talId);
	this->emissionStd->setTalId(m_talId);

 ok:
	g_memory_pool_dvb_rcs.release((char *) ip_buf);
	return 0;
 error:
	g_memory_pool_dvb_rcs.release((char *) ip_buf);
	return -1;
}


/**
 * Update statistics :
 *  - UL Incoming Throughput
 *  - UL Outgoing Throughput
 *  - DL Outgoing Throughput
 * These statistics will be updated when the DVB bloc receive a frame tick
 * because they depend on frame duration
 */
void BlocDVBRcsTal::updateStatsOnFrame()
{
	MacFifoStatContext macQStat;
	DAStatContext *damaStat;
	int fifoIndex;
	int ulOutgoingCells = 0;

	// DAMA agent stat
	damaStat = m_pDamaAgent->getStatsCxt();

	// MAC fifos stats
	for(fifoIndex = 0; fifoIndex < this->dvb_fifos_number; fifoIndex++)
	{
		this->dvb_fifos[fifoIndex].getStatsCxt(macQStat);

		// NB: mac queueing delay = macQStat.lastPkQueuingDelay
		// is writing at each UL cell emission by MAC layer and DA

		// compute UL incoming Throughput - in kbits/s
		m_statContext.ulIncomingThroughput[fifoIndex] =
			(m_statCounters.ulIncomingCells[fifoIndex]
			* this->out_encap_packet_length * 8) / this->frame_duration;

		// compute UL outgoing Throughput
		// NB: outgoingCells = cells directly sent from IP packets + cells
		//     stored before extraction next frame
		ulOutgoingCells = m_statCounters.ulOutgoingCells[fifoIndex] +
		                  macQStat.outPkNb;
		m_statContext.ulOutgoingThroughput[fifoIndex] = (ulOutgoingCells
			* this->out_encap_packet_length * 8) / this->frame_duration;

		// write in statitics file
		ENV_AGENT_Probe_PutInt(&EnvAgent,
		                       C_PROBE_ST_REAL_INCOMING_THROUGHPUT,
		                       this->dvb_fifos[fifoIndex].getId() + 1,
		                       m_statContext.ulIncomingThroughput[fifoIndex]);
		ENV_AGENT_Probe_PutInt(&EnvAgent,
		                       C_PROBE_ST_REAL_OUTGOING_THROUGHPUT,
		                       this->dvb_fifos[fifoIndex].getId() + 1,
		                       m_statContext.ulOutgoingThroughput[fifoIndex]);
	}

	// outgoing DL throughput
	m_statContext.dlOutgoingThroughput =
		(m_statCounters.dlOutgoingCells * this->in_encap_packet_length * 8) /
		this->frame_duration;

	// write in statitics file
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_ST_PHYSICAL_OUTGOING_THROUGHPUT,
	                       0, m_statContext.dlOutgoingThroughput);

	// reset stat context for next frame
	resetStatsCxt();
}

/**
 * Update statistics :
 *  - RBDC Request
 *  - VBDC Request
 *  - CRA Allocation
 *  - Allocation Size
 *  - Unused Capacity
 *  - BBFrame Drop Rate
 *  - Real Modcod
 *  - Used Modcod
 *  - Terminal queue size
 * These statistics will be updated when the DVB bloc receive a frame tick
 * and an UL encapsulation packet
 *
 */
void BlocDVBRcsTal::updateStatsOnFrameAndEncap()
{
	MacFifoStatContext macQStat;
	DAStatContext *damaStat;
	int fifoIndex;
	if(m_bbframe_dropped != 0 ||  m_bbframe_received !=0)
	{
		m_bbframe_dropped_rate = ((float) m_bbframe_dropped) /
		                         ((float) (m_bbframe_dropped + m_bbframe_received));
		UTI_DEBUG("m_bbframe_dropped_rate : %f \n", m_bbframe_dropped_rate);
	}

	// DAMA agent stat
	damaStat = m_pDamaAgent->getStatsCxt();

	// write in statitics file
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_RBDC_REQUEST_SIZE,
	                       0, damaStat->rbdcRequest);
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_VBDC_REQUEST_SIZE,
	                       0, damaStat->vbdcRequest);
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_CRA, 0, damaStat->craAlloc);
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_ALLOCATION_SIZE, 0,
	                       damaStat->globalAlloc);
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_UNUSED_CAPACITY, 0,
	                       damaStat->unusedAlloc);
	ENV_AGENT_Probe_PutFloat(&EnvAgent, C_PROBE_ST_BBFRAME_DROPED_RATE, 0,
	                         m_bbframe_dropped_rate);
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_ST_REAL_MODCOD, 0, 
	                       this->receptionStd->getRealModcod());
	ENV_AGENT_Probe_PutInt(&EnvAgent,  C_PROBE_ST_USED_MODCOD, 0,
	                       this->receptionStd->getReceivedModcod());

	// MAC fifos stats
	for(fifoIndex = 0; fifoIndex < this->dvb_fifos_number; fifoIndex++)
	{
		this->dvb_fifos[fifoIndex].getStatsCxt(macQStat);

		// write in statitics file : mac queue size
		ENV_AGENT_Probe_PutInt(&EnvAgent,
		                       C_PROBE_ST_TERMINAL_QUEUE_SIZE,
		                       this->dvb_fifos[fifoIndex].getId() + 1,
		                       macQStat.currentPkNb);
	}
}


/**
 * Reset statistics context
 * @return: none
 */
void BlocDVBRcsTal::resetStatsCxt()
{
	int i;
	m_statCounters.dlOutgoingCells = 0;
	m_statContext.dlOutgoingThroughput = 0;
	for(i = 0; i < this->dvb_fifos_number; i++)
	{
		m_statContext.ulIncomingThroughput[i] = 0;
		m_statContext.ulOutgoingThroughput[i] = 0;
		m_statCounters.ulIncomingCells[i] = 0;
		m_statCounters.ulOutgoingCells[i] = 0;
	}
}


/**
 * Delete ATM cells, MPEG or GSE packets in dvb_fifo
 */
void BlocDVBRcsTal::deletePackets()
{
	int size;
	MacFifoElement *elem;
	for(int j = 0; j < this->dvb_fifos_number; j++)
	{
		size = this->dvb_fifos[j].getCount();
		for(int i = 0; i < size; i++)
		{
			elem = (MacFifoElement *) this->dvb_fifos[j].remove();
			delete elem;
		}
	}
}


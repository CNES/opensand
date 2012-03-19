/**
 * @file bloc_encap_sat.cpp
 * @brief Generic Encapsulation Bloc for SE
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "bloc_encap_sat.h"

// debug
#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


BlocEncapSat::BlocEncapSat(mgl_blocmgr * blocmgr,
                           mgl_id fatherid,
                           const char *name):
	mgl_bloc(blocmgr, fatherid, name)
{
	this->encapCtx = NULL;
	this->initOk = false;
}

BlocEncapSat::~BlocEncapSat()
{
	if(this->encapCtx != NULL)
		delete this->encapCtx;
}

mgl_status BlocEncapSat::onEvent(mgl_event *event)
{
	const char *FUNCNAME = "[BlocEncapSat::onEvent]";
	mgl_status status = mgl_ko;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->initOk)
		{
			UTI_ERROR("%s bloc has already been initialized, "
			          "ignore init event\n", FUNCNAME);
		}
		else if(this->onInit() == mgl_ok)
		{
			this->initOk = true;
			status = mgl_ok;
		}
		else
		{
			UTI_ERROR("%s bloc initialization failed\n", FUNCNAME);
		}
	}
	else if(!this->initOk)
	{
		UTI_ERROR("%s satellite encapsulation bloc not initialized, ignore "
		          "non-init event\n", FUNCNAME);
	}
	else if(MGL_EVENT_IS_TIMER(event))
	{
		// timer event, flush corresponding encapsulation context
		status = this->onTimer((mgl_timer) event->event.timer.id);
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		// message received from another bloc

		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer())
		{
			UTI_DEBUG("%s message received from the lower layer\n", FUNCNAME);

			if(MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
			{
				NetBurst *burst;
				burst = (NetBurst *) MGL_EVENT_MSG_GET_BODY(event);
				status = this->onRcvBurstFromDown(burst);
			}
			else
			{
				UTI_ERROR("%s message type is unknown\n", FUNCNAME);
			}
		}
		else
		{
			UTI_ERROR("%s message received from an unknown bloc\n", FUNCNAME);
		}
	}
	else
	{
		UTI_ERROR("%s unknown event (type %ld) received\n",
		          FUNCNAME, event->type);
	}

	return status;
}

mgl_status BlocEncapSat::setUpperLayer(mgl_id bloc_id)
{
	const char *FUNCNAME = "[BlocEncapSat::setUpperLayer]";

	UTI_ERROR("%s bloc does not accept an upper-layer bloc\n", FUNCNAME);

	return mgl_ko;
}

mgl_status BlocEncapSat::onInit()
{
	const char *FUNCNAME = "[BlocEncapSat::onInit]";
	int packing_threshold;
	int qos_nbr;

	// read encapsulation scheme to use to output data
	if(globalConfig.getStringValue(GLOBAL_SECTION, OUT_ENCAP_SCHEME,
	                               this->output_encap_proto) < 0)
	{
		UTI_INFO("%s Section %s, %s missing. Send encapsulation scheme set to "
		         "ATM over MPEG2-TS.\n", FUNCNAME, GLOBAL_SECTION,
		         OUT_ENCAP_SCHEME);
		this->output_encap_proto = ENCAP_MPEG_ATM_AAL5;
	}

	if(this->output_encap_proto == ENCAP_MPEG_ATM_AAL5 ||
	   this->output_encap_proto == ENCAP_MPEG_ULE ||
	   this->output_encap_proto == ENCAP_MPEG_ATM_AAL5_ROHC ||
	   this->output_encap_proto == ENCAP_MPEG_ULE_ROHC)
	{
		// read packing threshold from config
		if(globalConfig.getIntegerValue(GLOBAL_SECTION, PACK_THRES,
		                                packing_threshold) < 0)
		{
			UTI_INFO("%s Section %s, %s missing. Packing threshold for MPEG "
			         "encapsulation protocol set to %d ms.\n", FUNCNAME,
			         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
			packing_threshold = DFLT_PACK_THRES;
		}

		if(packing_threshold < 0)
		{
			UTI_INFO("%s Section %s, bad value for %s. Packing threshold for "
			         "MPEG encapsulation protocol set to %d ms.\n", FUNCNAME,
			         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
			packing_threshold = DFLT_PACK_THRES;
		}

		// create the encapsulation context
		if(this->output_encap_proto == ENCAP_MPEG_ATM_AAL5 ||
		   this->output_encap_proto == ENCAP_MPEG_ATM_AAL5_ROHC)
		{
			// the encapsulation context encapsulates ATM cells into
			// MPEG2-TS frames
			this->encapCtx = new MpegCtx(AtmCell::length(), packing_threshold,
			                             AtmCell::length, AtmCell::create);
		}
		else
		{
			// the encapsulation context encapsulates ULE packets into
			// MPEG2-TS frames
			this->encapCtx = new MpegCtx(2, packing_threshold,
			                             UlePacket::length, UlePacket::create);
		}

		// check encapsulation context validity
		if(this->encapCtx == NULL)
		{
			UTI_ERROR("%s cannot create MPEG encapsulation context\n", FUNCNAME);
			goto error;
		}

		UTI_INFO("%s packing threshold for MPEG encapsulation protocol = %d ms\n",
		         FUNCNAME, packing_threshold);
	}
	else if(this->output_encap_proto == ENCAP_GSE_ATM_AAL5 ||
	        this->output_encap_proto == ENCAP_GSE_MPEG_ULE ||
	        this->output_encap_proto == ENCAP_GSE ||
			this->output_encap_proto == ENCAP_GSE_ATM_AAL5_ROHC ||
	        this->output_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC ||
	        this->output_encap_proto == ENCAP_GSE_ROHC)
	{
		// Get QoS number for GSE encapsulation context
		if(globalConfig.getIntegerValue(GLOBAL_SECTION, GSE_QOS_NBR,
		                                qos_nbr) < 0)
		if(qos_nbr < 0)
		{
			UTI_INFO("%s Section %s missing. QoS number for GSE "
			         "encapsulation scheme set to %d.\n", FUNCNAME,
			         GSE_QOS_NBR, DFLT_GSE_QOS_NBR);
			qos_nbr = DFLT_GSE_QOS_NBR;
		}

		// read packing threshold from config
		if(globalConfig.getIntegerValue(GLOBAL_SECTION, PACK_THRES,
		                                packing_threshold) < 0)
		{
			UTI_INFO("%s Section %s, %s missing. Packing threshold for GSE "
			         "encapsulation protocol set to %d ms.\n", FUNCNAME,
			         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
			packing_threshold = DFLT_PACK_THRES;
		}

		if(packing_threshold < 0)
		{
			UTI_INFO("%s Section %s, bad value for %s. Packing threshold for "
			         "MPEG encapsulation protocol set to %d ms.\n", FUNCNAME,
			         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
			packing_threshold = DFLT_PACK_THRES;
		}

		// create the encapsulation context
		if(this->output_encap_proto == ENCAP_GSE_ATM_AAL5 ||
		   this->output_encap_proto == ENCAP_GSE_ATM_AAL5_ROHC)
		{
			// the encapsulation context encapsulates ATM cells into GSE packets
			this->encapCtx = new GseCtx(qos_nbr, packing_threshold,
			                            AtmCell::length());
		}
		else if(this->output_encap_proto == ENCAP_GSE_MPEG_ULE ||
		        this->output_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC)
		{
			// the encapsulation context encapsulates MPEG frames into GSE packets
			this->encapCtx = new GseCtx(qos_nbr, packing_threshold,
			                            MpegPacket::length());
		}
		else
		{
			this->encapCtx = new GseCtx(qos_nbr, packing_threshold);
		}

		// check encapsulation context validity
		if(this->encapCtx == NULL)
		{
			UTI_ERROR("%s cannot create GSE encapsulation context\n", FUNCNAME);
			goto error;
		}

		UTI_INFO("%s QoS number for GSE encapsulation protocol = %d\n",
		         FUNCNAME, qos_nbr);
		UTI_INFO("%s packing threshold for GSE encapsulation protocol = %d ms\n",
		         FUNCNAME, packing_threshold);
	}
	else
	{
		UTI_INFO("%s bad value for input encapsulation scheme. "
		         "ATM over MPEG2-TS used instead\n", FUNCNAME);
		this->encapCtx = new MpegCtx(AtmCell::length(), packing_threshold,
		                             AtmCell::length, AtmCell::create);
	}

	return mgl_ok;

error:
	return mgl_ko;
}

mgl_status BlocEncapSat::onTimer(mgl_timer timer)
{
	const char *FUNCNAME = "[BlocEncapSat::onTimer]";
	std::map < mgl_timer, int >::iterator it;
	int id;
	NetBurst *burst;
	mgl_msg *msg; // margouilla message

	UTI_DEBUG("%s emission timer received, flush corresponding emission "
	          "context\n", FUNCNAME);

	// find encapsulation context to flush
	it = this->timers.find(timer);
	if(it == this->timers.end())
	{
		UTI_ERROR("%s timer not found\n", FUNCNAME);
		goto error;
	}

	// context found
	id = (*it).second;
	UTI_DEBUG("%s corresponding emission context found (ID = %d)\n",
	          FUNCNAME, id);

	// remove emission timer from the list
	this->timers.erase(it);

	// flush encapsulation context
	burst = this->encapCtx->flush(id);
	if(burst == NULL)
	{
		UTI_DEBUG("%s flushing context %d failed\n", FUNCNAME, id);
		goto error;
	}

	UTI_DEBUG("%s %d encapsulation packet(s) flushed\n",
	          FUNCNAME, burst->size());

	if(burst->size() <= 0)
		goto clean;

	// create the Margouilla message
	// with encapsulation burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, burst, sizeof(burst));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s encapsulation burst sent to the lower layer\n", FUNCNAME);

	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}

mgl_status BlocEncapSat::onRcvBurstFromDown(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncapSat::onRcvBurstFromDown]";
	mgl_status status;

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s message contains a burst of %d %s packet(s)\n",
	          FUNCNAME, burst->length(), burst->name().c_str());

	switch(burst->type())
	{
		// choose action depending on packet type
		case(NET_PROTO_MPEG):
			if(this->output_encap_proto == ENCAP_GSE_MPEG_ULE ||
			   this->output_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC)
			{
				status = this->EncapsulatePackets(burst);
			}
			else
			{
				status = this->ForwardPackets(burst);
			}
			break;
		case(NET_PROTO_GSE):
			// forward MPEG2-TS/GSE packets without modification
			status = this->ForwardPackets(burst);
			break;

		case(NET_PROTO_ATM):
			// encapsulate ATM cells into MPEG2-TS packets, then
			// forward them
			status = this->EncapsulatePackets(burst);
			break;

		default:
			// type not handled by bloc, drop with warning
			UTI_ERROR("bloc does not handle burst of type 0x%04x, drop burst\n",
			          burst->type());
			goto error;
	}

	return status;

error:
	return mgl_ko;
}

mgl_status BlocEncapSat::ForwardPackets(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncapSat::ForwardPackets]";
	mgl_msg *msg; // margouilla message

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	if(this->output_encap_proto == ENCAP_MPEG_ULE ||
	   this->output_encap_proto == ENCAP_MPEG_ULE_ROHC)
	{
		// burst must contains MPEG2-TS packets
		if(burst->type() != NET_PROTO_MPEG)
		{
			UTI_ERROR("%s burst (type 0x%04x) is not a MPEG burst\n",
			          FUNCNAME, burst->type());
			goto clean;
		}
	}
	else if(this->output_encap_proto == ENCAP_GSE ||
	        this->output_encap_proto == ENCAP_GSE_ROHC)
	{
		// burst must contains GSE packets
		if(burst->type() != NET_PROTO_GSE)
		{
			UTI_ERROR("%s burst (type 0x%04x) is not a GSE burst\n",
			          FUNCNAME, burst->type());
			goto clean;
		}
	}
	else
	{
		UTI_DEBUG("%s Bad output encapsulation scheme %s with "
		          "MPEG or GSE  burst\n",
		          FUNCNAME, this->output_encap_proto.c_str());
		goto clean;
	}

	UTI_INFO("%s output encapsulation scheme = %s\n", FUNCNAME,
	         this->output_encap_proto.c_str());

	// create the Margouilla message with MPEG burst as data
	msg = this->newMsgWithBodyPtr(msg_encap_burst, burst, sizeof(burst));
	if(!msg)
	{
		UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
		goto clean;
	}

	// send the message to the lower layer
	if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
	{
		UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
		goto clean;
	}

	UTI_DEBUG("%s MPEG burst sent to the lower layer\n", FUNCNAME);

	// everthing is fine
	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}

mgl_status BlocEncapSat::EncapsulatePackets(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncapSat::EncapsulatePackets]";
	NetBurst::iterator pkt_it;
	NetBurst *packets;
	int context_id;
	long time = 0;
	mgl_msg *msg; // margouilla message

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	// burst must contains ATM cells
	if(burst->type() != NET_PROTO_ATM &&
	   burst->type() != NET_PROTO_MPEG)
	{
		UTI_ERROR("%s burst (type 0x%04x) is not an ATM or MPEG burst\n",
		          FUNCNAME, burst->type());
		goto clean;
	}
	if(this->output_encap_proto != ENCAP_MPEG_ATM_AAL5 &&
	   this->output_encap_proto != ENCAP_GSE_ATM_AAL5 &&
	   this->output_encap_proto != ENCAP_GSE_MPEG_ULE &&
	   this->output_encap_proto != ENCAP_MPEG_ATM_AAL5_ROHC &&
	   this->output_encap_proto != ENCAP_GSE_ATM_AAL5_ROHC &&
	   this->output_encap_proto != ENCAP_GSE_MPEG_ULE_ROHC)
	{
		UTI_ERROR("%s Bad output encapsulation scheme %s with burst\n",
		          FUNCNAME, this->output_encap_proto.c_str());
	}

	// for each ATM cell within the burst...
	for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
	{
		UTI_DEBUG("%s encapsulate one %s packet (%d bytes)\n", FUNCNAME,
		          (*pkt_it)->name().c_str(), (*pkt_it)->totalLength());
		(*pkt_it)->addTrace(HERE());

		// encapsulate packet
		packets = encapCtx->encapsulate(*pkt_it, context_id, time);

		// set encapsulate timer if needed
		if(time > 0)
		{
			std::map < mgl_timer, int >::iterator it;
			bool found = false;

			// check if there is already a timer armed for the context
			for(it = this->timers.begin(); !found && it != this->timers.end(); it++)
			    found = ((*it).second == context_id);

			// set a new timer if no timer was found
			if(!found)
			{
				mgl_timer timer;
				this->setTimer(timer, time);
				this->timers.insert(std::make_pair(timer, context_id));
				UTI_DEBUG("%s timer for context ID %d armed with %ld ms\n",
				          FUNCNAME, context_id, time);
			}
			else
			{
				UTI_DEBUG("%s timer already set for context ID %d\n",
				          FUNCNAME, context_id);
			}
		}

		// check burst validity
		if(packets == NULL)
		{
			UTI_ERROR("%s encapsulation failed\n", FUNCNAME);
			goto clean;
		}

		if(this->output_encap_proto == ENCAP_MPEG_ATM_AAL5 ||
		   this->output_encap_proto == ENCAP_MPEG_ATM_AAL5_ROHC)
		{
			UTI_DEBUG("%s 1 %s packet => %d MPEG packet(s)\n", FUNCNAME,
			          (*pkt_it)->name().c_str(), packets->size());
		}
		else if(this->output_encap_proto == ENCAP_GSE_ATM_AAL5 ||
		        this->output_encap_proto == ENCAP_GSE_MPEG_ULE ||
				this->output_encap_proto == ENCAP_GSE_ATM_AAL5_ROHC ||
		        this->output_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC)
		{
			UTI_DEBUG("%s 1 %s packet => %d GSE packet(s)\n", FUNCNAME,
			          (*pkt_it)->name().c_str(), packets->size());
		}

		// create and send message only if at least one MPEG/GSE packet was created
		if(packets->size() <= 0)
		{
			delete packets;
			continue;
		}

		// create the Margouilla message with MPEG or GSE burst as data
		msg = this->newMsgWithBodyPtr(msg_encap_burst, packets,
		                              sizeof(packets));
		if(!msg)
		{
			UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
			delete packets;
			continue;
		}

		// send the message to the lower layer
		if(this->sendMsgTo(this->getLowerLayer(), msg) == mgl_ko)
		{
			UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
			delete packets;
			continue;
		}

		if(this->output_encap_proto == ENCAP_MPEG_ATM_AAL5 ||
		   this->output_encap_proto == ENCAP_MPEG_ATM_AAL5_ROHC)
		{
			UTI_DEBUG("%s MPEG burst sent to the lower layer\n", FUNCNAME);
		}
		else if(this->output_encap_proto == ENCAP_GSE_ATM_AAL5 ||
		        this->output_encap_proto == ENCAP_GSE_MPEG_ULE ||
				this->output_encap_proto == ENCAP_GSE_ATM_AAL5_ROHC ||
		        this->output_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC)
		{
			UTI_DEBUG("%s GSE burst sent to the lower layer\n", FUNCNAME);
		}
	} // for each ATM cell within the burst

	// delete the burst of MPEG or GSE packets
	delete burst;

	// everthing is fine
	return mgl_ok;

clean:
	delete burst;
error:
	return mgl_ko;
}


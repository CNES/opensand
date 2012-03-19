/**
 * @file bloc_encap.cpp
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "bloc_encap.h"

// debug
#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


BlocEncap::BlocEncap(mgl_blocmgr * blocmgr, mgl_id fatherid, const char *name):
	mgl_bloc(blocmgr, fatherid, name)
{
	this->receptionCxt = NULL;
	this->emissionCxt = NULL;
	this->initOk = false;

	// group & TAL id
	this->_group_id = -1;
	this->_tal_id = -1;

	// link state
	this->_state = link_down;
}

BlocEncap::~BlocEncap()
{
	if(this->receptionCxt != NULL)
		delete this->receptionCxt;
	if(this->emissionCxt != NULL)
		delete this->emissionCxt;
}

mgl_status BlocEncap::onEvent(mgl_event *event)
{
	const char *FUNCNAME = "[BlocEncap::onEvent]";
	mgl_status status = mgl_ko;

	if(MGL_EVENT_IS_INIT(event))
	{
		// initialization event
		if(this->initOk)
		{
			UTI_ERROR("%s bloc has already been initialized, ignore init event\n",
			          FUNCNAME);
		}
		else if(this->onInit() == mgl_ok)
		{
			this->initOk = true;
			status = mgl_ok;
		}
	}
	else if(!this->initOk)
	{
		UTI_ERROR("%s encapsulation bloc not initialized, ignore "
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

		if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getUpperLayer())
		{
			UTI_DEBUG("%s message received from the upper-layer bloc\n", FUNCNAME);

			if(MGL_EVENT_MSG_IS_TYPE(event, msg_ip))
			{
				NetPacket *packet;
				packet = (NetPacket *) MGL_EVENT_MSG_GET_BODY(event);
				status = this->onRcvIpFromUp(packet);
			}
			else
			{
				UTI_ERROR("%s message type is unknown\n", FUNCNAME);
			}
		}
		else if(MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getLowerLayer())
		{
			UTI_DEBUG("%s message received from the lower layer\n", FUNCNAME);

			if(MGL_EVENT_MSG_IS_TYPE(event, msg_link_up))
			{
				mgl_msg *msg; // margouilla message
				T_LINK_UP *link_up_msg;

				// 'link up' message received => forward it to upper layer
				UTI_DEBUG("%s 'link up' message received, forward it\n", FUNCNAME);

				link_up_msg = (T_LINK_UP *) MGL_EVENT_MSG_GET_BODY(event);

				if(this->_state == link_up)
				{
					UTI_INFO("%s duplicate link up msg\n", FUNCNAME);
					delete link_up_msg;
					goto end_link_up;
				}

				// save group id and TAL id sent by MAC layer
				this->_group_id = link_up_msg->group_id;
				this->_tal_id = link_up_msg->tal_id;
				this->_state = link_up;

				// tell the reception context to filter packets against the TAL ID
				this->receptionCxt->setFilter(this->_tal_id);

				// create the Margouilla message
				msg = this->newMsgWithBodyPtr(msg_link_up, link_up_msg,
				                              sizeof(link_up_msg));
				if(!msg)
				{
					UTI_ERROR("%s cannot create 'link up' message\n", FUNCNAME);
					delete link_up_msg;
					goto end_link_up;
				}

				// send the message to the upper layer
				if(this->sendMsgTo(this->getUpperLayer(), msg) == mgl_ko)
				{
					UTI_ERROR("%s cannot forward 'link up' message\n", FUNCNAME);
					delete link_up_msg;
					goto end_link_up;
				}

				UTI_DEBUG("%s 'link up' message sent to the upper layer\n", FUNCNAME);
end_link_up:

				status = mgl_ok;
			}
			else if(MGL_EVENT_MSG_IS_TYPE(event, msg_encap_burst))
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
		UTI_ERROR("%s unknown event (type %ld) received\n", FUNCNAME, event->type);
	}

	return status;
}

mgl_status BlocEncap::onInit()
{
	const char *FUNCNAME = "[BlocEncap::onInit]";
	string output_encap_proto;
	string input_encap_proto;
	int packing_threshold;
	int qos_nbr;

	// read encapsulation scheme to use to output data
	if(globalConfig.getStringValue(GLOBAL_SECTION, OUT_ENCAP_SCHEME,
	                               output_encap_proto) < 0)
	{
		UTI_INFO("%s Section %s, %s missing. Send encapsulation scheme set to "
		         "ATM/AAL5.\n", FUNCNAME, GLOBAL_SECTION, OUT_ENCAP_SCHEME);
		output_encap_proto = ENCAP_ATM_AAL5;
	}

	// create the encapsulation context to use to output data
	if(output_encap_proto == ENCAP_ATM_AAL5)
		this->emissionCxt = (AtmCtx *) new AtmAal5Ctx();
	else if(output_encap_proto == ENCAP_MPEG_ULE)
	{
		if(globalConfig.getIntegerValue(GLOBAL_SECTION, PACK_THRES,
		                                packing_threshold) < 0)
		{
			UTI_INFO("%s Section %s, %s missing. Packing threshold for MPEG "
			         "encapsulation scheme set to %d ms.\n", FUNCNAME,
			         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
			packing_threshold = DFLT_PACK_THRES;
		}

		if(packing_threshold < 0)
		{
			UTI_INFO("%s Section %s, bad value for %s. Packing threshold for "
			         "MPEG encapsulation scheme set to %d ms.\n", FUNCNAME,
			         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
			packing_threshold = DFLT_PACK_THRES;
		}
		this->emissionCxt = (MpegCtx *) new MpegUleCtx(packing_threshold);

		UTI_INFO("%s packing threshold for MPEG encapsulation scheme = %d ms\n",
		         FUNCNAME, packing_threshold);
	}
	else if(output_encap_proto == ENCAP_GSE ||
	        output_encap_proto == ENCAP_GSE_ROHC)
	{
		// Get QoS number for GSE encapsulation context
		qos_nbr = globalConfig.getNbListItems(SECTION_CLASS);
		if(qos_nbr < 0)
		{
			UTI_INFO("%s Section %s missing. QoS number for GSE "
			         "encapsulation scheme set to %d.\n", FUNCNAME,
			         SECTION_CLASS, DFLT_GSE_QOS_NBR);
			qos_nbr = DFLT_GSE_QOS_NBR;
		}

		if(output_encap_proto == ENCAP_GSE_ROHC)
		{
			this->emissionCxt = (GseCtx *) new GseRohcCtx(qos_nbr);
		}
		else
		{
			this->emissionCxt = (GseCtx *) new GseCtx(qos_nbr);
		}

		UTI_INFO("%s QoS number for GSE encapsulation scheme = %d\n",
		         FUNCNAME, qos_nbr);
	}
	else if(output_encap_proto == ENCAP_ATM_AAL5_ROHC)
		this->emissionCxt = (AtmCtx *) new AtmAal5RohcCtx();
	else if(output_encap_proto == ENCAP_MPEG_ULE_ROHC)
		this->emissionCxt = (MpegCtx *) new MpegUleRohcCtx(0);
	else
	{
		UTI_INFO("%s bad value for output encapsulation scheme. ATM/AAL5 used "
		         "instead\n", FUNCNAME);
		this->emissionCxt = (AtmCtx *) new AtmAal5Ctx();
	}

	// check output encapsulation context validity
	if(this->emissionCxt == NULL)
	{
		UTI_ERROR("%s cannot create output encapsulation context %s\n",
		          FUNCNAME, output_encap_proto.c_str());
		goto error;
	}

	UTI_INFO("%s output encapsulation scheme = %s\n", FUNCNAME,
	         output_encap_proto.c_str());

#if ULE_SECURITY
	// ULE Extension Headers (if emission context is MPEG/ULE)
	if(output_encap_proto == ENCAP_MPEG_ULE)
	{
		UleExt *ext;

		// create Test SNDU ULE extension
		ext = new UleExtTest();
		if(ext == NULL)
		{
			UTI_ERROR("%s failed to create Test SNDU ULE extension\n", FUNCNAME);
			goto clean_emission;
		}
		// add Test SNDU ULE extension to the emission context
		// but do not enable it
		if(!((UleCtx *) this->emissionCxt)->addExt(ext, false))
		{
			UTI_ERROR("%s failed to add Test SNDU ULE extension\n", FUNCNAME);
			delete ext;
			goto clean_emission;
		}

		// create Security ULE extension
		ext = new UleExtSecurity();
		if(ext == NULL)
		{
			UTI_ERROR("%s failed to create Padding ULE extension\n", FUNCNAME);
			goto clean_emission;
		}

		// add Security ULE extension to the emission context
		// and enable it
		if(!((UleCtx *) this->emissionCxt)->addExt(ext, true))
		{
			UTI_ERROR("%s failed to add Padding ULE extension\n", FUNCNAME);
			delete ext;
			goto clean_emission;
		}
	}
#endif
	// read encapsulation scheme to use to receive data
	if(globalConfig.getStringValue(GLOBAL_SECTION, IN_ENCAP_SCHEME,
	                               input_encap_proto) < 0)
	{
		UTI_INFO("%s Section %s, %s missing. Receive encapsulation "
					"scheme set to ATM/AAL5.\n", FUNCNAME, GLOBAL_SECTION,
					IN_ENCAP_SCHEME);
		input_encap_proto = ENCAP_ATM_AAL5;
	}

	// create the encapsulation context to use to receive data
	if(input_encap_proto == ENCAP_ATM_AAL5)
		this->receptionCxt = (AtmCtx *) new AtmAal5Ctx();
	else if(input_encap_proto == ENCAP_MPEG_ULE)
		this->receptionCxt = (MpegCtx *) new MpegUleCtx(0);
	else if(input_encap_proto == ENCAP_GSE ||
	        input_encap_proto == ENCAP_GSE_ATM_AAL5 ||
	        input_encap_proto == ENCAP_GSE_MPEG_ULE ||
	        input_encap_proto == ENCAP_GSE_ROHC ||
	        input_encap_proto == ENCAP_GSE_ATM_AAL5_ROHC ||
	        input_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC)

	{
		// Get QoS number for GSE encapsulation context
		qos_nbr = globalConfig.getNbListItems(SECTION_CLASS);
		if(qos_nbr < 0)
		{
			UTI_INFO("%s Section %s missing. QoS number for GSE "
			         "deencapsulation scheme set to %d.\n", FUNCNAME,
			         SECTION_CLASS, DFLT_GSE_QOS_NBR);
			qos_nbr = DFLT_GSE_QOS_NBR;
		}

		if(input_encap_proto == ENCAP_GSE)
		{
			this->receptionCxt = (GseCtx *) new GseCtx(qos_nbr);
		}
		else if(input_encap_proto == ENCAP_GSE_ROHC)
		{
			this->receptionCxt = (GseCtx *) new GseRohcCtx(qos_nbr);
		}
		else
		{
			// Get packing threshold for GSE encapsulation context
			if(globalConfig.getIntegerValue(GLOBAL_SECTION, PACK_THRES,
			                                packing_threshold) < 0)
			{
				UTI_INFO("%s Section %s, %s missing. Packing threshold for MPEG "
				         "encapsulation scheme set to %d ms.\n", FUNCNAME,
				         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
				packing_threshold = DFLT_PACK_THRES;
			}

			if(packing_threshold < 0)
			{
				UTI_INFO("%s Section %s, bad value for %s. Packing threshold for "
				         "MPEG encapsulation scheme set to %d ms.\n", FUNCNAME,
				         GLOBAL_SECTION, PACK_THRES, DFLT_PACK_THRES);
				packing_threshold = DFLT_PACK_THRES;
			}

			if(input_encap_proto == ENCAP_GSE_ATM_AAL5)
			{
				this->receptionCxt =
					(GseCtx *) new GseAtmAal5Ctx(qos_nbr, packing_threshold);
			}
			else if(input_encap_proto == ENCAP_GSE_MPEG_ULE)
			{
				this->receptionCxt =
					(GseCtx *) new GseMpegUleCtx(qos_nbr, packing_threshold);
			}
			else if(input_encap_proto == ENCAP_GSE_ATM_AAL5_ROHC)
			{
				this->receptionCxt =
					(GseCtx *) new GseAtmAal5RohcCtx(qos_nbr, packing_threshold);
			}
			else if(input_encap_proto == ENCAP_GSE_MPEG_ULE_ROHC)
			{
				this->receptionCxt =
					(GseCtx *) new GseMpegUleRohcCtx(qos_nbr, packing_threshold);
			}
			else
			{
				UTI_ERROR("%s bad value for input encapsulation scheme (%s)",
				          FUNCNAME, input_encap_proto.c_str());
				goto clean_emission;
			}

			UTI_INFO("%s packing threshold for GSE encapsulation scheme = %d ms\n",
			         FUNCNAME, packing_threshold);
		}

		UTI_INFO("%s QoS number for GSE encapsulation scheme = %d\n",
		         FUNCNAME, qos_nbr);
	}
	else if(input_encap_proto == ENCAP_MPEG_ATM_AAL5)
		this->receptionCxt = (MpegCtx *) new MpegAtmAal5Ctx(0);
	else if(input_encap_proto == ENCAP_ATM_AAL5_ROHC)
		this->receptionCxt = (AtmCtx *) new AtmAal5RohcCtx();
	else if(input_encap_proto == ENCAP_MPEG_ULE_ROHC)
		this->receptionCxt = (MpegCtx *) new MpegUleRohcCtx(0);
	else if(input_encap_proto == ENCAP_MPEG_ATM_AAL5_ROHC)
		this->receptionCxt = (MpegCtx *) new MpegAtmAal5RohcCtx(0);
	else
	{
		UTI_ERROR("%s bad value for input encapsulation scheme (%s). "
		          "ATM/AAL5 used instead\n",
		          FUNCNAME, input_encap_proto.c_str());
		this->receptionCxt = (AtmCtx *) new AtmAal5Ctx();
	}

	// check input encapsulation context validity
	if(this->receptionCxt == NULL)
	{
		UTI_ERROR("%s cannot create input encapsulation context %s\n",
		          FUNCNAME, input_encap_proto.c_str());
		goto clean_emission;
	}

	UTI_INFO("%s input encapsulation scheme = %s\n", FUNCNAME,
	         input_encap_proto.c_str());

#if ULE_SECURITY
	// ULE Extension Headers (if reception context is MPEG/ULE)
	if(input_encap_proto == ENCAP_MPEG_ULE)
	{
		UleExt *ext;

		// create Test SNDU ULE extension
		ext = new UleExtTest();
		if(ext == NULL)
		{
			UTI_ERROR("%s failed to create Test SNDU ULE extension\n", FUNCNAME);
			goto clean_reception;
		}

		// add Test SNDU ULE extension to the emission context
		// but do not enable it
		if(!((UleCtx *) this->receptionCxt)->addExt(ext, false))
		{
			UTI_ERROR("%s failed to add Test SNDU ULE extension\n", FUNCNAME);
			delete ext;
			goto clean_reception;
		}

		// create Security ULE extension
		ext = new UleExtSecurity();
		if(ext == NULL)
		{
			UTI_ERROR("%s failed to create Padding ULE extension\n", FUNCNAME);
			goto clean_reception;
		}

		// add Security ULE extension to the emission context
		// and enable it
		if(!((UleCtx *) this->receptionCxt)->addExt(ext, true))
		{
			UTI_ERROR("%s failed to add Padding ULE extension\n", FUNCNAME);
			delete ext;
			goto clean_reception;
		}
	}
#endif
	return mgl_ok;
#if ULE_SECURITY
clean_reception:
	delete this->receptionCxt;
#endif
clean_emission:
	delete this->emissionCxt;
error:
	return mgl_ko;
}

mgl_status BlocEncap::onTimer(mgl_timer timer)
{
	const char *FUNCNAME = "[BlocEncap::onTimer]";
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
	burst = this->emissionCxt->flush(id);
	if(burst == NULL)
	{
		UTI_ERROR("%s flushing context %d failed\n", FUNCNAME, id);
		goto error;
	}

	UTI_DEBUG("%s %d encapsulation packets flushed\n", FUNCNAME, burst->size());

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

mgl_status BlocEncap::onRcvIpFromUp(NetPacket *packet)
{
	const char *FUNCNAME = "[BlocEncap::onRcvIpFromUp]";
	NetBurst *burst;
	int context_id;
	long time = 0;
	mgl_msg *msg; // margouilla message

	// check packet validity
	if(packet == NULL)
	{
		UTI_ERROR("%s packet is not valid\n", FUNCNAME);
		goto error;
	}

	packet->addTrace(HERE());

	// check packet type
	if(packet->type() != NET_PROTO_IPV4 && packet->type() != NET_PROTO_IPV6)
	{
		UTI_ERROR("%s packet (type 0x%04x) is not an IP packet\n",
		          FUNCNAME, packet->type());
		goto drop;
	}

	UTI_DEBUG("%s encapsulate one %s packet (%d bytes, QoS = %d)\n",
	          FUNCNAME, packet->name().c_str(), packet->totalLength(),
	          packet->qos());

	// encapsulate packet
	burst = emissionCxt->encapsulate(packet, context_id, time);

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
	if(burst == NULL)
	{
		UTI_ERROR("%s encapsulation failed\n", FUNCNAME);
		goto drop;
	}

	if(burst->size() > 0)
	{
		UTI_DEBUG("encapsulation packet of type %s (QoS = %d)\n",
		          burst->front()->name().c_str(), burst->front()->qos());
	}

	UTI_DEBUG("1 %s packet => %d encapsulation packet(s)\n",
	          packet->name().c_str(), burst->size());

	// if no encapsulation packet was created, avoid sending a message
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

	// delete IP packet
	delete packet;

	// everything is fine
	return mgl_ok;

clean:
	delete burst;
drop:
	delete packet;
error:
	return mgl_ko;
}

mgl_status BlocEncap::onRcvBurstFromDown(NetBurst *burst)
{
	const char *FUNCNAME = "[BlocEncap::onRcvBurstFromDown]";
	NetBurst::iterator pkt_it;
	NetBurst *ip_packets;
	NetBurst::iterator ip_pkt_it;
	mgl_msg *msg; // margouilla message

	// check burst validity
	if(burst == NULL)
	{
		UTI_ERROR("%s burst is not valid\n", FUNCNAME);
		goto error;
	}

	UTI_DEBUG("%s message contains a burst of %d %s packet(s)\n",
	          FUNCNAME, burst->length(), burst->name().c_str());

	// for each encapsulation packet within the burst...
	for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
	{
		UTI_DEBUG("%s desencapsulate one %s packet (%d bytes)\n", FUNCNAME,
		          (*pkt_it)->name().c_str(), (*pkt_it)->totalLength());
		(*pkt_it)->addTrace(HERE());

		// desencapsulate packet
		ip_packets = receptionCxt->desencapsulate(*pkt_it);
		if(ip_packets == NULL)
		{
			UTI_ERROR("%s %s desencapsulation failed\n",
			          FUNCNAME, receptionCxt->type().c_str());
			continue;
		}

		UTI_DEBUG("%s 1 %s packet => %d IP packet(s)\n", FUNCNAME,
		          (*pkt_it)->name().c_str(), ip_packets->size());

		// for every desencapsulated IP packet...
		for(ip_pkt_it = ip_packets->begin();
		    ip_pkt_it != ip_packets->end(); ip_pkt_it++)
		{
			if((*ip_pkt_it)->type() != NET_PROTO_IPV4 &&
			   (*ip_pkt_it)->type() != NET_PROTO_IPV6)
			{
				UTI_ERROR("%s cannot send non-IP packet (0x%04x) to the "
				          "upper-layer block\n", FUNCNAME, (*ip_pkt_it)->type());
				delete *ip_pkt_it;
				continue;
			}
			(*ip_pkt_it)->addTrace(HERE());

			// create the Margouilla message
			// with IP packet as data
			//
			msg = this->newMsgWithBodyPtr(msg_ip, *ip_pkt_it, sizeof(*ip_pkt_it));
			if(!msg)
			{
				UTI_ERROR("%s newMsgWithBodyPtr() failed\n", FUNCNAME);
				delete *ip_pkt_it;
				continue;
			}

			// send the message to the upper layer
			if(this->sendMsgTo(this->getUpperLayer(), msg) == mgl_ko)
			{
				UTI_ERROR("%s sendMsgTo() failed\n", FUNCNAME);
				delete *ip_pkt_it;
				continue;
			}

			UTI_DEBUG("%s IP packet sent to the upper layer\n", FUNCNAME);
		}

		// clear the burst of IP packets without deleting the IpPacket
		// objects it contain then delete the burst
		ip_packets->clear();
		delete ip_packets;

	} // for each encapsulation packet within the burst

	// delete the burst of encapsulation packets
	delete burst;

	// everthing is fine
	return mgl_ok;

error:
	return mgl_ko;
}


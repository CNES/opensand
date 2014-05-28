/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file SlottedAloha.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

/**
 * @class SlottedAloha
 * @brief The Slotted Aloha
*/

#include "SlottedAloha.h"

#include "TerminalCategorySaloha.h"

#include <opensand_conf/conf.h>

SlottedAloha::SlottedAloha():
	sf_per_saframe(),
	frame_duration_ms(),
	nb_replicas(0),
	is_parent_init(false),
	pkt_hdl(NULL),
	categories(),
	terminal_affectation(),
	default_category(NULL)
// TODO init vars
{
	// TODO name
	this->log_saloha = Output::registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
	this->log_init = Output::registerLog(LEVEL_WARNING, "Dvb.init");
}


// TODO use a pointer on categories as it will be shared between here and Dama
//      categories will be cleaned in BlockDvb
bool SlottedAloha::initParent(time_ms_t frame_duration_ms,
                              EncapPlugin::EncapPacketHandler *const pkt_hdl,
                              TerminalCategories<TerminalCategorySaloha> &categories,
// TODO used in Ncc at least, if only NCC move in it
                              TerminalMapping<TerminalCategorySaloha> terminal_affectation,
                              TerminalCategorySaloha *default_category)
// TODO categories in Tal for slots definition
{
	TerminalCategories<TerminalCategorySaloha>::const_iterator cat_iter;

	srand(time(NULL));
	this->frame_duration_ms = frame_duration_ms;
	this->pkt_hdl = pkt_hdl;
	this->categories = categories;
	// we keep terminal affectation and default category but these affectations
	// and the default category can concern non Slotted Aloha categories
	// so be careful when adding a new terminal
	this->terminal_affectation = terminal_affectation;
	this->default_category = default_category;
	if(!this->default_category)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "No default terminal affectation defined, "
		    "some terminals may not be able to log in\n");
	}

	for(cat_iter = this->categories.begin(); cat_iter != this->categories.end();
	    ++cat_iter)
	{
		TerminalCategorySaloha *cat = (*cat_iter).second;
		cat->setSlotsNumber(frame_duration_ms,
		                    this->pkt_hdl->getFixedLength());
	}

	if(!Conf::getValue(SALOHA_SECTION, SALOHA_FPF, this->sf_per_saframe))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    SALOHA_SECTION, SALOHA_FPF);
		return false;
	}
	this->is_parent_init = true;

	return true;
}

SlottedAloha::~SlottedAloha()
{
	TerminalCategories<TerminalCategorySaloha>::iterator it;

	for(it = this->categories.begin();
	    it != this->categories.end(); ++it)
	{
		delete (*it).second;
	}
	this->categories.clear();

	this->terminal_affectation.clear();

}

// TODO this is not the same ID representation in both following fct, do something else
saloha_id_t SlottedAloha::buildPacketId(SlottedAlohaPacketData *packet)
{
	ostringstream os;
	
	// need int cast else there is some problems
	os << (int)packet->getId() << ':' << (int)packet->getSeq() << ':'
	   << (int)packet->getPduNb() << ':' << (int)packet->getQos();
	return saloha_id_t(os.str());
}

void SlottedAloha::convertPacketId(saloha_id_t id, uint16_t ids[4])
{
	istringstream iss((char *)id.c_str());
	char c;
	
	iss >> ids[SALOHA_ID_ID] >> c >> ids[SALOHA_ID_SEQ] >> c
		>> ids[SALOHA_ID_PDU_NB] >> c >> ids[SALOHA_ID_QOS];
}

bool SlottedAloha::isSuperFrameTick(time_sf_t superframe_counter)
{
	if(!(superframe_counter % this->sf_per_saframe))
	{
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "Slotted Aloha tick: %u", superframe_counter);
	}
	return !(superframe_counter % this->sf_per_saframe);
}

/*const char *SlottedAloha::hexa(Data input)
{
	static const char* lut = "0123456789ABCDEF";
	size_t length;
	string output;
	
	length = input.length();
	output.reserve(2 * length);
	for(size_t cpt = 0; cpt < length; ++cpt)
	{
		const unsigned char c = input[cpt];
		
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output.c_str();
}*/

/*void SlottedAloha::debug(const char* title, SlottedAlohaPacketData* packet)
{
	sa_ostream_t os;
	uint16_t* replicas;
	int cpt;
	
	if (!SALOHA_DEBUG)
		return;
	replicas = packet->getReplicas();
	for(cpt = 0; cpt < packet->getNbReplicas(); cpt++)
	{
		if (cpt)
			os << ', ';
		os << (int)replicas[cpt];
	}
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL %s Pkt[#%d] = { id: '%s', TS: %d, timeout: %d, nb_ret.: %d, "
		"nb_replicas: %d, replicas: [%s] }",
		title,
		(int)packet,
		this->buildPacketId(packet).c_str(),
		packet->getTs(),
		packet->getTimeout(),
		packet->getNbRetransmissions(),
		packet->getNbReplicas(),
		os.str().c_str());
}
void SlottedAloha::debug(const char* title, SlottedAlohaPacketCtrl* packet)
{
	if (!SALOHA_DEBUG)
		return;
	LOG(this->log_saloha, LEVEL_ERROR,
	    "WKL %s Pkt[#%d] = { type: %d, data: '%s' }",
		title,
		(int)packet,
		packet->getCtrlType(),
		packet->getCtrlData().c_str());
}
*/

/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file BlockEncap.h
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BLOCK_ENCAP_H
#define BLOCK_ENCAP_H


#include "OpenSandFrames.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "StackPlugin.h"
#include "EncapPlugin.h"
#include "OpenSandCore.h"
#include "LanAdaptationPlugin.h"
#include "TerminalCategory.h"
#include "BlockDvb.h"



#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>

/**
 * @class BlockEncap
 * @brief Generic Encapsulation Bloc
 */
class BlockEncap: public Block
{
 private:

	/// Expiration timers for encapsulation contexts
	std::map<event_id_t, int> timers;

	/// it is the MAC layer group id received through msg_link_up
	long group_id;

	/// it is the MAC layer MAC id received through msg_link_up
	tal_id_t tal_id;

	/// State of the satellite link
	link_state_t state;
	
	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;

	/// the satellite type (regenerative o transparent)
	sat_type_t satellite_type;

	/// the emission contexts list from lower to upper context
	std::vector<EncapPlugin::EncapContext *> emission_ctx;

	/// the reception contexts list from upper to lower context
	std::vector<EncapPlugin::EncapContext *> reception_ctx;

	/// the reception contexts list from upper to lower context for SCPC mode
	std::vector<EncapPlugin::EncapContext *> reception_ctx_scpc;




 public:

	/**
	 * Build an encapsulation block
	 *
	 * @param name  The name of the block
	 * @param name  The mac id of the terminal
	 */
	BlockEncap(const string &name, tal_id_t mac_id);

	/**
	 * Destroy the encapsulation block
	 */
	~BlockEncap();

 protected:

	// Log and debug
	OutputLog *log_rcv_from_up;
	OutputLog *log_rcv_from_down;
	OutputLog *log_send_down;

	// Init output (log, debug, probes, stats)
	bool initOutput();

	// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

 private:

	/**
	 * Handle the timer event
	 *
	 * @param timer_id  The id of the timer to handle
	 * @return          Whether the timer event was successfully handled or not
	 */
	bool onTimer(event_id_t timer_id);

	/**
	 * Handle a burst received from the upper-layer block
	 *
	 * @param burst  The burst received from the upper-layer block
	 * @return        Whether the IP packet was successful handled or not
	 */
	bool onRcvBurstFromUp(NetBurst *burst);

	/**
	 * Handle a burst of encapsulation packets received from the lower-layer
	 * block
	 *
	 * @param burst  The burst received from the lower-layer block
	 * @return       Whether the burst was successful handled or not
	 */
	bool onRcvBurstFromDown(NetBurst *burst);
	
	/**
	 * @brief Checks if SCPC mode is activated and configured
	 *        (Available FIFOs and Carriers for SCPC)
	 *
	 * @return       Whether there are SCPC FIFOs and SCPC Carriers available or not
	 */

	bool checkIfScpc();
	
	/**
	 * 
	 * Get the Encapsulation context of the Up/Return or the Down/Forward link
	 *
	 * @param scheme_list   The name of encapsulation scheme list
	 * @param l_plugin      The LAN adaptation plugin
	 * @ctx                 The encapsulation context for return/up or forward/down links
	 * @link_type           The type of link: "return/up" or "forward/down"
	 * @scpc_scheme			Whether SCPC is used for the return link or not
	 * @return              Whether the Encapsulation context has been
	 *                      correctly obtained or not
	 */
	
	bool getEncapContext(const char *scheme_list,
	                     LanAdaptationPlugin *l_plugin,
	                     vector <EncapPlugin::EncapContext *> &ctx,
	                     const char *link_type, 
	                     bool scpc_scheme);
	 /**
	 * @brief init the band according to configuration
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   spot                 The spot containing the section
	 * @param   section              The section in configuration file
	 *                               (up/return or down/forward)
	 * @param   access_type          The access type value
	 * @param   duration_ms          The frame duration on this band
	 * @param   categories           OUT: The terminal categories
	 * @param   terminal_affectation OUT: The terminal affectation in categories
	 * @param   default_category     OUT: The default category if terminal is not
	 *                                  in terminal affectation
	 * @param   fmt_groups           OUT: The groups of FMT ids
	 * @return true on success, false otherwise
	 */
	template<class T>
	bool initBand(ConfigurationList spot,
	              string section,
	              access_type_t access_type,
	              time_ms_t duration_ms,
	              const FmtDefinitionTable *fmt_def,
	              TerminalCategories<T> &categories,
	              TerminalMapping<T> &terminal_affectation,
	              T **default_category,
	              fmt_groups_t &fmt_groups);
	
	/**
	 * @brief  Compute the bandplan.
	 *
	 * Compute available carrier frequency for each carriers group in each
	 * category, according to the current number of users in these groups.
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   available_bandplan_khz  available bandplan (in kHz).
	 * @param   roll_off                roll-off factor
	 * @param   duration_ms             The frame duration on this band
	 * @param   categories              pointer to category list.
	 *
	 * @return  true on success, false otherwise.
	 */
	template<class T>
	bool computeBandplan(freq_khz_t available_bandplan_khz,
	                     double roll_off,
	                     time_ms_t duration_ms,
	                     TerminalCategories<T> &categories);

	/**
	 * @brief Read configuration for link MODCOD definition/simulation files
	 *
	 * @param def       The section in configuration file for MODCOD definitions
	 *                  (up/return or down/forward)
	 * @param simu      The section in configuration file for MODCOD simulation
	 *                  (up/return or down/forward)
	 * @param fmt_simu  The FMT simulation attribute to initialize
	 * @return  true on success, false otherwise
	 */
	bool initModcodFiles(const char *def, const char *simu,
	                     FmtSimulation &fmt_simu);



};

// TODO no need to do everything, and factorize
template<class T>
bool BlockEncap::initBand(ConfigurationList spot,
                          string section,
                          access_type_t access_type,
                          time_ms_t duration_ms,
                          const FmtDefinitionTable *fmt_def,
                          TerminalCategories<T> &categories,
                          TerminalMapping<T> &terminal_affectation,
                          T **default_category,
                          fmt_groups_t &fmt_groups)
{
	freq_khz_t bandwidth_khz;
	double roll_off;
	freq_mhz_t bandwidth_mhz = 0;
	ConfigurationList conf_list;
	ConfigurationList aff_list;
	typename TerminalCategories<T>::iterator cat_iter;
	unsigned int carrier_id = 0;
	int i;
	string default_category_name;

	// Get the value of the bandwidth for return link
	if(!Conf::getValue(spot, BANDWIDTH,
	                   bandwidth_mhz))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    section.c_str(), BANDWIDTH);
		goto error;
	}
	bandwidth_khz = bandwidth_mhz * 1000;
	LOG(this->log_init, LEVEL_INFO,
	    "%s: bandwitdh is %u kHz\n", 
	    section.c_str(), bandwidth_khz);

	// Get the value of the roll off
	if(!Conf::getValue(Conf::section_map[section], 
		               ROLL_OFF, roll_off))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    section.c_str(), ROLL_OFF);
		goto error;
	}

	// get the FMT groups
	if(!Conf::getListItems(spot,
	                       FMT_GROUP_LIST,
	                       conf_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n",
		    section.c_str(), FMT_GROUP_LIST);
		goto error;
	}

	// create group list
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		unsigned int group_id;
		string fmt_id;
		FmtGroup *group;

		// Get group id name
		if(!Conf::getAttributeValue(iter, GROUP_ID, group_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in FMT "
			    "groups\n", section.c_str(), 
			    GROUP_ID);
			goto error;
		}

		// Get FMT IDs
		if(!Conf::getAttributeValue(iter, FMT_ID, fmt_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in FMT "
			    "groups\n", section.c_str(),
			    FMT_ID);
			goto error;
		}

		if(fmt_groups.find(group_id) != fmt_groups.end())
		{
			LOG(this->log_init, LEVEL_INFO,
			    "Section %s, FMT group %u already loaded\n",
			    section.c_str(),
			    group_id);
			continue;
		}
		group = new FmtGroup(group_id, fmt_id, fmt_def);
		fmt_groups[group_id] = group;
	}

	conf_list.clear();
	// get the carriers distribution
	if(!Conf::getListItems(spot, CARRIERS_DISTRI_LIST, conf_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, %s missing\n", 
		    section.c_str(),
		    CARRIERS_DISTRI_LIST);
		goto error;
	}

	i = 0;
	// create terminal categories according to channel distribution
	for(ConfigurationList::iterator iter = conf_list.begin();
	    iter != conf_list.end(); ++iter)
	{
		string name;
		string ratio;
		vector<unsigned int> ratios;
		rate_symps_t symbol_rate_symps;
		string group_id;
		vector<unsigned int> group_ids;
		string access;
		unsigned int vcm_id = 0;
		T *category;

		i++;

		// Get carriers' name
		if(!Conf::getAttributeValue(iter, CATEGORY, name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", 
			    section.c_str(),
			    CATEGORY, i);
			goto error;
		}

		// Get carriers' ratio
		if(!Conf::getAttributeValue(iter, RATIO, ratio))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", 
			    section.c_str(), RATIO, i);
			goto error;
		}
		// parse ratio if there is many values
		ratios = tempSplit(ratio);

		// Get carriers' symbol ratge
		if(!Conf::getAttributeValue(iter, SYMBOL_RATE, symbol_rate_symps))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", 
			    section.c_str(),
			    SYMBOL_RATE, i);
			goto error;
		}

		// Get carriers' FMT id
		if(!Conf::getAttributeValue(iter, FMT_GROUP, group_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", 
			    section.c_str(),
			    FMT_GROUP, i);
			goto error;
		}
		// parse group ids if there is many values
		group_ids = tempSplit(group_id);

		if(group_ids.size() != ratios.size())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "There should be as many ratio values as fmt groups values\n");
			goto error;
		}

		// Get carriers' access type
		if(!Conf::getAttributeValue(iter, ACCESS_TYPE, access))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in carriers "
			    "distribution table entry %u\n", 
			    section.c_str(),
			    ACCESS_TYPE, i);
			goto error;
		}
		// TODO for SCPC we should not use the same fmt_def
		//      when initializing band on NCC, at the moment we get
		//      an error, this should not be displayed in this case
		//      SCPC are only loaded for ratio computation
		if(access != "VCM" &&
		   (group_ids.size() > 1 || ratios.size() > 1))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Too many FMT groups or ratio for non-VCM access type\n");
			goto error;
		}
		if(access == "VCM" && this->satellite_type == REGENERATIVE)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Cannot use VCM carriers with regenerative satellite\n");
			goto error;
		}

		LOG(this->log_init, LEVEL_NOTICE,
		    "%s: new carriers: category=%s, Rs=%G, FMT group=%s, "
		    "ratio=%s, access type=%s\n", 
		    section.c_str(), name.c_str(),
		    symbol_rate_symps, group_id.c_str(), ratio.c_str(),
		    access.c_str());

		for(vector<unsigned int>::iterator it = group_ids.begin();
		    it != group_ids.end(); ++it)
		{
			fmt_groups_t::const_iterator group_it;
			group_it = fmt_groups.find(*it);
			if(group_it == fmt_groups.end())
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "Section %s, no entry for FMT group with ID %u\n",
				    section.c_str(), (*it));
				goto error;
			}
			if(group_ids.size() > 1 && (*group_it).second->getFmtIds().size() > 1)
			{
				LOG(this->log_init, LEVEL_ERROR,
				    "For each VCM carriers, the FMT group should only "
				    "contain one FMT id\n");
				goto error;
			}

			// create the category if it does not exist
			// we also create categories with wrong access type because:
			//  - we may have many access types in the category
			//  - we need to get all carriers for band computation
			cat_iter = categories.find(name);
			category = dynamic_cast<T *>((*cat_iter).second);
			if(cat_iter == categories.end())
			{
				category = new T(name, access_type);
				categories[name] = category;
			}
			category->addCarriersGroup(carrier_id, (*group_it).second,
			                           ratios[vcm_id],
			                           symbol_rate_symps,
			                           strToAccessType(access));
			vcm_id++;
			// do not increment carrier_id here
		}
		carrier_id++;
	}

	// Compute bandplan
	if(!this->computeBandplan(bandwidth_khz, roll_off, duration_ms, categories))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot compute band plan for %s\n", 
		    section.c_str());
		goto error;
	}

	// delete category with no carriers corresponding to the access type
	for(cat_iter = categories.begin(); cat_iter != categories.end(); ++cat_iter)
	{
		T *category = (*cat_iter).second;
		// getCarriersNumber returns the number of carriers with the desired
		// access type only
		if(!category->getCarriersNumber())
		{
			LOG(this->log_init, LEVEL_INFO,
			    "Skip category %s with no carriers with desired access type\n",
			    category->getLabel().c_str());
			categories.erase(cat_iter);
			delete category;
		}
	}

	if(categories.size() == 0)
	{
		// no more category here, this will be handled by caller
		return true;
	}

	// get the default terminal category
	if(!Conf::getValue(spot, DEFAULT_AFF,
	                   default_category_name))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section %s, missing %s parameter\n", 
		    section.c_str(),
		    DEFAULT_AFF);
		goto error;
	}

	// Look for associated category
	*default_category = NULL;
	cat_iter = categories.find(default_category_name);
	if(cat_iter != categories.end())
	{
		*default_category = (*cat_iter).second;
	}
	if(*default_category == NULL)
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "Section %s, could not find category %s, "
		    "no default category for access type %u\n",
		    section.c_str(),
		    default_category_name.c_str(), access_type);
	}
	else
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "ST default category: %s in %s\n",
		    (*default_category)->getLabel().c_str(), 
		    section.c_str());
	}

	// get the terminal affectations
	if(!Conf::getListItems(spot, TAL_AFF_LIST, aff_list))
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "Section %s, missing %s parameter\n", 
		    section.c_str(),
		    TAL_AFF_LIST);
		goto error;
	}

	i = 0;
	for(ConfigurationList::iterator iter = aff_list.begin();
	    iter != aff_list.end(); ++iter)
	{
		// To prevent compilator to issue warning about non initialised variable
		tal_id_t tal_id = -1;
		string name;
		T *category;

		i++;
		if(!Conf::getAttributeValue(iter, TAL_ID, tal_id))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in terminal "
			    "affection table entry %u\n", 
			    section.c_str(), TAL_ID, i);
			goto error;
		}
		if(!Conf::getAttributeValue(iter, CATEGORY, name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Section %s, problem retrieving %s in terminal "
			    "affection table entry %u\n", 
			    section.c_str(), CATEGORY, i);
			goto error;
		}

		// Look for the category
		category = NULL;
		cat_iter = categories.find(name);
		if(cat_iter != categories.end())
		{
			category = (*cat_iter).second;
		}
		if(category == NULL)
		{
			LOG(this->log_init, LEVEL_NOTICE,
			    "Could not find category %s for terminal %u affectation, "
			    "it is maybe concerned by another access type",
			    name.c_str(), tal_id);
			// keep the NULL affectation for this terminal to avoid
			// setting default category
			terminal_affectation[tal_id] = NULL;
		}
		else
		{
			terminal_affectation[tal_id] = category;
			LOG(this->log_init, LEVEL_INFO,
			    "%s: terminal %u will be affected to category %s\n",
			    section.c_str(), tal_id, name.c_str());
		}
	}

	return true;

error:
	return false;
}

template<class T>
bool BlockEncap::computeBandplan(freq_khz_t available_bandplan_khz,
                                 double roll_off,
                                 time_ms_t duration_ms,
                                 TerminalCategories<T> &categories)
{
	typename TerminalCategories<T>::const_iterator category_it;

	double weighted_sum_ksymps = 0.0;

	// compute weighted sum
	for(category_it = categories.begin();
	    category_it != categories.end();
	    ++category_it)
	{
		T *category = (*category_it).second;

		// Compute weighted sum in ks/s since available bandplan is in kHz.
		weighted_sum_ksymps += category->getWeightedSum();
	}

	LOG(this->log_init, LEVEL_DEBUG,
	    "Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if(equals(weighted_sum_ksymps, 0.0))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Weighted ratio sum is 0\n");
		goto error;
	}

	// compute carrier number per category
	for(category_it = categories.begin();
	    category_it != categories.end();
		category_it++)
	{
		unsigned int carriers_number = 0;
		T *category = (*category_it).second;
		unsigned int ratio = category->getRatio();

		carriers_number = floor(
		    (ratio / weighted_sum_ksymps) *
		    (available_bandplan_khz / (1 + roll_off)));
		// create at least one carrier
		if(carriers_number == 0)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Band is too small for one carrier. "
			    "Increase band for one carrier\n");
			carriers_number = 1;
		}
		LOG(this->log_init, LEVEL_NOTICE,
		    "Number of carriers for category %s: %d\n",
		    category->getLabel().c_str(), carriers_number);

		// set the carrier numbers and capacity in carrier groups
		category->updateCarriersGroups(carriers_number,
		                               duration_ms);
	}

	return true;
error:
	return false;
}

#endif

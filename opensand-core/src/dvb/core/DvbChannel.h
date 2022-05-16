/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file DvbChannel.h
 * @brief A high level channel that implements some functions
 *        used by ST, SAT and/or GW
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 *
 */

#ifndef DVB_CHANNEL_H
#define DVB_CHANNEL_H

#include <sstream>

#include "PhysicStd.h"
#include "TerminalCategory.h"
#include "BBFrame.h"
#include "Sac.h"
#include "Ttp.h"
#include "StFmtSimu.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>


/**
 * @brief A high level channel that implements some functions
 *        used by ST, SAT and/or GW
 */
class DvbChannel
{
 public:
	DvbChannel():
		req_burst_length(0),
		super_frame_counter(0),
		fwd_down_frame_duration_ms(),
		ret_up_frame_duration_ms(),
		pkt_hdl(NULL),
		stats_period_ms(),
		stats_period_frame(),
		log_init_channel(NULL),
		log_receive_channel(NULL),
		log_send_channel(NULL),
		check_send_stats(0)
	{
		// register static log
		auto output = Output::Get();
		dvb_fifo_log = output->registerLog(LEVEL_WARNING, "Dvb.FIFO");
		this->log_init_channel = output->registerLog(LEVEL_WARNING, "Dvb.Channel.init");
		this->log_receive_channel = output->registerLog(LEVEL_WARNING, "Dvb.Channel.receive");
		this->log_send_channel = output->registerLog(LEVEL_WARNING, "Dvb.Channel.send");
	};

	virtual ~DvbChannel()
	{
	};

 protected:

	/**
	 * @brief Read MODCOD Definition types
	 *
	 * @return true if success, false otherwise
	 */
	bool initModcodDefinitionTypes(void);

	/**
	 * @brief Read the encapsulation shcemes to get packet handler
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @param pkt_hdl       The packet handler corresponding to the encapsulation scheme
	 * @return true on success, false otherwise
	 */
	bool initPktHdl(EncapSchemeList encap_schemes,
	                EncapPlugin::EncapPacketHandler **pkt_hdl);

	/**
	 * @brief Read the encapsulation shcemes to get packet handler
	 *
	 * @param pkt_hdl          The packet handler corresponding to the encapsulation name
	 * @return true on success, false otherwise
	 */
	bool initScpcPktHdl(EncapPlugin::EncapPacketHandler **pkt_hdl);

	/**
	 * @brief Read the common configuration parameters
	 *
	 * @param encap_schemes The section in configuration file for encapsulation
	 *                      schemes (up/return or down/forward)
	 * @return true on success, false otherwise
	 */
	bool initCommon(EncapSchemeList encap_schemes);

	/**
	 * @brief Init the timer for statistics
	 *
	 * @param frame_duration_ms  The frame duration that will be used to
	 *                           adujst the timer
	 */
	void initStatsTimer(time_ms_t frame_duration_ms);


	/**
	 * @brief init the band according to configuration
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   spot                 The spot containing the section
	 * @param   section              The section in configuration file
	 *                               (up/return or down/forward)
	 * @param   access_type          The access type value
	 * @param   duration_ms          The frame duration on this band
	 * @param   fmt_def              The MODCOD definition table
	 * @param   categories           OUT: The terminal categories
	 * @param   terminal_affectation OUT: The terminal affectation in categories
	 * @param   default_category     OUT: The default category if terminal is not
	 *                                    in terminal affectation
	 * @param   fmt_groups           OUT: The groups of FMT ids
	 * @return true on success, false otherwise
	 */
	template<class T>
	bool initBand(const OpenSandModelConf::spot &spot,
	              string section,
	              AccessType access_type,
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
	 * Receive Push data in FIFO
	 *
	 * @param fifo          The FIFO to put data in
	 * @param data          The data
	 * @param fifo_delay    The minimum delay the data must stay in the
	 *                      FIFO (used on SAT to emulate delay)
	 * @return              true on success, false otherwise
	 */
	bool pushInFifo(DvbFifo *fifo,
	                NetContainer *data,
	                time_ms_t fifo_delay);

	/**
	 * @brief Whether it is time to send statistics or not
	 *
	 * @return true if statistics shoud be sent, false otherwise
	 */
	bool doSendStats(void);


 	/**
	 * @brief   allocate more band to the demanding category
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   duration_ms          The frame duration on this band
	 * @param   cat_label            The label of the category
	 * @param   new_rate_kbps        The new rate for the category
	 * @param   categories           OUT: The terminal categories
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool allocateBand(time_ms_t duration_ms,
	                  string cat_label,
	                  rate_kbps_t new_rate_kbps,
	                  TerminalCategories<T> &categories);

	/**
	 * @brief   release band of the demanding category
	 *
	 * @tparam  T The type of terminal category to create
	 * @param   duration_ms          The frame duration on this band
	 * @param   cat_label            The label of the category
	 * @param   new_rate_kbps        The new rate for the category
	 * @param   categories           OUT: The terminal categories
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool releaseBand(time_ms_t duration_ms,
	                 string cat_label,
	                 rate_kbps_t new_rate_kbps,
	                 TerminalCategories<T> &categories);

	/**
	 * @brief   Calculation of the carriers needed to be transfer from cat1 to cat2
	 *          in order to have a rate of new_rate_kbps on cat2
	 * @tparam  T The type of terminal category
	 * @param   duration_ms   The frame duration on this band
	 * @param   cat           The category with to much carriers
	 * @param   rate_symps    The rate to be transfer (OUT: the surplus)
	 * @param   carriers      OUT: The informations about the carriers to be transfer
	 * @return  true on success, false otherwise
	 */
	template<class T>
	bool carriersTransferCalculation(T* cat, rate_symps_t &rate_symps,
	                                  map<rate_symps_t, unsigned int> &carriers);

/**
 * @brief   Transfer of the carrier
 *
 * @tparam  T The type of terminal category
 * @param   duration_ms   The frame duration on this band
 * @param   cat1          The category with to much carriers
 * @param   cat2          The category with to less carriers
 * @param   carriers      The informations about the carriers to be transfer
 * @return  true on success, false otherwise
 */
	template<class T>
	bool carriersTransfer(time_ms_t duration_ms, T* cat1, T* cat2,
	                       map<rate_symps_t , unsigned int> carriers);

	/// the RCS2 required burst length in symbol
	vol_b_t req_burst_length;

	/// the current super frame number
	time_sf_t super_frame_counter;

	/// the frame durations
	time_ms_t fwd_down_frame_duration_ms;
	time_ms_t ret_up_frame_duration_ms;

	/// The encapsulation packet handler
	EncapPlugin::EncapPacketHandler *pkt_hdl;

	/// The statistics period
	time_ms_t stats_period_ms;
	time_frame_t stats_period_frame;

	// log
	std::shared_ptr<OutputLog> log_init_channel;
	std::shared_ptr<OutputLog> log_receive_channel;
	std::shared_ptr<OutputLog> log_send_channel;

	static std::shared_ptr<OutputLog> dvb_fifo_log;

 private:
	/// Whether we can send stats or not (can send stats when 0)
	time_frame_t check_send_stats;

};


/**
 * @brief Get integer values separated by ',' or ';' or a space
 *        in a string.
 *        Used for temporal division in VCM carriers
 *
 * @param values  The value to split
 * @return The vector containing the splitted values
 */
inline vector<unsigned int> tempSplit(string values)
{
	vector<string>::iterator it;
	vector<string> first_step;
	vector<unsigned int> output;

	// first get groups of strings separated by ';'
	tokenize(values, first_step, ";");
	for(it = first_step.begin(); it != first_step.end(); ++it)
	{
		vector<string> second_step;
		vector<string>::iterator it2;

		// then get groups of strings separated by ','
		tokenize(*it, second_step, ",");
		for(it2 = second_step.begin(); it2 != second_step.end(); ++it2)
		{
			vector<string> third_step;
			vector<string>::iterator it3;

			// then split the integers separated by '-'
			tokenize(*it2, third_step, "-");
			for(it3 = third_step.begin(); it3 != third_step.end(); ++it3)
			{
				std::stringstream str(*it3);
				unsigned int val;
				str >> val;
				if(str.fail())
				{
					continue;
				}
				output.push_back(val);
			}
		}
	}
	return output;
}

// Implementation of functions with templates

template<class T>
bool DvbChannel::initBand(const OpenSandModelConf::spot &spot,
                          string section,
                          AccessType access_type,
                          time_ms_t duration_ms,
                          const FmtDefinitionTable *fmt_def,
                          TerminalCategories<T> &categories,
                          TerminalMapping<T> &terminal_affectation,
                          T **default_category,
                          fmt_groups_t &fmt_groups)
{
	// Get the value of the bandwidth
	freq_khz_t bandwidth_khz = spot.bandwidth_khz;
	LOG(this->log_init_channel, LEVEL_INFO,
	    "%s: bandwitdh is %u kHz\n",
	    section.c_str(), bandwidth_khz);

	// Get the value of the roll off
	double roll_off = spot.roll_off;

	unsigned int carrier_id = 0;
	group_id_t group_id = 0;
	for (auto& carrier : spot.carriers) {
		bool is_vcm = carrier.format_ratios.size() > 1;
		for (auto& format_ratios : carrier.format_ratios) {
			FmtGroup *group = nullptr;
			std::string fmt_ids = format_ratios.first;
			if (carrier.access_type == access_type) {
				// we won't initialize FMT group here for other access
				group = new FmtGroup(++group_id, fmt_ids, fmt_def);
				fmt_groups[group_id] = group;

				auto modcod_amount = group->getFmtIds().size();
				if ((is_vcm || access_type == AccessType::ALOHA) && modcod_amount > 1) {
					LOG(this->log_init_channel, LEVEL_ERROR,
					    "Carrier cannot have more than one modcod for saloha or VCM\n");
					return false;
				}
			}

			std::string name = carrier.category;
			unsigned int ratio = format_ratios.second;
			rate_symps_t symbol_rate_symps = carrier.symbol_rate;

			// TODO: Improve this log, esp. for access type
			LOG(this->log_init_channel, LEVEL_NOTICE,
			    "%s: new carriers: category=%s, Rs=%G, FMTs=%s, "
			    "ratio=%d, access type=%d\n",
			    section.c_str(), name.c_str(),
			    symbol_rate_symps, fmt_ids.c_str(), ratio,
			    carrier.access_type);

			// group may be NULL if this is not the good access type, this should be
			// only used in other_carriers in TerminalCategory that won't access
			// fmt_groups

			// create the category if it does not exist
			// we also create categories with wrong access type because:
			//  - we may have many access types in the category
			//  - we need to get all carriers for band computation
			T *category;
			auto cat_iter = categories.find(name);
			if(cat_iter == categories.end())
			{
				category = new T(name, access_type);
				categories[name] = category;
			}
			else
			{
				category = dynamic_cast<T *>(cat_iter->second);
			}
			category->addCarriersGroup(carrier_id,
			                           group, ratio,
			                           symbol_rate_symps,
			                           carrier.access_type);
		}
		++carrier_id;
	}

	// Compute bandplan
	if(!this->computeBandplan(bandwidth_khz, roll_off, duration_ms, categories))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Cannot compute band plan for %s\n",
		    section.c_str());
		return false;
	}

	auto cat_iter = categories.begin();
	// delete category with no carriers corresponding to the access type
	while(cat_iter != categories.end())
	{
		T *category = (*cat_iter).second;
		// getCarriersNumber returns the number of carriers with the desired
		// access type only
		if(!category->getCarriersNumber())
		{
			LOG(this->log_init_channel, LEVEL_INFO,
			    "Skip category %s with no carriers with desired access type\n",
			    category->getLabel().c_str());
			categories.erase(cat_iter++);
			delete category;
		}
		else
		{
			++cat_iter;
		}
	}

	if(categories.size() == 0)
	{
		// no more category here, this will be handled by caller
		return true;
	}


	spot_id_t default_spot_id;
	string default_category_name;
	std::map<tal_id_t, std::pair<spot_id_t, std::string>> terminals;
	if (!OpenSandModelConf::Get()->getTerminalAffectation(default_spot_id,
	                                                      default_category_name,
	                                                      terminals))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Terminals categories initialisation failed\n");
		return false;
	}

	// Look for associated category
	*default_category = nullptr;
	cat_iter = categories.find(default_category_name);
	if(cat_iter != categories.end())
	{
		*default_category = (*cat_iter).second;
	}
	if(*default_category == nullptr)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Section %s, could not find category %s, "
		    "no default category for access type %u\n",
		    section.c_str(),
		    default_category_name.c_str(), access_type);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "ST default category: %s in %s\n",
		    (*default_category)->getLabel().c_str(),
		    section.c_str());
	}

	for (auto& terminal : terminals)
	{
		tal_id_t tal_id = terminal.first;
		string name = terminal.second.second;
		T *category = nullptr;
		cat_iter = categories.find(name);
		if (cat_iter != categories.end())
		{
			category = cat_iter->second;
		}
		if (category == nullptr)
		{
			LOG(this->log_init_channel, LEVEL_NOTICE,
			    "Could not find category %s for terminal %u affectation, "
			    "it is maybe concerned by another access type",
			    name.c_str(), tal_id);
			// keep the NULL affectation for this terminal to avoid
			// setting default category
			terminal_affectation[tal_id] = nullptr;
		}
		else
		{
			terminal_affectation[tal_id] = category;
			LOG(this->log_init_channel, LEVEL_INFO,
			    "%s: terminal %u will be affected to category %s\n",
			    section.c_str(), tal_id, name.c_str());
		}
	}

	return true;
}


template<class T>
bool DvbChannel::computeBandplan(freq_khz_t available_bandplan_khz,
                                 double roll_off,
                                 time_ms_t duration_ms,
                                 TerminalCategories<T> &categories)
{
	double weighted_sum_ksymps = 0.0;

	// compute weighted sum
  for (auto&& category_it : categories)
	{
		T *category = category_it.second;

		// Compute weighted sum in ks/s since available bandplan is in kHz.
		weighted_sum_ksymps += category->getWeightedSum();
	}

	LOG(this->log_init_channel, LEVEL_DEBUG,
	    "Weigthed ratio sum: %f ksym/s\n", weighted_sum_ksymps);

	if (weighted_sum_ksymps == 0.0)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Weighted ratio sum is 0\n");
		goto error;
	}

	// compute carrier number per category
  for (auto&& category_it : categories)
	{
		unsigned int carriers_number = 0;
		T *category = category_it.second;
		unsigned int ratio = category->getRatio();

		carriers_number = round(
		    (ratio / weighted_sum_ksymps) *
		    (available_bandplan_khz / (1 + roll_off)));
		// create at least one carrier
		if(carriers_number == 0)
		{
			LOG(this->log_init_channel, LEVEL_WARNING,
			    "Band is too small for one carrier. "
			    "Increase band for one carrier\n");
			carriers_number = 1;
		}
		LOG(this->log_init_channel, LEVEL_NOTICE,
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



/**
 * @brief Some FMT functions for Dvb spots ou channels
 */
class DvbFmt
{
 public:
	enum ModcodDefFileType
	{
		MODCOD_DEF_S2,
		MODCOD_DEF_RCS2,
	};

	DvbFmt();

	virtual ~DvbFmt();

	/**
	 * @brief setter of input_sts
	 *
	 * @param the new input_sts
	 */
	void setInputSts(StFmtSimuList* new_input_sts);

	/**
	 * @brief setter of output_sts
	 *
	 * @param the new output_sts
	 */
	void setOutputSts(StFmtSimuList* new_output_sts);

	/**
	 * @brief Get the current MODCOD ID of the ST whose ID is given as input
	 *        for input list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the current MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	uint8_t getCurrentModcodIdInput(tal_id_t id) const;

	/**
	 * @brief Get the current MODCOD ID of the ST whose ID is given as input
	 *        for output list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the current MODCOD ID of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	uint8_t getCurrentModcodIdOutput(tal_id_t id) const;

	/**
	 * @brief Get the CNI MODCOD ID of the ST whose ID is given as input
	 *        for output list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the CNI of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	double getRequiredCniInput(tal_id_t tal_id);

	/**
	 * @brief Get the CNI of the ST whose ID is given as input
	 *        for output list of sts
	 *
	 * @param id     the ID of the ST
	 * @return       the CNI of the ST
	 *
	 * @warning Be sure sure that the ID is valid before calling the function
	 */
	double getRequiredCniOutput(tal_id_t tal_id);

 protected:

	/**
	 * @brief Read configuration for the MODCOD definition file and create the
	 *        FmtDefinitionTable class
	 *
	 * @param def               The section in configuration file for MODCOD definitions
	 *                          (up/return or down/forward)
	 * @param modcod_def        The FMT Definition Table attribute to initialize
	 * @param req_burst_length  The required burst length (only for DVB-RCS2)
	 * @return  true on success, false otherwise
	 */
	bool initModcodDefFile(ModcodDefFileType def, FmtDefinitionTable **modcod_def, vol_sym_t req_burst_length = 0);

	/**
	 * @brief Add a new Satellite Terminal (ST) in the output list
	 *
	 * @param id           the ID of the ST
	 * @param modcod_def   the MODCOD definition for the terminal on the input link
	 * @return             true if the addition is successful, false otherwise
	 */
	bool addOutputTerminal(tal_id_t id,
	                       const FmtDefinitionTable *const modcod_def);

	/**
	 * @brief Add a new Satellite Terminal (ST) in the input list
	 *
	 * @param id           the ID of the ST
	 * @param modcod_def   the MODCOD definition for the terminal on the input link
	 * @return             true if the addition is successful, false otherwise
	 */
	bool addInputTerminal(tal_id_t id,
	                      const FmtDefinitionTable *const modcod_def);

	/**
	 * @brief Delete a Satellite Terminal (ST) from the output list
	 *
	 * @param id      the ID of the ST
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delOutputTerminal(tal_id_t id);

	/**
	 * @brief Delete a Satellite Terminal (ST) from the input list
	 *
	 * @param id      the ID of the ST
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delInputTerminal(tal_id_t id);

	/**
	 * @brief Set the required  CNI for of the ST in input
	 *        whid ID is given as input according to the required Es/N0
	 *
	 * @param id               the ID of the ST
	 * @param cni              the required Es/N0 for that terminal
	 */
	void setRequiredCniInput(tal_id_t tal_id, double cni);

	/**
	 * @brief Set the required  Cni for of the ST in output
	 *        whid ID is given as input according to the required Es/N0
	 *
	 * @param id               the ID of the ST
	 * @param cni              the required Es/N0 for that terminal
	 */
	void setRequiredCniOutput(tal_id_t tal_id, double cni);

	/**
	 * @brief get the modcod change state
	 *
	 * @return the modcod change state
	 */
	bool getCniInputHasChanged(tal_id_t tal_id);

	/**
	 * @brief get the modcod change state
	 *
	 * @return the modcod change state
	 */
	bool getCniOutputHasChanged(tal_id_t tal_id);

	/**
	 * set extension to the packet GSE
	 *
	 * @param pkt_hdl         The GSE packet handler
	 * @param elem            The fifo element to replace by the
	 *                        packet with extension
	 * @param fifo            The fifo to place the element
	 * @param packet     The input packet
	 * @param extension_pkt   The return packet with extension
	 * @param source          The terminal source id
	 * @param dest            The terminal dest id
	 * @param extension_name  The name of the extension we need to add as
	 *                        declared in plugin
	 * @param super_frame_counter  The superframe counter (for debug messages)
	 * @param is_gw           Whether we are on GW or not
	 *
	 * @return true on success, false otherwise
	 */
	bool setPacketExtension(EncapPlugin::EncapPacketHandler *pkt_hdl,
	                        MacFifoElement *elem,
	                        DvbFifo *fifo,
	                        NetPacket* packet,
	                        NetPacket **extension_pkt,
	                        tal_id_t source,
	                        tal_id_t dest,
	                        string extension_name,
	                        time_sf_t super_frame_counter,
	                        bool is_gw);

	/** The internal map that stores all the STs and modcod id for input */
	StFmtSimuList *input_sts;

	/// The MODCOD Definition Table for S2
	FmtDefinitionTable *s2_modcod_def;

	/** The internal map that stores all the STs and modcod id for output */
	StFmtSimuList *output_sts;

	/// The MODCOD Definition Table for RCS
	FmtDefinitionTable *rcs_modcod_def;

	/// The ACM loop margin
	double fwd_down_acm_margin_db;
	double ret_up_acm_margin_db;

	// log
	std::shared_ptr<OutputLog> log_fmt;

 private:
	/// Whether we can send stats or not (can send stats when 0)
	time_frame_t check_send_stats;

	/**
	 * @brief Delete a Satellite Terminal (ST) from the list
	 *
	 * @param id      the ID of the ST
	 * @param sts     the list which we have to delete a st
	 * @return    true if the deletion is successful, false otherwise
	 */
	bool delTerminal(tal_id_t id, StFmtSimuList* sts);

};

template<class T>
bool DvbChannel::allocateBand(time_ms_t duration_ms,
                              string cat_label,
                              rate_kbps_t new_rate_kbps,
                              TerminalCategories<T> &categories)
{
	// Category SNO (the default one)
	string cat_sno_label ("SNO");
	typename map<string, T*>::iterator cat_sno_it = categories.find(cat_sno_label);
	if(cat_sno_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "%s category doesn't exist",
		    cat_sno_label.c_str());
		return false;
	}
	T* cat_sno = cat_sno_it->second;

	// The category we are interesting on
	typename map<string, T*>::iterator cat_it = categories.find(cat_label);
	if(cat_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "This category %s doesn't exist yet\n",
		    cat_label.c_str());
		return false; //TODO or create it ?
	}
	T* cat = cat_it->second;

	// Fmt
	const FmtDefinitionTable* fmt_definition_table;
	const FmtGroup* cat_fmt_group = cat->getFmtGroup();

	unsigned int id;
	rate_symps_t new_rs;
	rate_symps_t old_rs;
	rate_symps_t rs_sno;
	rate_symps_t rs_needed;
	map<rate_symps_t, unsigned int> carriers;


	// Get the FMT Definition Table
	fmt_definition_table = cat_fmt_group->getModcodDefinitions();

	// Get the new total symbol rate
	id = cat_fmt_group->getMaxFmtId();
	new_rs = fmt_definition_table->kbitsToSym(id, new_rate_kbps);

	// Get the old total symbol rate
	old_rs = cat->getTotalSymbolRate();

	if(new_rs <= old_rs)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Request for an allocation while the rate (%.2E symps) is smaller "
		    "than before (%.2E symps)\n", new_rs, old_rs);
		return false;
	}

	// Calculation of the symbol rate needed
	rs_needed = new_rs - old_rs;

	// Get the total symbol rate available
	rs_sno = cat_sno->getTotalSymbolRate();

	if(rs_sno < rs_needed)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Not enough rate available\n");
		return false;
	}

	if(!this->carriersTransferCalculation(cat_sno, rs_needed, carriers))
	{
		return false;
	}

	return this->carriersTransfer(duration_ms, cat_sno, cat, carriers);
}

template<class T>
bool DvbChannel::releaseBand(time_ms_t duration_ms,
                             string cat_label,
                             rate_kbps_t new_rate_kbps,
                             TerminalCategories<T> &categories)
{
	// Category SNO (the default one)
	string cat_sno_label ("SNO");
	typename map<string, T*>::iterator cat_sno_it = categories.find(cat_sno_label);
	if(cat_sno_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "%s category doesn't exist",
		    cat_sno_label.c_str());
		return false;
	}
	T* cat_sno = cat_sno_it->second;

	// The category we are interesting on
	typename map<string, T*>::iterator cat_it = categories.find(cat_label);
	if(cat_it == categories.end())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "This category %s doesn't exist\n",
		    cat_label.c_str());
		return false;
	}
	T* cat = cat_it->second;

	// Fmt
	const FmtDefinitionTable* fmt_definition_table;
	const FmtGroup* cat_fmt_group = cat->getFmtGroup();

	unsigned int id;
	rate_symps_t new_rs;
	rate_symps_t old_rs;
	rate_symps_t rs_unneeded;
	map<rate_symps_t, unsigned int> carriers;


	// Get the FMT Definition Table
	fmt_definition_table = cat_fmt_group->getModcodDefinitions();

	// Get the new total symbol rate
	id = cat_fmt_group->getMaxFmtId();
	new_rs = fmt_definition_table->kbitsToSym(id, new_rate_kbps);

	// Get the old total symbol rate
	old_rs = cat->getTotalSymbolRate();

	if(old_rs <= new_rs)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Request for an release while the rate is higher than before\n");
		return false;
	}

	// Calculation of the symbol rate needed
	rs_unneeded = old_rs - new_rs;

	if(!this->carriersTransferCalculation(cat, rs_unneeded, carriers))
		return false;

	if(rs_unneeded < 0)
	{
		carriers.begin()->second -= 1;
		rs_unneeded += carriers.begin()->first; // rs_unneeded should be positive
	}

	return this->carriersTransfer(duration_ms, cat, cat_sno, carriers);
}

template<class T>
bool DvbChannel::carriersTransferCalculation(T* cat, rate_symps_t &rate_symps,
                                             map<rate_symps_t, unsigned int> &carriers)
{
	unsigned int num_carriers;

	// List of the carriers available (Rs, number)
	map<rate_symps_t, unsigned int> carriers_available;
	map<rate_symps_t, unsigned int>::reverse_iterator carriers_ite1;
	map<rate_symps_t, unsigned int>::reverse_iterator carriers_ite2;


	// Get the classification of the available
	// carriers in function of their symbol rate
	carriers_available = cat->getSymbolRateList();

	// Calculation of the needed carriers
	carriers_ite1 = carriers_available.rbegin();
	while(rate_symps > 0)
	{
		if(carriers_ite1 == carriers_available.rend())
		{
			if(carriers.find(carriers_ite2->first) == carriers.end())
			{
				carriers.insert(std::make_pair<rate_symps_t, unsigned int>(
				                   (rate_symps_t) carriers_ite2->first,
				                   (unsigned int) 1));
			}
			else
			{
				carriers.find(carriers_ite2->first)->second += 1;
			}
			rate_symps -= carriers_ite2->first; // rate should be negative now
			carriers_available.find(carriers_ite2->first)->second -= 1;
			// Erase the smaller carriers (because they are wasted)
			carriers_ite2++;
			while(carriers_ite2 != carriers_available.rend())
			{
				if(carriers.find(carriers_ite2->first) != carriers.end())
				{
					// rate should still be negative after that
					rate_symps += (carriers_ite2->first * carriers_ite2->second);
					carriers_available.find(carriers_ite2->first)->second
					    += carriers_ite2->second;
				}
				carriers.erase(carriers_ite2->first);
				carriers_ite2++;
			}
			continue;
		}
		if(rate_symps < carriers_ite1->first)
		{
			// in case the next carriers aren't enought
			carriers_ite2 = carriers_ite1;
			carriers_ite1++;
			continue;
		}
		num_carriers = floor(rate_symps/carriers_ite1->first);
		if(num_carriers > carriers_ite1->second)
		{
			num_carriers = carriers_ite1->second;
		}
		carriers_available.find(carriers_ite1->first)->second -= num_carriers;
		carriers.insert(std::make_pair<rate_symps_t, unsigned int>(
		                    (rate_symps_t) carriers_ite1->first,
		                    (unsigned int) num_carriers));
		rate_symps -= (carriers_ite1->first * num_carriers);
		if(num_carriers != carriers_ite1->second)
		{
			carriers_ite2 = carriers_ite1;
		}
		carriers_ite1++;
	}

	return true;
}

template<class T>
bool DvbChannel::carriersTransfer(time_ms_t duration_ms, T* cat1, T* cat2,
	                               map<rate_symps_t , unsigned int> carriers)
{
	unsigned int highest_id;
	unsigned int associated_ratio;

	// Allocation and deallocation of carriers
	highest_id = cat2->getHighestCarrierId();
	for(map<rate_symps_t, unsigned int>::iterator it = carriers.begin();
	    it != carriers.end(); it++)
	{
		if(it->second == 0)
		{
			LOG(this->log_init_channel, LEVEL_INFO,
			    "Empty carriers group\n");
			continue;
		}

		// Deallocation of SNO carriers
		if(!cat1->deallocateCarriers(it->first, it->second, associated_ratio))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Wrong calculation of the needed carriers");
			return false;
		}

		// Allocation of cat carriers
		highest_id++;
		cat2->addCarriersGroup(highest_id, cat2->getFmtGroup(),
		                       it->second, associated_ratio,
		                       it->first, cat2->getDesiredAccess(),
		                       duration_ms);
	}

	return true;
}


#endif

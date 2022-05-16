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
 * @file    TerminalCategory.h
 * @brief   Represent a category of terminal
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 * @author  Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#ifndef _TERMINAL_CATEGORY_H
#define _TERMINAL_CATEGORY_H

#include "OpenSandCore.h"
#include "TerminalContext.h"
#include "CarriersGroup.h"

#include <opensand_output/Output.h>

#include <string>
#include <map>
#include <vector>


/**
 * @class TerminalCategory
 * @brief Template for a category of terminal.
 */
template<class T = CarriersGroup>
class TerminalCategory
{

 public:

	/**
	 * @brief  Create a terminal category.
	 *
	 * @param  label           label of the category.
	 * @param  desired_access  the access type we support for our carriers
	 */
	TerminalCategory(const std::string& label, AccessType desired_access):
		terminals(),
		carriers_groups(),
		desired_access(desired_access),
		label(label),
		symbol_rate_list(),
		other_carriers()
	{
		// Output log
		this->log_terminal_category = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Ncc.Band");
	};

	virtual ~TerminalCategory()
	{
		typename std::vector<T *>::const_iterator it;
    std::vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			delete *it;
		}

		// delete other carriers in case updateCarriersGroups where not called
		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			delete *other_it;
		}
		// do not delete terminals, they will be deleted in DAMA or SlottedAloha
	};


	/**
	 * @brief  Get the label.
	 *
	 * @return  the label.
	 */
  std::string getLabel() const
	{
		return this->label;
	};


	/**
	 * @brief Get the weighted sum among all carriers groups on this category
	 *
	 * @return the weighted sum
	 */
	double getWeightedSum() const
	{
		// Compute weighted sum in ks/s since available bandplan is in kHz
		double weighted_sum_ksymps = 0.0;
		typename std::vector<T *>::const_iterator it;
    std::vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			// weighted_sum = ratio * Rs (ks/s)
			weighted_sum_ksymps += (*it)->getRatio() * (*it)->getSymbolRate() / 1E3;
		}

		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			// weighted_sum = ratio * Rs (ks/s)
			weighted_sum_ksymps += (*other_it)->getRatio() * (*other_it)->getSymbolRate() / 1E3;
		}

		return weighted_sum_ksymps;
	};

	/**
	 * @brief  Get the estimated occupation ratio other all carriers groups of
	 *         this category.
	 *
	 * @return  the estimated occupation ratio.
	 */
	unsigned int getRatio() const
	{
		unsigned int ratio = 0;
		typename std::vector<T *>::const_iterator it;
    std::vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			ratio += (*it)->getRatio();
		}

		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			ratio += (*other_it)->getRatio();
		}

		return ratio;
	};

 	/**
	 * @brief  Get the sum of the maximum rate of all carriers
	 *
	 * @return The total rate
	 */
	rate_kbps_t getMaxRate() const
	{
		rate_kbps_t rate_kbps;
		typename std::vector<T *>::iterator it;
		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); it++)
		{
			rate_kbps += (*it)->getMaxRate();
		}
		return rate_kbps;
	}

	/**
	 * @brief  Get the total symbol rate.
	 *
	 * @return  the total symbol rate.
	 */
	rate_symps_t getTotalSymbolRate() const
	{
		rate_symps_t  rate_symps = 0;
		typename std::vector<T *>::const_iterator it;
    std::vector<CarriersGroup *>::const_iterator other_it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			rate_symps += (*it)->getCarriersNumber() * (*it)->getSymbolRate();
		}

		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			rate_symps += (*other_it)->getCarriersNumber() * (*other_it)->getSymbolRate();
		}

		return rate_symps;
	};


	/**
	 * @brief  Set the number and the capacity of carriers in each group
	 *
	 * @param  carriers_number         The number of carriers in the group
	 * @param  superframe_duration_ms  The superframe duration (in ms)
	 */
	void updateCarriersGroups(unsigned int carriers_number,
	                          time_ms_t superframe_duration_ms)
	{
		unsigned int total_ratio = this->getRatio();
		unsigned int total_number = 0;
		typename std::vector<T *>::const_iterator it;
    std::vector<CarriersGroup *>::const_iterator other_it;

		if(carriers_number < this->carriers_groups.size())
		{
			LOG(this->log_terminal_category, LEVEL_WARNING, 
			    "Not enough carriers for category %s that contains %zu "
			    "groups. Increase carriers number to the number of "
			    "groups\n",
			    this->label.c_str(), this->carriers_groups.size());
			carriers_number = this->carriers_groups.size();
		}
		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			unsigned int number;
			vol_sym_t capacity_sym;
			rate_symps_t rs_symps = (*it)->getSymbolRate();
	
			// get number per carriers from total number in category
			number = round(carriers_number * (*it)->getRatio() / total_ratio);
			if(number == 0)
			{
				number = 1;
			}
			total_number += number;
			(*it)->setCarriersNumber(number);
			LOG(this->log_terminal_category, LEVEL_NOTICE, 
			    "Carrier group %u: number of carriers %u\n",
			    (*it)->getCarriersId(), number);

			if(this->symbol_rate_list.find(rs_symps) == this->symbol_rate_list.end())
			{
				this->symbol_rate_list.insert(
				    std::make_pair<rate_symps_t, unsigned int>((rate_symps_t)rs_symps,
									       (unsigned int) number));
			}
			else
			{
				this->symbol_rate_list.find(rs_symps)->second += number;
			}
	
			// get the capacity of the carriers
			capacity_sym = ceil(rs_symps * superframe_duration_ms / 1000.0);
			(*it)->setCapacity(capacity_sym);
			LOG(this->log_terminal_category, LEVEL_NOTICE, 
			    "Carrier group %u: capacity for Symbol Rate %.2E: %u "
			    "symbols\n", (*it)->getCarriersId(),
			    rs_symps, capacity_sym);
		}
		// no need to update other groups, they won't be used anymore
		// then released them
		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			delete *other_it;
		}
		this->other_carriers.clear();
	};

	/**
	 * @brief  Add a terminal to the category
	 *
	 * @param  terminal  terminal to be added.
	 */
	void addTerminal(TerminalContext *terminal)
	{
		terminal->setCurrentCategory(this->label);
		this->terminals.push_back(terminal);
	};

	/**
	 * @brief  Remove a terminal from the category
	 *
	 * @param  terminal  terminal to be removed.
	 * @return true on success, false otherwise
	 */
	bool removeTerminal(TerminalContext *terminal)
	{
    std::vector<TerminalContext *>::iterator terminal_it
		                                    = this->terminals.begin();
		const tal_id_t tal_id = terminal->getTerminalId();
		while(terminal_it != this->terminals.end()
			  && (*terminal_it)->getTerminalId() != tal_id)
		{
			++terminal_it;
		}
	
		if(terminal_it != this->terminals.end())
		{
			this->terminals.erase(terminal_it);
		}
		else
		{
			LOG(this->log_terminal_category, LEVEL_ERROR, 
			    "ST#%u not registered on category %s",
			    tal_id, this->label.c_str());
			return false;
		}
		return true;
	};

	/**
	 * @brief   Get the carriers groups with the desired access type
	 *
	 * @return  the carriers groups
	 */
  std::vector<T *> getCarriersGroups(void) const
	{
		return this->carriers_groups;
	};

	/**
	 * @brief  Add a carriers group to the category
	 *
	 * @param  carriers_id  The ID of the carriers group
	 * @param  fmt_group    The FMT group associated to the carrier
	 * @param  ratio        The estimated occupation ratio
	 * @param  rate_symps   The group symbol rate (symbol/s)
	 * @param  access_type  The carriers access type
	 */
	void addCarriersGroup(unsigned int carriers_id,
	                      const FmtGroup *const fmt_group,
	                      unsigned int ratio,
	                      rate_symps_t rate_symps,
	                      AccessType access_type)
	{
		typename std::vector<T *>::const_iterator it;
    std::vector<CarriersGroup *>::const_iterator other_it;
		// first, check if we already have this carriers id in case
		// of VCM carriers
		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			if((*it)->getCarriersId() == carriers_id)
			{
				(*it)->addVcm(fmt_group, ratio);
				return;
			}
		}
		for(other_it = this->other_carriers.begin();
		    other_it != this->other_carriers.end(); ++other_it)
		{
			if((*other_it)->getCarriersId() == carriers_id)
			{
				(*other_it)->addVcm(fmt_group, ratio);
				return;
			}
		}

		if(access_type == this->desired_access)
		{
			T *group = new T(carriers_id, fmt_group,
			                 ratio, rate_symps,
			                 access_type);
			// we call that because with Dama we need to count this carriers
			// in the VCM list
			group->addVcm(fmt_group, ratio);
			this->carriers_groups.push_back(group);
		}
		else
		{
			CarriersGroup *group = new CarriersGroup(carriers_id, fmt_group,
			                                         ratio, rate_symps,
			                                         access_type);
			this->other_carriers.push_back(group);
		}
		if(this->symbol_rate_list.find(rate_symps) == this->symbol_rate_list.end())
		{
			this->symbol_rate_list.insert(
			       std::make_pair<rate_symps_t, unsigned int>((rate_symps_t) rate_symps,
									  (unsigned int) 0));
		}
	};


	/**
	 * @brief   Get number of carriers with the desired access type.
	 *
	 * @return  number of carriers.
	 */
	unsigned int getCarriersNumber(void) const
	{
		unsigned int carriers_number = 0;
		typename std::vector<T *>::const_iterator it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); ++it)
		{
			carriers_number += (*it)->getCarriersNumber();
		}
		return carriers_number;
	};

	/**
	 * @brief   Get terminal list.
	 *
	 * @return  terminal list.
	 */
  std::vector<TerminalContext *> getTerminals() const
	{
		return this->terminals;
	};


	/**
	 * @brief  Add a carriers group to the category
	 *         If a group with the same symbol rate exist,
	 *         increase the carriers number and ratio
	 *
	 * @param  carriers_id      The ID of the carriers group
	 * @param  fmt_group        The FMT group associated to the carrier
	 * @param  carriers_number  The number of carriers
	 * @param  ratio            The estimated occupation ratio
	 * @param  rate_symps       The group symbol rate (symbol/s)
	 * @param  access_type      The carriers access type
	 * @param  duration_ms      The duration of a carrier (in ms)
	 */
	void addCarriersGroup(unsigned int carriers_id,
	                      const FmtGroup *const fmt_group,
	                      unsigned int carriers_number,
	                      unsigned int ratio,
	                      rate_symps_t rate_symps,
	                      AccessType access_type,
	                      time_ms_t duration_ms)
	{
		typename std::vector<T *>::const_iterator it;
		// first, we check if there is already a group with this symbol rate
		T* carriers_group = this->searchCarriersGroup(rate_symps);
		if(carriers_group != NULL)
		{
			carriers_group->setCarriersNumber(carriers_number + 
			                        carriers_group->getCarriersNumber());
			carriers_group->setRatio(ratio + carriers_group->getRatio());
		}
		else
		{
			// second, check if we already have this carriers id in case
			// of VCM carriers
			for(it = this->carriers_groups.begin();
			    it != this->carriers_groups.end(); ++it)
			{
				if((*it)->getCarriersId() == carriers_id)
				{
					(*it)->addVcm(fmt_group, ratio);
					return;
				}
			}

			if(access_type == this->desired_access)
			{
				T *group = new T(carriers_id, fmt_group,
				                 ratio, rate_symps,
				                 access_type);
				// we call that because with Dama we need to count this carriers
				// in the VCM list
				group->addVcm(fmt_group, ratio);
				group->setCarriersNumber(carriers_number);
				vol_sym_t capacity = floor(rate_symps*duration_ms/1000);
				group->setCapacity(capacity);
				this->carriers_groups.push_back(group);
			}
			else
			{
				CarriersGroup *group = new CarriersGroup(carriers_id, fmt_group,
				                                         ratio, rate_symps,
				                                         access_type);
				group->setCarriersNumber(carriers_number);
				vol_sym_t capacity = floor(rate_symps*duration_ms/1000);
				group->setCapacity(capacity);
			}
			if(this->symbol_rate_list.find(rate_symps) == this->symbol_rate_list.end()) {
				this->symbol_rate_list.insert(
				    std::make_pair<rate_symps_t, unsigned int>(
				         (rate_symps_t) rate_symps, (unsigned int) carriers_number));
			}
			else
			{
				this->symbol_rate_list.find(rate_symps)->second
				                                      += carriers_number;
			}
		}
	}


	/**
	 * @brief   Get the symbol_rate_list
	 *
	 * @return  the symbol_rate_list.
	 */
  std::map<rate_symps_t, unsigned int> getSymbolRateList() const
	{
		return this->symbol_rate_list;
	}

	/**
	 * @brief   Get the highest carrier id
	 *
	 * @return  the highest carrier id
	 */
	unsigned int getHighestCarrierId() const
	{
		unsigned int max_carrier_id = 0;
		typename std::vector<T *>::const_iterator it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); it++)
		{
			if((*it)->getCarriersId() > max_carrier_id)
			{
				max_carrier_id = (*it)->getCarriersId();
			}
		}
		return max_carrier_id;
	}

	/**
	 * @brief   Deallocation of carriers
	 *
	 * @param   symbol_rate        the symbol rate of the carrier to deallocate
	 * @param   number             the number of carriers to deallocate
	 * @param   associated_ratio   OUT: the associated ratio
	 *
	 * @return  true on success, false otherwise
	 */
	bool deallocateCarriers(rate_symps_t symbol_rate,
	                        unsigned int number,
	                        unsigned int &associated_ratio)
	{
		typename std::vector<T *>::iterator it;
		unsigned int number_carriers = number;
		unsigned int ratio;
		unsigned int new_ratio;
		unsigned int actual_number;

		associated_ratio = 0;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); it++)
		{
			actual_number = (*it)->getCarriersNumber();
			if(actual_number == 0)
			{
				LOG(this->log_terminal_category, LEVEL_INFO,
				    "Empty carrier\n");
				continue;
			}
			ratio = (*it)->getRatio();
			if((*it)->getSymbolRate() == symbol_rate)
			{
				if(actual_number < number_carriers)
				{
					number_carriers -= actual_number;
					(*it)->setCarriersNumber(0);
					associated_ratio += ratio;
					(*it)->setRatio(0);
					continue;
				}
				else
				{
					new_ratio = floor((ratio * (actual_number - number_carriers)
					            / actual_number) + 0.5);
					associated_ratio += (ratio - new_ratio);
					(*it)->setRatio(new_ratio);
					(*it)->setCarriersNumber(actual_number - number_carriers);
					number_carriers = 0;
					break;
				}
			}
		}
		if(number_carriers > 0)
			return false;

		return true;
	}

	/**
	 * @brief   Get the access type of the carriers
	 *
	 * @return  the access type of the carriers
	 */
	AccessType getDesiredAccess() const
	{
		return this->desired_access;
	}

	/**
	 * @brief  Get the fmt_group of the category (same for all carriers)
	 *
	 * @return the fmt_group
	 */
	const FmtGroup* getFmtGroup()
	{
		typename std::vector<T *>::iterator it = this->carriers_groups.begin();
		if(it == this->carriers_groups.end())
		{
			return NULL;
		}
		return (*it)->getFmtGroup();
	}

	/**
	 * @brief  Print the category (for debug)
	 */
	void print()
	{
		typename std::vector<T *>::iterator it;
		LOG(this->log_terminal_category, LEVEL_ERROR,
		    "Name : %s, access type = %u\n",
		    this->label.c_str(), this->desired_access);
		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); it++)
		{
			LOG(this->log_terminal_category, LEVEL_ERROR,
			    "carriers_id = %u, carriers_number = %u,"
			    " ratio = %u, symbol_rate = %lf\n",
			    (*it)->getCarriersId(), (*it)->getCarriersNumber(),
			    (*it)->getRatio(), (*it)->getSymbolRate());
		}
	}

	/**
	 * @brief  Search if there is a carriers group with this symbol rate
	 *
	 * @param  symbol_rate   The Rs of the carriers
	 *
	 * @return NULL if there is no carriers group with this symbol rate,
	 *         the carriers group otherwise
	 */
	T* searchCarriersGroup(rate_symps_t symbol_rate)
	{
		T* carriers_group = NULL;
		typename std::vector<T *>::iterator it;

		for(it = this->carriers_groups.begin();
		    it != this->carriers_groups.end(); it++)
		{
			if((*it)->getSymbolRate() == symbol_rate)
			{
				carriers_group = (*it);
				break;
			}
		}

		return carriers_group;
	}

 protected:
	// Output Log
  std::shared_ptr<OutputLog> log_terminal_category;

	/** List of terminals. */
  std::vector<TerminalContext *> terminals;

	/** List of carriers */
  std::vector<T *> carriers_groups;

	/** The access type of the carriers */
	AccessType desired_access;
	
	/** The label */
  std::string label;

	/** The list of Symbol rate, list(rs,nb_carriers) **/
  std::map<rate_symps_t, unsigned int> symbol_rate_list;

 private:
	/** The carriers groups that does not correspond to the desired access type
	 *   needed for band computation */
  std::vector<CarriersGroup *> other_carriers;
};


template<class T>
class TerminalCategories: public std::map<std::string, T *> {};
template<class T>
class TerminalMapping: public std::map<tal_id_t, T *> {};

#endif


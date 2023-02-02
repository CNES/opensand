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
#include <numeric>


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
		for(auto&& carrier : this->carriers_groups)
		{
			delete carrier;
		}

		// delete other carriers in case updateCarriersGroups where not called
		for(auto&& carrier : this->other_carriers)
		{
			delete carrier;
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
		static const auto get_symbols_rate = [](double weighted_sum_symps, CarriersGroup * carrier)
		{
			return weighted_sum_symps + carrier->getRatio() * carrier->getSymbolRate();
		};

		// Compute weighted sum in ks/s since available bandplan is in kHz
		return (std::accumulate(this->carriers_groups.begin(), this->carriers_groups.end(), 0.0, get_symbols_rate)
		      + std::accumulate(this->other_carriers.begin(), this->other_carriers.end(), 0.0, get_symbols_rate)) / 1e3;
	};

	/**
	 * @brief  Get the estimated occupation ratio other all carriers groups of
	 *         this category.
	 *
	 * @return  the estimated occupation ratio.
	 */
	unsigned int getRatio() const
	{
		static const auto get_ratio = [](unsigned int ratio, CarriersGroup * carrier)
		{
			return ratio + carrier->getRatio();
		};

		return (std::accumulate(this->carriers_groups.begin(), this->carriers_groups.end(), 0U, get_ratio)
		      + std::accumulate(this->other_carriers.begin(), this->other_carriers.end(), 0U, get_ratio));
	};

	/**
	 * @brief  Get the sum of the maximum rate of all carriers
	 *
	 * @return The total rate
	 */
	rate_kbps_t getMaxRate() const
	{
		static const auto get_rate = [](rate_kbps_t rate, CarriersGroup * carrier)
		{
			return rate + carrier->getMaxRate();
		};

		return std::accumulate(this->carriers_groups.begin(), this->carriers_groups.end(), rate_kbps_t{}, get_rate);
	}

	/**
	 * @brief  Get the total symbol rate.
	 *
	 * @return  the total symbol rate.
	 */
	rate_symps_t getTotalSymbolRate() const
	{
		static const auto get_total_symbols_rate = [](rate_symps_t rate_symps, CarriersGroup * carrier)
		{
			return rate_symps + carrier->getCarriersNumber() * carrier->getSymbolRate();
		};

		return (std::accumulate(this->carriers_groups.begin(), this->carriers_groups.end(), rate_symps_t{}, get_total_symbols_rate)
		      + std::accumulate(this->other_carriers.begin(), this->other_carriers.end(), rate_symps_t{}, get_total_symbols_rate));
	};


	/**
	 * @brief  Set the number and the capacity of carriers in each group
	 *
	 * @param  carriers_number         The number of carriers in the group
	 * @param  superframe_duration     The superframe duration (in μs)
	 */
	void updateCarriersGroups(unsigned int carriers_number,
	                          time_us_t superframe_duration)
	{
		unsigned int total_ratio = this->getRatio();

		if(carriers_number < this->carriers_groups.size())
		{
			LOG(this->log_terminal_category, LEVEL_WARNING, 
			    "Not enough carriers for category %s that contains %zu "
			    "groups. Increase carriers number to the number of "
			    "groups\n",
			    this->label.c_str(), this->carriers_groups.size());
			carriers_number = this->carriers_groups.size();
		}

		for(auto&& carrier : this->carriers_groups)
		{
			rate_symps_t rs_symps = carrier->getSymbolRate();
	
			// get number per carriers from total number in category
			unsigned int number = round(carriers_number * carrier->getRatio() / total_ratio);
			if(number == 0)
			{
				number = 1;
			}
			carrier->setCarriersNumber(number);
			LOG(this->log_terminal_category, LEVEL_NOTICE, 
			    "Carrier group %u: number of carriers %u\n",
			    carrier->getCarriersId(), number);

			if(this->symbol_rate_list.find(rs_symps) == this->symbol_rate_list.end())
			{
				this->symbol_rate_list.insert({rs_symps, number});
			}
			else
			{
				this->symbol_rate_list.find(rs_symps)->second += number;
			}

			// get the capacity of the carriers
			vol_sym_t capacity_sym = std::chrono::duration_cast<std::chrono::seconds>(rs_symps * superframe_duration).count();
			carrier->setCapacity(capacity_sym);
			LOG(this->log_terminal_category, LEVEL_NOTICE, 
			    "Carrier group %u: capacity for Symbol Rate %.2E: %u "
			    "symbols\n", carrier->getCarriersId(),
			    rs_symps, capacity_sym);
		}
		// no need to update other groups, they won't be used anymore
		// then released them
		for(auto&& carrier : this->other_carriers)
		{
			delete carrier;
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
		const tal_id_t tal_id = terminal->getTerminalId();
		auto terminal_it = std::find_if(this->terminals.begin(),
		                                this->terminals.end(),
		                                [tal_id](TerminalContext *t){ return t->getTerminalId() == tal_id; });

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
		// first, check if we already have this carriers id in case
		// of VCM carriers
		for(auto&& carrier : this->carriers_groups)
		{
			if(carrier->getCarriersId() == carriers_id)
			{
				carrier->addVcm(fmt_group, ratio);
				return;
			}
		}
		for(auto&& carrier : this->other_carriers)
		{
			if(carrier->getCarriersId() == carriers_id)
			{
				carrier->addVcm(fmt_group, ratio);
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
			this->symbol_rate_list.insert({rate_symps, 0});
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

		for(auto&& carrier : this->carriers_groups)
		{
			carriers_number += carrier->getCarriersNumber();
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
	 * @param  duration         The duration of a carrier
	 */
	void addCarriersGroup(unsigned int carriers_id,
	                      const FmtGroup *const fmt_group,
	                      unsigned int carriers_number,
	                      unsigned int ratio,
	                      rate_symps_t rate_symps,
	                      AccessType access_type,
	                      time_us_t duration)
	{
		// first, we check if there is already a group with this symbol rate
		T* carriers_group = this->searchCarriersGroup(rate_symps);
		if(carriers_group != NULL)
		{
			carriers_group->setCarriersNumber(carriers_number + carriers_group->getCarriersNumber());
			carriers_group->setRatio(ratio + carriers_group->getRatio());
		}
		else
		{
			// second, check if we already have this carriers id in case
			// of VCM carriers
			for(auto&& carrier : this->carriers_groups)
			{
				if(carrier->getCarriersId() == carriers_id)
				{
					carrier->addVcm(fmt_group, ratio);
					return;
				}
			}

			vol_sym_t capacity = std::chrono::duration_cast<std::chrono::seconds>(rate_symps * duration).count();
			if(access_type == this->desired_access)
			{
				T *group = new T(carriers_id, fmt_group,
				                 ratio, rate_symps,
				                 access_type);
				// we call that because with Dama we need to count this carriers
				// in the VCM list
				group->addVcm(fmt_group, ratio);
				group->setCarriersNumber(carriers_number);
				group->setCapacity(capacity);
				this->carriers_groups.push_back(group);
			}
			else
			{
				CarriersGroup *group = new CarriersGroup(carriers_id, fmt_group,
				                                         ratio, rate_symps,
				                                         access_type);
				group->setCarriersNumber(carriers_number);
				group->setCapacity(capacity);
			}
			if(this->symbol_rate_list.find(rate_symps) == this->symbol_rate_list.end()) {
				this->symbol_rate_list.insert({rate_symps, carriers_number});
			}
			else
			{
				this->symbol_rate_list.find(rate_symps)->second += carriers_number;
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

		for(auto&& carrier : this->carriers_groups)
		{
			if(carrier->getCarriersId() > max_carrier_id)
			{
				max_carrier_id = carrier->getCarriersId();
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
		unsigned int number_carriers = number;
		unsigned int ratio;
		unsigned int new_ratio;
		unsigned int actual_number;

		associated_ratio = 0;

		for(auto&& carrier : this->carriers_groups)
		{
			actual_number = carrier->getCarriersNumber();
			if(actual_number == 0)
			{
				LOG(this->log_terminal_category, LEVEL_INFO,
				    "Empty carrier\n");
				continue;
			}
			ratio = carrier->getRatio();
			if(carrier->getSymbolRate() == symbol_rate)
			{
				if(actual_number < number_carriers)
				{
					number_carriers -= actual_number;
					carrier->setCarriersNumber(0);
					associated_ratio += ratio;
					carrier->setRatio(0);
					continue;
				}
				else
				{
					new_ratio = floor((ratio * (actual_number - number_carriers) / actual_number) + 0.5);
					associated_ratio += (ratio - new_ratio);
					carrier->setRatio(new_ratio);
					carrier->setCarriersNumber(actual_number - number_carriers);
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
		auto it = this->carriers_groups.begin();
		if(it == this->carriers_groups.end())
		{
			return nullptr;
		}
		return (*it)->getFmtGroup();
	}

	/**
	 * @brief  Print the category (for debug)
	 */
	void print()
	{
		LOG(this->log_terminal_category, LEVEL_ERROR,
		    "Name : %s, access type = %u\n",
		    this->label.c_str(), this->desired_access);
		for(auto&& carrier : this->carriers_groups)
		{
			LOG(this->log_terminal_category, LEVEL_ERROR,
			    "carriers_id = %u, carriers_number = %u,"
			    " ratio = %u, symbol_rate = %lf\n",
			    carrier->getCarriersId(), carrier->getCarriersNumber(),
			    carrier->getRatio(), carrier->getSymbolRate());
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
		auto carriers_it = std::find_if(this->carriers_groups.begin(),
		                                this->carriers_groups.end(),
		                                [symbol_rate](CarriersGroup *carrier){ return carrier->getSymbolRate() == symbol_rate; });
		if (carriers_it == this->carriers_groups.end())
		{
		  return nullptr;
		}
		return *carriers_it;
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

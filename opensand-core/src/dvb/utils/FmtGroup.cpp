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
 * @file FmtGroup.cpp
 * @brief FMT group
 * @author Julien BERNARD / Viveris Technologies
 */


#include "FmtGroup.h"

#include "OpenSandCore.h"

#include <opensand_output/Output.h>

#include <vector>
#include <sstream>


FmtGroup::FmtGroup(unsigned int group_id,
                   std::string ids,
                   const FmtDefinitionTable &modcod_def):
	id(group_id),
	fmt_ids(),
	num_fmt_ids(),
	modcod_def(modcod_def)
{
	// Output log
	this->log_fmt = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.Fmt.Group");

	this->parse(ids);
};

fmt_id_t FmtGroup::getNearest(fmt_id_t fmt_id) const
{
	std::list<FmtId>::const_reverse_iterator it; 
	double esn0 = this->modcod_def.getRequiredEsN0(fmt_id);
	if(esn0 == 0.0)
	{
		LOG(this->log_fmt, LEVEL_ERROR,
		    "Cannot get nearest FMT id\n");
		return 0;
	}
	FmtId desired_fmt(fmt_id, esn0);
	// FMT IDs are sorted from more to less robust
	for(it = this->fmt_ids.rbegin();
	    it != this->fmt_ids.rend();
	    ++it)
	{
		if((*it) <= desired_fmt)
		{
			return (*it).id;
		}
	}
	return 0;
};

void FmtGroup::parse(std::string ids)
{
	std::vector<std::string>::iterator it;
	std::vector<std::string> first_step;
	std::list<fmt_id_t>::const_iterator id_it;

	// first get groups of strings separated by ';'
	tokenize(ids, first_step, ";");
	for(it = first_step.begin(); it != first_step.end(); ++it)
	{
		std::string temp = *it;
		std::vector<std::string> second_step;
		std::vector<std::string>::iterator it2;
		fmt_id_t previous_id = 0;

		// then split the integers separated by '-'
		tokenize(temp, second_step, "-");
		for(it2 = second_step.begin(); it2 != second_step.end(); ++it2)
		{
			std::stringstream str(*it2);
			unsigned int dummy;
			fmt_id_t val;
			str >> dummy;
			if(str.fail())
			{
				continue;
			}
			val = (fmt_id_t)dummy;
			// keep the current value if it does not exists
			if(std::find(this->fmt_ids.begin(),
			             this->fmt_ids.end(), val) == this->fmt_ids.end())
			{
				double esn0 = this->modcod_def.getRequiredEsN0(val);
				if(esn0 == 0.0)
				{
					LOG(this->log_fmt, LEVEL_ERROR,
					    "Cannot parse FMT group\n");
					continue;
				}
				FmtId fmt_id(val, esn0);

				this->fmt_ids.push_back(fmt_id);
				LOG(this->log_fmt, LEVEL_INFO,
				    "Add ID %u in FMT group %u\n", val, this->id);
			}
			if(previous_id == 0)
			{
				previous_id = val;
				continue;
			}

			// add the values between two tokens separated by '-'
			for(fmt_id_t i = std::min(previous_id + 1, val + 1);
			    i < std::max(previous_id, val); i++)
			{
				if(std::find(this->fmt_ids.begin(),
				             this->fmt_ids.end(), i) == this->fmt_ids.end())
				{
					double esn0 = this->modcod_def.getRequiredEsN0(i);
					if(esn0 == 0.0)
					{
						LOG(this->log_fmt, LEVEL_ERROR,
						    "Cannot parse FMT group\n");
						continue;
					}
					FmtId fmt_id(i, esn0);

					this->fmt_ids.push_back(fmt_id);
				}
			}

			previous_id = val;
		}
	}

	this->fmt_ids.sort();
	// we need the list of numeric IDs to avoid creating it each time
	// we call getFmtIds
	for(std::list<FmtId>::const_iterator id_it = this->fmt_ids.begin();
	    id_it != this->fmt_ids.end(); ++id_it)
	{
		LOG(this->log_fmt, LEVEL_INFO,
		    "Add ID %u in FMT group %u\n", (*id_it).id, this->id);
		this->num_fmt_ids.push_back((*id_it).id);
	}
}

const std::list<fmt_id_t> FmtGroup::getFmtIds() const
{
	return this->num_fmt_ids;
}

const FmtDefinitionTable &FmtGroup::getModcodDefinitions() const
{
	return this->modcod_def;
}

fmt_id_t FmtGroup::getMaxFmtId() const
{
	return this->num_fmt_ids.back();
}


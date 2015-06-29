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
 * @file FmtGroup.cpp
 * @brief FMT group
 * @author Julien BERNARD / Viveris Technologies
 */


#include "FmtGroup.h"

#include "OpenSandCore.h"

#include <opensand_output/Output.h>

#include <vector>
#include <sstream>

using std::vector;
using std::stringstream;

FmtGroup::FmtGroup(unsigned int group_id,
                   string ids,
                   const FmtDefinitionTable *modcod_def):
	id(group_id),
	fmt_ids(),
	num_fmt_ids(),
	modcod_def(modcod_def)
{
	// Output log
	this->log_fmt = Output::registerLog(LEVEL_WARNING, "Dvb.Fmt.Group");

	this->parse(ids);
};

unsigned int FmtGroup::getNearest(unsigned int fmt_id) const
{
	list<unsigned int>::const_reverse_iterator it;
	// FMT IDs are sorted from more to less robust
	for(it = this->num_fmt_ids.rbegin();
		it != this->num_fmt_ids.rend();
		++it)
	{
		if((*it) <= fmt_id)
		{
			return (*it);
		}
	}
	return 0;
};


void FmtGroup::parse(string ids)
{
	vector<string>::iterator it;
	vector<string> first_step;
	list<unsigned int>::const_iterator id_it;

	// first get groups of strings separated by ';'
	tokenize(ids, first_step, ";");
	for(it = first_step.begin(); it != first_step.end(); ++it)
	{
		string temp = *it;
		vector<string> second_step;
		vector<string>::iterator it2;
		unsigned int previous_id = 0;

		// then split the integers separated by '-'
		tokenize(temp, second_step, "-");
		for(it2 = second_step.begin(); it2 != second_step.end(); ++it2)
		{
			stringstream str(*it2);
			unsigned int val;
			str >> val;
			if(str.fail())
			{
				continue;
			}
			// keep the current value if it does not exists
			if(std::find(this->fmt_ids.begin(),
			             this->fmt_ids.end(), val) == this->fmt_ids.end())
			{
				FmtId fmt_id(val, this->modcod_def->getRequiredEsN0(val));
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
			for(unsigned int i = std::min(previous_id + 1, val + 1);
			    i < std::max(previous_id, val); i++)
			{
				if(std::find(this->fmt_ids.begin(),
				             this->fmt_ids.end(), i) == this->fmt_ids.end())
				{
					FmtId fmt_id(i, this->modcod_def->getRequiredEsN0(i));
					this->fmt_ids.push_back(fmt_id);
				}
			}

			previous_id = val;
		}
	}

	this->fmt_ids.sort();
	// we need the list of numeric IDs to avoid creating it each time
	// we call getFmtIds
	for(list<FmtId>::const_iterator id_it = this->fmt_ids.begin();
	    id_it != this->fmt_ids.end(); ++id_it)
	{
		LOG(this->log_fmt, LEVEL_INFO,
		    "Add ID %u in FMT group %u\n", (*id_it).id, this->id);
		this->num_fmt_ids.push_back((*id_it).id);
	}
}

const list<unsigned int> FmtGroup::getFmtIds() const
{
	return this->num_fmt_ids;
}

const FmtDefinitionTable *FmtGroup::getModcodDefinitions() const
{
	return this->modcod_def;
}

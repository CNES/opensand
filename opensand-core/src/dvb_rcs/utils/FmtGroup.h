/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file FmtGroup.h
 * @brief A group of FMT IDs
 * @author Julien BERNARD / Viveris Technologies
 */

#ifndef FMT_GROUP_H
#define FMT_GROUP_H

#include <list>
#include <string>
#include <map>

#include <opensand_output/OutputLog.h>

using std::list;
using std::string;
using std::map;

/**
 * @class FmtGroup
 * @brief The definition of a FMT
 */
class FmtGroup
{
 private:

	/** The ID of the FMT group */
	unsigned int id;

	list<unsigned int> fmt_ids;

 protected:
	// Output log
	OutputLog *log_fmt;

 public:

	/**
	 * @brief Create a new FMT group
	 *
	 * @param group_id  The group id
	 * @param fmt_ids   The FMT IDs to add
	 */
	FmtGroup(unsigned int group_id, string fmt_ids);

	/**
	 * @brief Get the nearest supported value in the group
	 *        i.e. get a FMT id equal or smaller in the group
	 *             as FMT are classified from more to less robust
	 *
	 * @param fmt_id  The desired FMT id
	 * @return The nearest available FMT id if found
	 *         0 if there is no supported FMT id
	 */
	unsigned int getNearest(unsigned int fmt_id) const;

	/**
	 * @brief Get the list of available MODCODs
	 *
	 * @return the list of MODCODs
	 */
	const list<unsigned int> getFmtIds() const;

  private:

	/**
	 * @brief parse the FMT IDs string read in configuration
	 *
	 * @param  fmt_ids  The FMT IDs as in configuration
	 */
	void parse(string fmt_ids);
};

typedef map<unsigned int, FmtGroup *> fmt_groups_t;

#endif

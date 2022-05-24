/*
 *
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
 * @file FmtGroup.h
 * @brief A group of FMT IDs
 * @author Julien BERNARD / Viveris Technologies
 */

#ifndef FMT_GROUP_H
#define FMT_GROUP_H


#include "FmtDefinitionTable.h"

#include <opensand_output/OutputLog.h>

#include <list>
#include <string>
#include <map>


class FmtId;

/**
 * @class FmtGroup
 * @brief The definition of a FMT
 */
class FmtGroup
{
 private:

	/** The ID of the FMT group */
	fmt_id_t id;

	/** The list of FMT IDs */
  std::list<FmtId> fmt_ids;

	/** The list of IDs from FMT IDs */
  std::list<fmt_id_t> num_fmt_ids;

	/** The table of MODCOD definitions */
	const FmtDefinitionTable *modcod_def;

 protected:
	// Output log
	std::shared_ptr<OutputLog> log_fmt;

 public:

	/**
	 * @brief Create a new FMT group
	 *
	 * @param group_id    The group id
	 * @param ids         The FMT IDs to add
	 * @param modcod_def  The MODCOD definitions
	 */
	FmtGroup(unsigned int group_id,
	         std::string ids,
	         const FmtDefinitionTable *modcod_def);

	/**
	 * @brief Get the nearest supported value in the group
	 *        i.e. get a FMT id equal or smaller in the group
	 *             as FMT are classified from more to less robust
	 *
	 * @param fmt_id  The desired FMT id
	 * @return The nearest available FMT id if found
	 *         0 if there is no supported FMT id
	 */
	fmt_id_t getNearest(fmt_id_t fmt_id) const;

	/**
	 * @brief Get the list of available MODCODs
	 *
	 * @return the list of MODCODs
	 */
	const std::list<fmt_id_t> getFmtIds() const;

	/**
	 * @brief Get the MODCOD definitions
	 *
	 * @return the MODCOD definitions
	 */
	const FmtDefinitionTable *getModcodDefinitions() const;

	/**
	 * @brief  Get the highest Fmt id
	 *
	 * @return the highest Fmt id
	 */
	fmt_id_t getMaxFmtId() const;

  private:

	/**
	 * @brief parse the FMT IDs string read in configuration
	 *
	 * @param  fmt_ids  The FMT IDs as in configuration
	 */
	void parse(std::string fmt_ids);
};


/**
 * @class FmtId
 * @brief A FMT ID
 */
class FmtId
{
 public:
	FmtId(fmt_id_t id, float es_n0):
		id(id),
		es_n0(es_n0)
	{};

	/// operator < used by sort on required Es/N0
	bool operator<(const FmtId &id) const
	{
		return (this->es_n0 < id.es_n0);
	}

	/// operator <= used to compare with FMT ID
	bool operator<=(fmt_id_t id) const
	{
		return (this->id <= id);
	}

	/// operator >= used to compare with FMT ID
	bool operator>=(fmt_id_t id) const
	{
		return (this->id >= id);
	}

	/// operator == used to compare with FMT ID
	bool operator==(fmt_id_t id) const
	{
		return (this->id == id);
	}

	/// operator != used to compare with FMT ID
	bool operator!=(fmt_id_t id) const
	{
		return (this->id != id);
	}

	/// operator <= used to compare two FmtId
	bool operator<=(const FmtId &id) const
	{
		return (this->es_n0 <= id.es_n0);
	}

	/// operator > used to compare two FmtId
	bool operator>(const FmtId &id) const
	{
		return (this->es_n0 > id.es_n0);
	}

	/// operator >= used to compare two FmtId
	bool operator>=(const FmtId &id) const
	{
		return (this->es_n0 >= id.es_n0);
	}

	/// operator == used to compare two FmtId
	bool operator==(const FmtId &id) const
	{
		return (this->es_n0 == id.es_n0);
	}

	/// operator != used to compare two FmtId
	bool operator!=(const FmtId &id) const
	{
		return (this->es_n0 != id.es_n0);
	}

	/// The FMT ID
	fmt_id_t id;

 private:
	/// The required Es/N0
	float es_n0;
};



typedef std::map<fmt_id_t, FmtGroup *> fmt_groups_t;


#endif

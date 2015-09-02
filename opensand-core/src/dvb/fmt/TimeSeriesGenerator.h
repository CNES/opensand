/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 CNES
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
 * @file TimeSeriesGenerator.h
 * @brief TimeSeriesGenerator
 * @author Julien BERNARD
 */

#ifndef TIME_SERIES_GENE_H
#define TIME_SERIES_GENE_H

#include "OpenSandCore.h"
#include "StFmtSimu.h"

#include <opensand_output/OutputLog.h>

#include <string>
#include <vector>
#include <fstream>


using std::string;
using std::vector;
using std::ofstream;

/**
 * @class TimeSeriesGenerator
 * @brief Write simulated MODCOD in a file that
 *        can be opened by the FMT simulator
 */
class TimeSeriesGenerator
{
 private:

	/// The list of previous modcods in order to keep
	/// correct modcods on a new entry
	vector<fmt_id_t> previous_modcods;

	/// The output file 
	ofstream output_file;

	/// The index
	uint32_t index;

	/// Logger
	OutputLog *simu_log;

 public:

	/**
	 * @brief Constructor
	 *
	 * @param  output  The output filename
	 */
	TimeSeriesGenerator(string output);

	/**
	 * @brief Destructor
	 */
	~TimeSeriesGenerator();


	/**
	 * @brief Write a new MODCOD in the simulation file
	 *
	 * @param The list of ST FMT
	 * @return true on success, false otherwise
	 */
	bool add(const ListStFmt *const sts);

};


#endif

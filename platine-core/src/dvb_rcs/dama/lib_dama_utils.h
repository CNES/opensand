/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file lib_dama_utils.h
 * @brief Utilities definitions and functions for DAMA
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef LIB_DAMA_UTILS_H
#define LIB_DAMA_UTILS_H

#ifndef MIN
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif


/**
 * @class DU_Converter
 * @brief class managing unit conversion between kbits/s, cells per frame, etc
 */
class DU_Converter
{
 protected:

	int CellSize;      ///< UL ATM cells size, in bytes
	int FrameDuration; ///< UL frame duration, in ms
	double KbitsToCellsPerSecRatio;
	double KbitsToCellsPerFrameRatio;

 public:

	DU_Converter(int Duration, int Size);
	~DU_Converter();

	double /*kbits/s */ ConvertFromCellsPerSecToKbits(double RateCells);
	double /*cells/s */ ConvertFromKbitsToCellsPerSec(int RateKbits);
	double /*kbits/s */ ConvertFromCellsPerFrameToKbits(double RateCells);
	double /*cells/s */ ConvertFromKbitsToCellsPerFrame(int RateKbits);
};

#endif //LIB_DAMA_UTILS_H

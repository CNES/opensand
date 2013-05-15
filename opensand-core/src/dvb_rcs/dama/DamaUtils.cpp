/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
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
 * @file lib_dama_utils.cpp
 * @brief Utilities definitions and functions for DAMA
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "DamaUtils.h"

/*
 * @param Duration is the Frame duration in ms
 * @param Size is the UL packet size in bytes
 */
DU_Converter::DU_Converter(int Duration, int Size)
{
	CellSize = Size;
	FrameDuration = Duration;
	KbitsToCellsPerSecRatio = 1000.0 / (double) (CellSize * 8);
	KbitsToCellsPerFrameRatio = (double) Duration / (double) (CellSize * 8);
}

DU_Converter::~DU_Converter()
{
}

/**
 * Conversion of rate from kbits/s to cells/sec
 *
 * @param RateKbits in kbits/s
 * @return rate in cells/s
 */
double DU_Converter::ConvertFromKbitsToCellsPerSec(int RateKbits)
{
	return (RateKbits * KbitsToCellsPerSecRatio);
}


/**
 * Conversion of rate from cells/sec to  kbits/s
 *
 * @param RateCells in cells/s
 * @return rate in kbits/s
 */
double DU_Converter::ConvertFromCellsPerSecToKbits(double RateCells)
{
	return (RateCells / KbitsToCellsPerSecRatio);
}


/**
 * Conversion of rate from kbits/s to cells/frame
 *
 * @param RateKbits in kbits/s
 * @return rate in cells/frame
 */
double DU_Converter::ConvertFromKbitsToCellsPerFrame(int RateKbits)
{
	return (RateKbits * KbitsToCellsPerFrameRatio);
}


/**
 * Conversion of rate from cells/frame to  kbits/s
 *
 * @param RateCells in cells/frame
 * @return rate in kbits/s
 */
double DU_Converter::ConvertFromCellsPerFrameToKbits(double RateCells)
{
	return (RateCells / KbitsToCellsPerFrameRatio);
}


vol_b_t DU_Converter::pktToBits(vol_pkt_t vol_pkt)
{
	return vol_pkt * CellSize * 1000; // TODO try to get CellSize in bit instead
}

/**
 * @file lib_dama_utils.cpp
 * @brief Utilities definitions and functions for DAMA
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#include "lib_dama_utils.h"

/*
 * @param Duration is the Frame duration in ms
 * @param Size is the UL ATM cell size in bytes
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


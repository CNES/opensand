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

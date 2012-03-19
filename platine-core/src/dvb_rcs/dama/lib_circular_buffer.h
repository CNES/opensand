/**
 * @file lib_circular_buffer.h
 * @brief This is a circular buffer class.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef _CIRCULAR_BUFFER_FUNC_HEADER
#define _CIRCULAR_BUFFER_FUNC_HEADER

#include "lib_dama_utils.h"


/**
 * @class CircularBuffer
 * @brief Manage a circular buffer with >= 1 elem, or a buffer saving only
 *        last value
 */
class CircularBuffer
{
 private:

	/// if size = 0 --> flag = true --> only last value is saved, sum = 0
	bool m_SaveOnlyLastValue;

	int m_Size;      ///< circular buffer max size
	int m_Index;     ///< current index
	int m_NbValues;  ///< current nb of elem
	double *m_Value; ///< circular buffer array
	double m_Min;    ///< min value contained in the circular buffer
	double m_Sum;    ///< sum of all values contained in the circular buffer

 public:

	CircularBuffer(int size);
	virtual ~CircularBuffer();

	void Update(double newValue);
	double GetLastValue();
	double GetPreviousValue();
	double GetMean();
	double GetMin();
	double GetSum();
	void Debug();
};

#endif

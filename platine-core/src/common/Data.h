/**
 * @file Data.h
 * @brief A set of data for network packets
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef DATA_H
#define DATA_H

#include <string>


/**
 * @class Data
 * @brief A set of data for network packets
 */
class Data: public std::basic_string<unsigned char>
{
 public:

	/**
	 * Create an empty set of data
	 */
	Data();

	/**
	 * Create a set of data from a string of unsigned characters
	 *
	 * @param string  the string of unsigned characters
	 */
	Data(std::basic_string<unsigned char> string);

	/**
	 * Create a set of data from unsigned characters
	 *
	 * @param data  the unsigned characters to copy
	 * @param len   the number of unsigned characters to copy
	 */
	Data(unsigned char *data, unsigned int len);

	/**
	 * Create a set of data from a subset of data
	 *
	 * @param data  the set of data to copy data from
	 * @param pos   the index of first byte to copy from
	 * @param len   the number of bytes to copy
	 */
	Data(Data data, unsigned int pos, unsigned int len);

};

#endif


/**
 * @file ModulationType.h
 * @brief The different types of modulation for MODCOD or DRA schemes
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef MODULATION_TYPE_H
#define MODULATION_TYPE_H


/**
 * @brief The different types of modulations accepted for MODCOD or DRA schemes
 */
typedef enum
{
	MODULATION_UNKNOWN,  /**< An unknown modulation */
	MODULATION_BPSK,     /**< The BPSK modulation */
	MODULATION_QPSK,     /**< The QPSK modulation */
	MODULATION_8PSK,     /**< The 8PSK modulation */
	/* add new modulations here */
} modulation_type_t;


#endif


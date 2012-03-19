/**
 * @file SatelliteTerminal.cpp
 * @brief The internal representation of a Satellite Terminal (ST)
 * @author Didier Barvaux / Viveris Technologies
 */

#include "SatelliteTerminal.h"


/**
 * @brief Create an internal representation of a Satellite Terminal (ST)
 *
 * @param id               the ID of the ST (called TAL ID or MAC ID elsewhere
 *                         in the code)
 * @param simu_column_num  the column # associated to the ST for DRA/MODCOD
 *                         simulation files
 * @param modcod_id        the initial MODCOD ID of the ST
 * @param dra_scheme_id    the initial DRA scheme ID of the ST
 */
SatelliteTerminal::SatelliteTerminal(long id,
                                     unsigned long simu_column_num,
                                     unsigned int modcod_id,
                                     unsigned int dra_scheme_id)
{
	this->id = id;
	this->simu_column_num = simu_column_num;
	this->current_modcod_id = modcod_id;
	this->previous_modcod_id = this->current_modcod_id;
	this->current_dra_scheme_id = dra_scheme_id;

	// force the advertising of MODCOD ID at startup
	this->is_current_modcod_advertised = false;
}


/**
 * @brief Destroy an internal representation of a Satellite Terminal (ST)
 */
SatelliteTerminal::~SatelliteTerminal()
{
	// nothing particular to do
}


/**
 * @brief Get the ID of the ST
 *
 * The ID of the ST is often called TAL ID or MAC ID elsewhere in the code
 *
 * @return  the ID of the ST
 */
long SatelliteTerminal::getId()
{
	return this->id;
}


/**
 * @brief Get the column # associated to the ST for DRA/MODCOD simulation files
 *
 * @return  the column number for DRA/MODCOD simulation files
 */
unsigned long SatelliteTerminal::getSimuColumnNum()
{
	return this->simu_column_num;
}


/**
 * @brief Get the current MODCOD ID of the ST
 *
 * @return  the current MODCOD ID of the ST
 */
unsigned int SatelliteTerminal::getCurrentModcodId()
{
	return this->current_modcod_id;
}


/**
 * @brief Update the MODCOD ID of the ST
 *
 * @param new_id  the new MODCOD ID of the ST
 */
void SatelliteTerminal::updateModcodId(unsigned int new_id)
{
	this->previous_modcod_id = this->current_modcod_id;
	this->current_modcod_id = new_id;

	// mark the MODCOD as not advertised yet if the MODCOD changed
	if(this->current_modcod_id != this->previous_modcod_id)
	{
		this->is_current_modcod_advertised = false;
	}
}


/**
 * @brief Get the previous MODCOD ID of the ST
 *
 * @return  the previous MODCOD ID of the ST
 */
unsigned int SatelliteTerminal::getPreviousModcodId()
{
	return this->previous_modcod_id;
}


/**
 * @brief Was the current MODCOD ID advertised to the ST ?
 *
 * @return  true if the current MODCOD ID was already advertised to the ST,
 *          false if it was not advertised yet
 */
bool SatelliteTerminal::isCurrentModcodAdvertised()
{
	return this->is_current_modcod_advertised;
}


/**
 * @brief Get the current DRA scheme ID of the ST
 *
 * @return  the current DRA scheme ID of the ST
 */
unsigned int SatelliteTerminal::getCurrentDraSchemeId()
{
	return this->current_dra_scheme_id;
}


/**
 * @brief Update the DRA scheme ID of the ST
 *
 * @param new_id  the new DRA scheme ID of the ST
 */
void SatelliteTerminal::updateDraSchemeId(unsigned int new_id)
{
	this->current_dra_scheme_id = new_id;
}


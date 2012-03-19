/*
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
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file ConfigurationFile.cpp
 * @brief Reading parameters from a configuration file
 * @author Viveris Technologies
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "uti_debug.h" // use default package
#include "ConfigurationFile.h"

ConfigurationFile globalConfig;


/**
 * Ctor
 */
ConfigurationFile::ConfigurationFile()
{
	// uncomment to get debug messages during file loading
	// (because levels haven't yet been read from config file)
	//UTI_DEBUG_LEVEL(2);
}


/**
 * dtor = free association table
 */
ConfigurationFile::~ConfigurationFile()
{
	unloadConfig();
}



/**
 * Load the whole configuration file content into memory
 * @param confFile path and name of the configuration file
 * @return  0 ok, -1 failed
 */
int ConfigurationFile::loadConfig(const string confFile)
{
	struct stat statFile;
	entries_t *entries = NULL;
	string section, key, value;
	char *buffer = NULL;
	char *ptrBuffer;
	char **pptrBuffer;
	int confFd = -1;
	int ret = -1;

	// Build whole path and name of configuration file
	if(confFile.empty())
	{
		UTI_ERROR("Configuration filename is empty\n");
		goto FCT_END;
	}

	if(access(confFile.c_str(), R_OK) < 0)
	{
		UTI_ERROR("unable to access configuration file '%s' (%s)\n",
				  confFile.c_str(), strerror(errno));
		goto FCT_END;
	}

	//----- Load file content into a huge buffer
	if(stat(confFile.c_str(), &statFile))
	{
		UTI_ERROR("System call stat failure on file [%s], errno %d\n",
		          confFile.c_str(), errno);
		goto FCT_END;
	}

	buffer = new char[statFile.st_size+1];
	if(!buffer)
	{
		UTI_ERROR("System call malloc failure, errno %d\n", errno);
		goto FCT_END;
	}

	if((confFd = open ( confFile.c_str(), O_RDONLY, 0)) < 0)
	{
		UTI_ERROR("System call open failure on file [%s], errno %d\n",
		          confFile.c_str(), errno);
		goto FCT_END;
	}

	memset(buffer, 0, statFile.st_size+1);
	if(read(confFd, buffer,  statFile.st_size) < 0)
	{
		UTI_ERROR("System call read failure on file [%s], errno %d\n",
		          confFile.c_str(), errno);
		goto FCT_END;
	}

	//----- Parse configuration parameters
	ptrBuffer = buffer;
	pptrBuffer = &ptrBuffer;
	while(readLine(pptrBuffer, section, key, value) != -1)
	{
		if(section.size()) // section not null -> new section
		{
			UTI_DEBUG("section [%s]\n", section.c_str());

			entries = new entries_t;
			if(!entries)
				goto FCT_END;

			sectionEntries[section] = entries;
			entries->nbListItems = 0;
		}
		else if(entries != NULL) // inside a section
		{
			UTI_DEBUG("key [%s], value [%s]\n", key.c_str(), value.c_str());

			if(key.size()) // (key, value) pair
			{
				entries->keyItems[key] = value;
			}
			else if(value.size()) // list item
			{
				entries->nbListItems++;
				entries->listItems[entries->nbListItems] = value;
			}
		}
		else
		{
			UTI_DEBUG("Key %s ignored: out of section\n", key.c_str());
		}
	} // while
	ret = 0;

FCT_END:
	if(buffer != NULL)
		delete [] buffer;
	if(confFd != -1) close(confFd);
		return ret;

} // loadConfig


/**
 * Unload the whole configuration file content from memory,
 * i.e. free tables
 */
void ConfigurationFile::unloadConfig()
{
	sectionEntries_t::iterator iterS;
	entries_t *entries;

	for(iterS = sectionEntries.begin(); iterS != sectionEntries.end(); ++iterS)
	{
		entries = (*iterS).second;
		if(entries != NULL)
		{
			entries->keyItems.clear();
			entries->listItems.clear();
			delete entries;
		}
	}
	this->sectionEntries.clear();
} // unloadConfig



/**
 * Read a string value from configuration with format keyName=strValue
 *
 * @param  section  name of the section (without [ ])
 * @param  key      name of the key
 * @param  value    value of the string
 * @return  0 ok, -1 failed
 */
int ConfigurationFile::getStringValue(const char *section,
                                      const char *key,
                                      string &value)
{
	entries_t *entries;
	string _section=section, _key=key;

	if(sectionEntries.count(_section))
	{
		entries = sectionEntries[_section];
		if(entries->keyItems.count(_key))
		{
			value = entries->keyItems[_key];
			return 0;
		}
		else
		{
			UTI_DEBUG("Key %s not found in section %s\n", key, section);
			return -1;
		}
	}
	else
	{
		UTI_DEBUG("Section %s not found\n", section);
		return -1;
	}
} // getStringValue


/**
 * Read an integer value from configuration with format keyName=IntValue
 *
 * @param  section  name of the section (without [ ])
 * @param  key      name of the key
 * @param  value    integer value
 * @return  0 ok, -1 failed
 */
int ConfigurationFile::getIntegerValue(const char *section,
                                       const char *key,
                                       int &value)
{
	string valueStr;

	if((getStringValue(section, key, valueStr) == 0) &&
	   (valueStr.size() > 0))
	{
		value = atoi(valueStr.c_str());
		return 0;
	}
	else
		return -1;
} // getIntegerValue


/**
 * Read a long integer value from configuration with format keyName=LongIntValue
 *
 * @param  section  name of the section (without [ ])
 * @param  key      name of the key
 * @param  value    integer value
 * @return  0 ok, -1 failed
 */
int ConfigurationFile::getLongIntegerValue(const char *section,
                                           const char *key,
                                           long &value)
{
	string valueStr;

	if((this->getStringValue(section, key, valueStr) == 0) &&
	   (valueStr.size() > 0))
	{
		value = atol(valueStr.c_str());
		return 0;
	}
	else
	{
		return -1;
	}
}


/**
 * Read the number of lines in the items list of a section
 *
 * @param  section  name of the section (without [ ])
 * @return  nb list items (-1 if section not found)
 */
int ConfigurationFile::getNbListItems(const char *section)
{
	entries_t *entries;
	string _section=section;

	if(sectionEntries.count(_section))
	{
		entries = sectionEntries[_section];
		return entries->nbListItems;
	}
	else
		return -1;
} // getNbListItems



/**
 * Read an item from the items list of a section
 *
 * @param  section   name of the section (without [ ])
 * @param  itemIdx   item index (from 1 to nbListItems)
 * @param  lineValue value of the whole line
 * @return  0 ok, -1 failed
 */
int ConfigurationFile::getListItem(const char *section,
                                   unsigned short itemIdx,
                                   string &lineValue)
{
	entries_t *entries;
	string _section=section;

	if(sectionEntries.count(_section))
	{
		entries = sectionEntries[_section];
		if(itemIdx > 0 && itemIdx <= entries->nbListItems)
		{
			lineValue = entries->listItems[itemIdx];
			return 0;
		}
		else
		{
			UTI_DEBUG("List item index %d not found in section %s\n", itemIdx, section);
			return -1;
		}
	}
	else
	{
		UTI_DEBUG("Section %s not found\n", section);
		return -1;
	}
}


/**
 * Suppress spaces and tabulations at beginning and end of a string
 *
 * @param  str  string to process
 * @return  length of the resulting string
 */
int ConfigurationFile::supprSpaces(char *str)
{
	int i, j;
	int lgmax;  // length of source string

	if(str == NULL)
		return 0;

	lgmax = strlen(str);
	if(lgmax == 0)
		return 0;

	// At string beginning
	i = 0;
	j = 0;
	// find first character different from space & tab
	while(isspace(str[i]) || (str[i] == '\t'))
	{
		i++;
	}
	// move the string left
	while(i <= lgmax)
	{
		str[j++] = str[i++];
	}

	// At string end
	i = strlen(str) - 1 ;
	while(i >= 0 && ((isspace(str[i]) || (str[i] == '\t'))))
	{
		str[i--] = '\0';
	}

	return i + 1;
} // supprSpaces



/**
 * Read a line from file and split it into (key, value) or lineItem
 *   if comment or blank line, ignore
 *
 * @param  ptrBuffer pointer to the current character in the config buffer
 * @param  section   section name if section line, else empty
 * @param  key       key name if pair (key, value), else empty
 * @param  value     value of a pair (key,value) or value of a lineItem
 * @return  0 ok, -1 failed
 */
int ConfigurationFile::readLine(char **ptrBuffer,
                                string &section,
                                string &key,
                                string &value)
{
	char *line ;
	int iChar, lg;
	int current ;

	line = new char[CONF_LINE_MAX];
	if(!line)
		return -1;
	section = key = "";

	//printf("buf %50.50s\n", *ptrBuffer);

	do
	{
		// Read next line
		current = 0 ;
		while((**ptrBuffer) && (current < CONF_LINE_MAX-1))
		{
			iChar = **ptrBuffer;
			(*ptrBuffer)++;

			if(iChar == '\n')
				break;

			line[current++] = (char) iChar ;
		}

		line[current] = '\0';
		// remove spaces/tabs at beginning & end of strings
		lg = supprSpaces(line);

		//----- Extract section or key from the line
		// Comment
		if((lg == 0) || (line[0] == CONF_COMMENT))
		{
			continue;
		}
		// Section
		else if((line[0] == CONF_SECTION_BEGIN) &&
		       (line[lg-1] == CONF_SECTION_END))
		{
			line[lg-1] = 0;
			section = line + 1;
			break;
		}
		else
		{
			char *valuePtr;

			valuePtr = strrchr(line, CONF_AFFECTATION);
			if(valuePtr) // (key,value) pair
			{
				line[valuePtr-line] = 0;
				supprSpaces(line);
				key = line;
				valuePtr++;
			}
			else // line item
			{
				valuePtr = line;
			}

			// remove spaces/tabs at beginning & end of strings
			supprSpaces(valuePtr);
			value = valuePtr;
			break;
		}
	} while(**ptrBuffer);

	delete [] line;
	if(current > 0)
		return 0 ;
	else
		return -1 ;
} // readLine

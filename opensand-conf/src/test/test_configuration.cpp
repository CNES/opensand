/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file     src/test.cpp
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief    Test the configuration parsing
 */

#include "Configuration.h"

#include <opensand_output/Output.h>

#include <fstream>
#include <cstring>

using namespace std;

#define COMP_FILE "comparison"


/**
 * @brief Print usage of the test application
 */
static void usage(void)
{
	cerr << "Configuration test: test the opensand configuration library" << endl
	     << "usage: configuration_test [OPTIONS]" << endl
	     << "with:" << endl
	     << "options" << endl
	     << "   -i                 Input file (may be used more than once)" << endl
	     << "   -r                 Result file" << endl;
}


int main(int argc, char **argv)
{
	int failure = 1;
	int lcount;
	string value;
	int args_used;
	vector<string> conf_files;

	// sections, keys map
	map<string, vector<string> > config;
	map<string, vector<string> > config_spot;
	map<string, ConfigurationList >::iterator iter;
	// section, keys map for lists
	map<pair<string, string>, vector<string> > config_list;
	map<pair<pair<string, string>, string>, vector<string> > config_spot_list;
	map<pair<string, string>, vector<string> >::iterator iter_list;
	map<pair<pair<string, string>, string>, vector<string> >::iterator iter_spot_list;
	vector<string> vec;
	vector<string>::iterator vec_it;
	vector<string>::iterator vec_spot_it;
	ConfigurationList section;
	string sec_name;
	string spot = "spot";
	ofstream comp_ofile(COMP_FILE);
	ifstream res_file;
	ifstream comp_ifile;
	vector<string> input_files;
	string result_filename;
   
	/* parse program arguments, print the help message in case of failure */
	if(argc <= 1)
	{
		usage();
		goto error;
	}

	for(argc--, argv++; argc > 0; argc -= args_used, argv += args_used)
	{
		args_used = 1;

		if(!strcmp(*argv, "-h"))
		{
			/* print help */
			usage();
			goto error;
		}
		else if(!strcmp(*argv, "-i"))
		{
			/* get the name of the file where the configuration is stored */
			input_files.push_back(argv[1]);
			cout << "%%%%%%%%%% input files : " << argv[1] << endl;
			args_used++;
		}
		else if(!strcmp(*argv, "-r"))
		{
			/* get the name of the file where the configuration is stored */
			result_filename = argv[1];
			args_used++;
		}
		else
		{
			usage();
			goto error;
		}
	}

	res_file.open(result_filename.c_str(), ifstream::in);
	if(!res_file.is_open())
	{
		cerr << "cannot open result file: " << result_filename << endl;
		goto close;
	}

    // load the output file for comparison
    if(!comp_ofile || !comp_ofile.is_open())
    {
        cerr << "cannot open comparison file: " << COMP_FILE << endl;
        goto close;
    }

    vec.push_back("s1key1");
    vec.push_back("s1key2");
    // load the configuration to check
    config["section1"] = vec;
    vec.clear();
    vec.push_back("s2key1");
    config["section2"] = vec;
    vec.clear();
    vec.push_back("s3key1");
    config["section3"] = vec;
    vec.clear();
    vec.push_back(spot);
    config["section4"] = vec;
    vec.clear();
    vec.push_back("s4att1");
    vec.push_back("s4att2");
    config_spot_list[make_pair(make_pair("section4", spot), "s4tables")] = vec;
	vec.clear();
	vec.push_back("s4key1");
	config_spot[spot] = vec;
    vec.clear();
    vec.push_back("s1att1");
    vec.push_back("s1att2");
    config_list[make_pair("section1", "s1tables")] = vec;
    vec.clear();
    vec.push_back("s3att1");
    vec.push_back("s3att2");
    config_list[make_pair("section3", "s3tables")] = vec;
    // duplicated section
    vec.clear();
    vec.push_back("dupkey1");
    vec.push_back("dupkey2");
    config["dup"] = vec;

    Output::enableStdlog();
    Output::init(true);
    Output::finishInit();

    // load the configuration files
    // be careful the maps are ordered, the output will not be ordered like above
    if(!Conf::loadConfig(input_files))
    {
        cerr << "cannot load configuration files" << endl;
        goto close;
    }

    // get sections keys in configuration file
    for(iter = Conf::section_map.begin(); iter != Conf::section_map.end(); iter++)
    {
        section = (*iter).second;
        sec_name = (*iter).first;

		for(vec_it = config[sec_name].begin(); vec_it != config[sec_name].end(); vec_it++)
		{
			if(!strcmp((*vec_it).c_str(), spot.c_str()))
			{
				ConfigurationList spot_list;
				if(!Conf::getListNode(section, spot.c_str(), spot_list))
				{
					cerr << "cannot get spot for section " << (*iter).first << endl;
				}
				
				for(vec_spot_it = config_spot[spot].begin(); 
				    vec_spot_it != config_spot[spot].end(); vec_spot_it++)
				{
					if(!Conf::getValue(spot_list, (*vec_spot_it).c_str(), value))
					{
						cerr << "cannot get the value for section '" << (*iter).first
							<< "', key '" << (*vec_spot_it) << "'" << endl;
						goto close;
					}
					comp_ofile << (*vec_spot_it) << "=" << value << endl;
					cout << "got value '" << value << "' for section '" << sec_name
						<< "', key '" << (*vec_spot_it) << "'" << endl;
				}
			}
			else
			{
				 if(!Conf::getValue(section, (*vec_it).c_str(), value))
				 {
					 cerr << "cannot get the value for section '" << (*iter).first
						 << "', key '" << (*vec_it) << "'" << endl;
					 goto close;
				 }
				 comp_ofile << (*vec_it) << "=" << value << endl;
				 cout << "got value '" << value << "' for section '" << sec_name
					 << "', key '" << (*vec_it) << "'" << endl;
			}
		}
	}
    comp_ofile << endl;

    // get sections lists in configuration files
    for(iter_list = config_list.begin();
        iter_list != config_list.end(); iter_list++)
    {
        ConfigurationList list;
        ConfigurationList::iterator line;
        pair<string, string> table = (*iter_list).first;

        if(!Conf::getListItems(Conf::section_map[table.first.c_str()],
                               table.second.c_str(),
                               list))
        {
            cerr << "cannot get the items list for section '" << table.first
                 << "' key '" << table.second << "'" << endl;
            goto close;
        }
        for(line = list.begin(); line != list.end(); line++)
        {
            vec = (*iter_list).second;
            for(vec_it = vec.begin(); vec_it != vec.end(); vec_it++)
            {
                if(!Conf::getAttributeValue(line, (*vec_it).c_str(),
                                            value))
                {
                    cerr << "cannot get the vec_itribute '" << (*vec_it)
                         << "' for section '" << table.first << "', key '"
                         << table.second << "'" << endl;
                    goto close;
                }
                comp_ofile << (*vec_it) << "=" << value << " ";
                cout << "got value '" << value << "' for attribute "
                    << "'"<< (*vec_it).c_str() << "' at section '" 
                    << table.first << "', key '" << table.second << "'" << endl;
            }
            comp_ofile << endl;
        }
    }
    
    for(iter_spot_list = config_spot_list.begin();
        iter_spot_list != config_spot_list.end(); iter_spot_list++)
    {
        ConfigurationList list;
        ConfigurationList::iterator line;
        pair<pair<string, string>, string> table = (*iter_spot_list).first;
        ConfigurationList spot_list;
		
		if(!Conf::getListNode(Conf::section_map[table.first.first], 
                              table.first.second.c_str(), spot_list)) 
		{
			cerr << "cannot get spot for section " << table.first.first << endl;
		}
		
		if(!Conf::getListItems(spot_list,
                               table.second.c_str(),
                               list))
        {
            cerr << "cannot get the items list for section '" << table.first.first
                 << "' key '" << table.second << "'" << endl;
            goto close;
        }

        for(line = list.begin(); line != list.end(); line++)
        {
            vec = (*iter_spot_list).second;
            for(vec_it = vec.begin(); vec_it != vec.end(); vec_it++)
            {
                if(!Conf::getAttributeValue(line, (*vec_it).c_str(),
                                            value))
                {
                    cerr << "cannot get the vec_attribute '" << (*vec_it)
                         << "' for section '" << table.first.first << "'spot' ', key '"
                         << table.second << "'" << endl;
                    goto close;
                }
                comp_ofile << (*vec_it) << "=" << value << " ";
                cout << "got value '" << value << "' for attribute "
                    << "'"<< (*vec_it).c_str() << "' at section '" 
                    << table.first.first << "', key '" << table.second << "'" << endl;
            }
            comp_ofile << endl;
        }
    }
    failure = 0;
    
    // compare the two files
    comp_ofile.flush();
    comp_ofile.close();
    comp_ifile.open(COMP_FILE);
    lcount = 0;
    while(!comp_ifile.eof() && !res_file.eof())
    {
        char res[256];
        char comp[256];

        res_file.getline(res, 256);
        comp_ifile.getline(comp, 256);

        lcount++;
        if(strncmp(res, comp, 256))
        {
            cerr << "line " << lcount << " differs in file comparison: " << endl
                 << "expected: '" << res << "'" << endl
                 << "obtained: '" << comp<< "'" << endl;
            failure = 1;
            goto close;
        }
    }
    if(comp_ifile.eof() != res_file.eof())
    {
        cerr << "files have different size" << endl;
        failure = 1;
        goto close;
    }

close:
    if(res_file && res_file.is_open())
    {
        res_file.close();
    }
    if(comp_ofile && comp_ofile.is_open())
    {
        comp_ofile.close();
    }
    if(comp_ifile && comp_ifile.is_open())
    {
        comp_ifile.close();
    }
error:
    remove(COMP_FILE);
    return failure;
}

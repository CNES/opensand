/**
 * @file     src/test.cpp
 * @author   Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief    Test the configuration parsing
 *
 */

#include "ConfigurationFile.h"

#include <fstream>
#include <cstring>

using namespace std;

#define COMP_FILE "comparison"


/**
 * @brief Print usage of the test application
 */
static void usage(void)
{
	cerr << "Configuration test: test the platine configuration library" << endl
	     << "usage: configuration_test [OPTIONS]" << endl
	     << "with:" << endl
	     << "options" << endl
	     << "   -i                 Input file (may be used more than once" << endl
	     << "   -r                 Result file" << endl;
}


int main(int argc, char **argv)
{
    int failure = 1;
    int lcount;
    string value;
    int args_used;

    // sections, keys map
    map<string, vector<string> > config;
    map<string, vector<string> >::iterator iter;
    // section, keys map for lists
    map<pair<string, string>, vector<string> > config_list;
    map<pair<string, string>, vector<string> >::iterator iter_list;
    vector<string>::iterator vec_it;
    vector<string> vec;
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

    // be careful the maps are ordered, the output will not be ordered like above

    // load the configuration files
    for(vector<string>::const_iterator it = input_files.begin();
        it != input_files.end();
        ++it)
    {
    	if(!globalConfig.loadConfig(*it))
		{
			cerr << "cannot load '" << *it << "' configuration file" << endl;
			goto close;
		}
	}

    // get the values in configuration file
    for(iter = config.begin(); iter != config.end(); iter++)
    {
        vec = (*iter).second;

        for(vec_it = vec.begin(); vec_it != vec.end(); vec_it++)
        {
            if(!globalConfig.getValue((*iter).first.c_str(),
                                      (*vec_it).c_str(), value))
            {
                cerr << "cannot get the value for section '" << (*iter).first
                    << "', key '" << (*vec_it) << "'" << endl;
                goto unload;
            }
            comp_ofile << (*vec_it) << "=" << value << endl;
            cout << "got value '" << value << "' for section '" << (*iter).first
                << "', key '" << (*vec_it) << "'" << endl;
        }
    }
    comp_ofile << endl;

    // get the lists in configuration files
    for(iter_list = config_list.begin();
        iter_list != config_list.end(); iter_list++)
    {
        ConfigurationList list;
        ConfigurationList::iterator line;
        pair<string, string> table = (*iter_list).first;

        if(!globalConfig.getListItems(table.first.c_str(),
                                      table.second.c_str(), list))
        {
            cerr << "cannot get the items list for section '" << table.first
                 << "' key '" << table.second << "'" << endl;
            goto unload;
        }
        for(line = list.begin(); line != list.end(); line++)
        {
            vec = (*iter_list).second;
            for(vec_it = vec.begin(); vec_it != vec.end(); vec_it++)
            {
                if(!globalConfig.getAttributeValue(line, (*vec_it).c_str(),
                                                   value))
                {
                    cerr << "cannot get the vec_itribute '" << (*vec_it)
                         << "' for section '" << table.first << "', key '"
                         << table.second << "'" << endl;
                    goto unload;
                }
                comp_ofile << (*vec_it) << "=" << value << " ";
                cout << "got value '" << value << "' for attribute "
                    << "'s1att1' at section 'section1', key 's1tables'" << endl;
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
            goto unload;
        }
    }
    if(comp_ifile.eof() != res_file.eof())
    {
        cerr << "files have different size" << endl;
        failure = 1;
        goto unload;
    }

unload:
    globalConfig.unloadConfig();
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

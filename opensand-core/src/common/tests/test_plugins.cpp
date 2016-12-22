/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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

/*
 * Simple test application for encapsulation plugins
 *
 * The application takes a flow of LAN packets as input, encapsulates
 * the LAN packets and then desencapsulates them to get the LAN packets back.
 *
 * The application outputs encapsulated and LAN packets into PCAP files
 * given as arguments. The PCAP files can be compared with references.
 *
 * Launch the application with -h to learn how to use it.
 *
 * Author: Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * Author: Julien Bernard <julien.bernard@toulouse.viveris.com>
 * Author: Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

// OpenSAND includes
#include "StackPlugin.h"
#include "EncapPlugin.h"
#include "LanAdaptationPlugin.h"
#include "IpAddress.h"
#include "Ipv4Address.h"
#include "Ipv6Address.h"
#include "Plugin.h"

#include <opensand_output/Output.h>

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sstream>
#include <net/ethernet.h>
#include <algorithm>
#include <arpa/inet.h>

// include for the PCAP library
#include <pcap.h>

using namespace std;

/// The length of the Linux Cooked Sockets header
#define LINUX_COOKED_HDR_LEN  16

/// The program version
#define VERSION   "Encapsulation plugins test application, version 0.1\n"

/// The program usage
#define USAGE \
"Encapsulation plugins test application: test the encapsulation plugins with a flow of LAN packets\n\n\
usage: test [-h] [-v] [-d level] [-o] [-f folder] flow\n\
  -h        print this usage and exit\n\
  -v        print version information and exit\n\
  -d level  print debug information\n\
               - 0 error only\n\
               - 1 debug messages\n\
               - 2 dump all encapsulated packets\n\
  -o        save the generated encapsulated packets for each encapsulation scheme \n\
            instead of comparing them (PCAP format)\n\
  -f folder the folder where the files will be read/written (default: '.')\n\
  flow      flow of Ethernet frames to encapsulate (PCAP format)\n\n"


static unsigned int verbose;

/** DEBUG macro */
#define INFO(format, ...) \
	do { \
		if(verbose) \
			printf(format, ##__VA_ARGS__); \
	} while(0)


#define DEBUG(format, ...) \
	do { \
		if(verbose > 1) \
			printf(format, ##__VA_ARGS__); \
	} while(0)

#define DEBUG_L2(format, ...) \
	do { \
		if(verbose > 2) \
			printf(format, ##__VA_ARGS__); \
	} while(0)

#define ERROR(format, ...) \
	do { \
		fprintf(stderr, format, ##__VA_ARGS__); \
	} while(0)


static bool compare_packets(const unsigned char *pkt1, int pkt1_size,
                            const unsigned char *pkt2, int pkt2_size);
static bool open_pcap(string filename, pcap_t **handle,
                      uint32_t &link_len);
static bool test_iter(string src_filename, string encap_filename,
                       bool compare, string name,
                       lan_contexts_t lan_contexts,
                       encap_contexts_t encap_contexts);
static bool test_lan_adapt(string src_filename,
                           string folder,
                           bool compare);
static void test_encap_and_decap(
	LanAdaptationPlugin::LanAdaptationPacketHandler *pkt_hdl,
	lan_contexts_t lan_contexts,
	vector<string> &failure,
	string src_filename,
	string folder,
	bool compare);


int main(int argc, char *argv[])
{
	int status = 1;
	string src_filename = "";
	string folder = "./";
	string base_protocol = "IP";
	bool compare = true;
	int args_used;

	if(argc <= 1)
	{
		ERROR(USAGE);
		goto quit;
	}

	for(argc--, argv++; argc > 0; argc -= args_used, argv += args_used)
	{
		args_used = 1;

		if(!strcmp(*argv, "-v"))
		{
			// print version
			ERROR(VERSION);
			goto quit;
		}
		else if(!strcmp(*argv, "-h"))
		{
			// print help
			ERROR(USAGE);
			goto quit;
		}
		else if(!strcmp(*argv, "-o"))
		{
			// get the name of the file to store encapsulated packets
			compare = false;
		}
		else if(!strcmp(*argv, "-f"))
		{
			// get the name of the file to store the LAN packets
			folder = argv[1];
			args_used++;
		}
		else if(!strcmp(*argv, "-d"))
		{
			// get the name of the file to store the LAN packets
			verbose = atoi(argv[1]);
			args_used++;
		}
		else if(src_filename.empty())
		{
			// get the name of the file that contains the packets to
			// encapsulate/desencapsulate
			src_filename = argv[0];
		}
		else
		{
			// do not accept more than one filename without option name
			ERROR(USAGE);
			goto quit;
		}
	}

	// the filenames are mandatory
	if(src_filename.empty())
	{
		ERROR(USAGE);
		goto quit;
	}

	if(test_lan_adapt(src_filename, folder, compare))
	{
		// success
		status = 0;
	}

quit:
	return status;
}

static bool test_lan_adapt(string src_filename,
                           string folder,
                           bool compare)
{
	pl_list_t lan_plug;
	pl_list_it_t plugit;
	vector<string> failure;
	unsigned int nbr_tests = 0;
	bool success = false;

	Output::init(false);
	Output::enableStdlog();

	// load the plugins
	if(!Plugin::loadPlugins(false, string("/etc/opensand/plugins/")))
	{
		ERROR("cannot load the plugins\n");
		goto error;
	}

	Plugin::getAllLanAdaptationPlugins(lan_plug);

	// test each lan adaptation plugin
	for(plugit = lan_plug.begin(); plugit != lan_plug.end(); ++plugit)
	{
		lan_contexts_t contexts;
		string name = plugit->first;
		string name_low = name;
		transform(name.begin(), name.end(),
		          name_low.begin(), ::tolower);
		LanAdaptationPlugin *plugin;
		LanAdaptationPlugin::LanAdaptationContext *context;
		LanAdaptationPlugin::LanAdaptationPacketHandler *pkt_hdl;
		int found;

		// LanAdaptationContext initialisation
		SarpTable sarp_table;
		IpAddress *ip_addr;
		IpAddress *ip6_addr;
		MacAddress *src_mac;
		MacAddress *dst_mac;

		found = name_low.find("/");
		while(found != (signed)string::npos)
		{
			name_low.replace(found, 1, "_");
			found = name_low.find("/", found);
		}

		if(!Plugin::getLanAdaptationPlugin(name, &plugin))
		{
			ERROR("failed to initialize plugin %s\n", name.c_str());
			failure.push_back(name.c_str());
			continue;
		}
		pkt_hdl = plugin->getPacketHandler();
		context = plugin->getContext();
		if(!context->setUpperPacketHandler(NULL, TRANSPARENT))
		{
			INFO("LAN adaptation plugin %s needs a packet handler, find one\n",
			       name.c_str());

			vector<string> upper = context->getAvailableUpperProto(TRANSPARENT);
			// try to add a supported upper layer
			for(vector<string>::iterator iter = upper.begin();
			    iter != upper.end(); ++iter)
			{
				if(lan_plug[*iter] != NULL)
				{
					LanAdaptationPlugin *up_plugin = NULL;
					if(!Plugin::getLanAdaptationPlugin(*iter, &up_plugin))
					{
						ERROR("failed to initialize upper plugin %s for %s\n",
						      (*iter).c_str(), name.c_str());
						failure.push_back(name.c_str());
						break;
					}
					if(!context->setUpperPacketHandler(
								up_plugin->getPacketHandler(),
								TRANSPARENT))
					{
						ERROR("failed to set upper packet src_handler for "
						        "%s context\n", name.c_str());
						failure.push_back(name.c_str());
						break;
					}
					// set network as upper layer for the new context
					if(!up_plugin->getContext()->setUpperPacketHandler(NULL, TRANSPARENT))
					{
						INFO("%s does not support %s as upper layer either\n",
						      up_plugin->getName().c_str(), pkt_hdl->getName().c_str());
						continue;
					}

					INFO("add %s context over %s\n",
					       up_plugin->getName().c_str(), name.c_str());

					contexts.push_back(up_plugin->getContext());
					break;
				}
			}
			if(contexts.size() == 0)
			{
				ERROR("failed to get an upper layer for %s context\n", name.c_str());
				failure.push_back(name.c_str());
				continue;
			}
		}

		contexts.push_back(context);
		// init contexts
		// both IPAddress will be deleted with SarpTable
		ip_addr = new Ipv4Address("0.0.0.0");
		ip6_addr = new Ipv6Address("0");
		sarp_table.add(ip_addr, 0, 1);
		sarp_table.add(ip6_addr, 0, 1);
		// Add Ethernet entries in SARP table
		// TODO get these value in capture
		// for icmp28 test
		src_mac = new MacAddress("00:B0:D0:C7:C1:9D");
		sarp_table.add(src_mac, 0);
		dst_mac = new MacAddress("00:13:72:32:3d:bc");
		sarp_table.add(dst_mac, 1);
		// for icmp64 test
		src_mac = new MacAddress("00:50:04:2d:f3:30");
		sarp_table.add(src_mac, 0);
		dst_mac = new MacAddress("00:04:76:0B:31:8b");
		sarp_table.add(dst_mac, 1);
		for(lan_contexts_t::iterator ctx = contexts.begin();
		    ctx != contexts.end(); ++ctx)
	    {
			(*ctx)->initLanAdaptationContext(1, 0, TRANSPARENT,
			                                 &sarp_table);
		}

		test_encap_and_decap(pkt_hdl, contexts, failure, src_filename,
		                     folder, compare);
		nbr_tests += 1;
	}
	Plugin::releasePlugins();
	if(nbr_tests == 0)
	{
		ERROR("No adequat plugin found\n");
		success = false;
		goto error;
	}

	if(failure.size() == 0)
	{
		INFO("All tests were successful\n");
		success = true;
	}
	else
	{
		vector<string>::iterator it;
		ERROR("The following tests failed:\n");
		for(it = failure.begin(); it != failure.end(); ++it)
		{
			ERROR("  - %s\n", (*it).c_str());
		}
		success = false;
	}
	Output::finishInit();

error:
	return success;
}


static void test_encap_and_decap(
	LanAdaptationPlugin::LanAdaptationPacketHandler *pkt_hdl,
	lan_contexts_t lan_contexts,
	vector<string> &failure,
	string src_filename,
	string folder,
	bool compare)
{
	pl_list_t encap_plug;
	pl_list_it_t plugit;
	string stack = "";

	for(lan_contexts_t::reverse_iterator ctxit = lan_contexts.rbegin();
        ctxit != lan_contexts.rend(); ++ctxit)
	{
		if(stack.size() > 0)
		{
			stack += "/";
		}
		stack += (*ctxit)->getName();
	}

	Plugin::getAllEncapsulationPlugins(encap_plug);

	// test each encap context
	for(plugit = encap_plug.begin(); plugit != encap_plug.end(); ++plugit)
	{
		encap_contexts_t encap_contexts;
		string name = plugit->first;
		string name_low;
		EncapPlugin *plugin = NULL;
		EncapPlugin::EncapContext *context;
		int found;

		if(!Plugin::getEncapsulationPlugin(name, &plugin))
		{
			ERROR("failed to initialize plugin %s\n", name.c_str());
			if(stack.size() > 0)
			{
				stack += "/";
			}
			stack += name;
			failure.push_back(stack);
			continue;
		}
		context = plugin->getContext();
		if(!context->setUpperPacketHandler(pkt_hdl, TRANSPARENT))
		{

			INFO("cannot set %s as upper layer for %s context, find another one\n",
			       pkt_hdl->getName().c_str(), name.c_str());

			vector<string> upper = context->getAvailableUpperProto(TRANSPARENT);
			// try to add a supported upper layer
			for(vector<string>::iterator iter = upper.begin();
			    iter != upper.end(); ++iter)
			{
				if(encap_plug[*iter] != NULL)
				{
					EncapPlugin *up_plugin;
					if(!Plugin::getEncapsulationPlugin(*iter, &up_plugin))
					{
						ERROR("failed to initialize upper plugin %s for %s\n",
						      (*iter).c_str(), name.c_str());
						if(stack.size() > 0)
						{
							stack += "/";
						}
						stack += name;
						failure.push_back(stack);
						break;
					}
					if(!context->setUpperPacketHandler(
								up_plugin->getPacketHandler(),
								TRANSPARENT))
					{
						ERROR("failed to set upper packet src_handler for "
						        "%s context\n", name.c_str());
						if(stack.size() > 0)
						{
							stack += "/";
						}
						stack += name;
						failure.push_back(stack);
						break;
					}

					// add LAN Adapation packet handler over this context
					if(!up_plugin->getContext()->setUpperPacketHandler(pkt_hdl, TRANSPARENT))
					{
						INFO("%s does not support %s as upper layer either\n",
						      up_plugin->getName().c_str(), pkt_hdl->getName().c_str());
						continue;
					}

					INFO("add %s context over %s\n",
					       up_plugin->getName().c_str(), name.c_str());

					encap_contexts.push_back(up_plugin->getContext());
					break;
				}
			}
			if(encap_contexts.size() == 0)
			{
				ERROR("failed to get an upper layer for %s context\n", name.c_str());
				if(stack.size() > 0)
				{
					stack += "/";
				}
				stack += name;
				failure.push_back(stack);
				continue;
			}
		}
		context->setFilterTalId(0);
		encap_contexts.push_back(context);


		// protocol stack
		INFO("Stack:\n");
		stack = "";
		for(lan_contexts_t::iterator ctxit = lan_contexts.begin();
		    ctxit != lan_contexts.end(); ++ctxit)
		{
			INFO("   - %s\n", (*ctxit)->getName().c_str());
			if(stack.size() > 0)
			{
				stack += "/";
			}
			stack += (*ctxit)->getName();
		}
		for(encap_contexts_t::iterator ctxit = encap_contexts.begin();
		    ctxit != encap_contexts.end(); ++ctxit)
		{
			INFO("   - %s\n", (*ctxit)->getName().c_str());
			if(stack.size() > 0)
			{
				stack += "/";
			}
			stack += (*ctxit)->getName();
		}

		name_low = stack;
		transform(stack.begin(), stack.end(),
		          name_low.begin(), ::tolower);
		found = name_low.find("/");
		while(found != (signed)string::npos)
		{
			name_low.replace(found, 1, "_");
			found = name_low.find("/", found);
		}

		if(!test_iter(src_filename, folder + '/' + name_low + ".pcap",
		              compare, name, lan_contexts, encap_contexts))
		{
			ERROR("FAILURE %s\n\n", stack.c_str());
			failure.push_back(stack);
			continue;
		}
		else
		{
			INFO("SUCCESS %s\n\n", stack.c_str());
		}
	}
}



/**
 * @brief Run the encapsulation and decapsulation test for a plugin
 *
 * @param src_filename   The LAN source file in PCAP format
 * @param encap_filename The encapsualted packet PCAP file for dump or comparison
 * @param compare        Whether we dump or compare the encapsualted packets
 * @param name           The name of the tested encapsulation plugin
 * @param contexts       The stack of encapsulated contexts
 *
 * @return true on success, false otherwise
 */
static bool test_iter(string src_filename, string encap_filename,
                       bool compare, string name,
                       lan_contexts_t lan_contexts,
                       encap_contexts_t encap_contexts)
{
	bool success = false;

	bool is_eth = ((*lan_contexts.begin())->getName() == "Ethernet");
	INFO("Upper lan context is Ethernet\n");
	pcap_t *src_handle;
	pcap_t *comp_handle;
	pcap_t *encap_handle = NULL;
	pcap_dumper_t *dumper = NULL;
	unsigned int src_link_len;
	unsigned int encap_link_len;

	struct pcap_pkthdr header;
	unsigned char *packet;

	NetPacket *net_packet = NULL;
	NetBurst *encap_packets;
	NetBurst *packets = NULL;
	NetBurst::iterator it;
	NetBurst::iterator it2;

	map<long, int> time_contexts;

	unsigned int counter_src = 0;
	unsigned int counter_encap = 0;
	unsigned int counter_comp = 0;
	unsigned int len;

	static unsigned char output_header[65600];
	unsigned int header_init = 0;
	static unsigned char output_packet[65600];
	struct ether_header *eth_header;

	// open the source dump file
	INFO("Open source file '%s'\n", src_filename.c_str());
	if(!open_pcap(src_filename, &src_handle, src_link_len))
	{
		ERROR("failed to open the source pcap file\n");
		goto error;
	}

	// open the ip comparison dump file
	INFO("Open comparison file '%s'\n", src_filename.c_str());
	if(!open_pcap(src_filename, &comp_handle, src_link_len))
	{
		ERROR("failed to open the comparison pcap file\n");
		goto close_src;
	}

	// open the encap file for storage
	if(!compare)
	{
		INFO("Open dump file '%s'\n", encap_filename.c_str());
		dumper = pcap_dump_open(src_handle, encap_filename.c_str());
		if(dumper == NULL)
		{
			ERROR("failed to open dump file: %s\n", pcap_geterr(src_handle));
			goto close_comp;
		}
	}
	// open the encap file for comparison
	else
	{
		INFO("Open encapsulated packets file '%s'\n", encap_filename.c_str());
		if(!open_pcap(encap_filename, &encap_handle, encap_link_len))
		{
			ERROR("failed to open the encapsulated packets pcap file\n");
			goto close_comp;
		}
	}

	if(is_eth)
	{
		// keep ethernet header
		src_link_len = 0;
		encap_link_len = 0;
	}
	// for each packet in source dump
	success = true;
	counter_src = 0;
	while((packet = (unsigned char *) pcap_next(src_handle, &header)) != NULL)
	{
		counter_src++;

		if(!is_eth && !header_init)
		{
			memcpy(output_header, packet, src_link_len);
			header_init = 1;
		}

		net_packet = new NetPacket(packet + src_link_len,
		                           header.len - src_link_len);
		if(net_packet == NULL)
		{
			ERROR("[packet #%d] failed to create input packet\n", counter_src);
			success = false;
			continue;
		}

		DEBUG("[packet #%d] %s packet is %zu-byte long\n", counter_src,
		      (*lan_contexts.begin())->getName().c_str(),
		      net_packet->getTotalLength());

		// encapsulation
		encap_packets = new NetBurst();
		if(encap_packets == NULL)
		{
			ERROR("cannot allocate memory for burst of network frames\n");
			delete encap_packets;
			goto close_encap;
		}

		encap_packets->push_back(net_packet);
		DEBUG("[packet #%d] encapsulate in lan contexts\n", counter_src);

		for(lan_contexts_t::iterator ctxit = lan_contexts.begin();
		    ctxit != lan_contexts.end(); ++ctxit)
		{
			encap_packets = (*ctxit)->encapsulate(encap_packets, time_contexts);
			if(encap_packets == NULL)
			{
				ERROR("[packet #%d] %s encapsulation failed\n",
				      counter_src, (*ctxit)->getName().c_str());
				success = false;
				break;
			}
		}
		if(!encap_packets)
		{
			continue;
		}

		DEBUG("[packet #%d] encapsulate %d lan packets in encap contexts\n",
		      counter_src, encap_packets->length());
		for(encap_contexts_t::iterator ctxit = encap_contexts.begin();
			ctxit != encap_contexts.end(); ++ctxit)
		{
			encap_packets = (*ctxit)->encapsulate(encap_packets, time_contexts);
			if(encap_packets == NULL)
			{
				ERROR("[packet #%d] %s encapsulation failed\n",
				      counter_src, (*ctxit)->getName().c_str());
				success = false;
				break;
			}
			NetBurst *flushed = (*ctxit)->flushAll();
			if(flushed && !flushed->empty())
			{
				encap_packets->insert(encap_packets->end(), flushed->begin(),
				                      flushed->end());
			}
		}
		if(!encap_packets)
		{
			continue;
		}

		DEBUG("[packet #%d] 1 %s packet => %d %s packets\n", counter_src,
		       (*lan_contexts.begin())->getName().c_str(),
			   encap_packets->length(),
			   encap_packets->name().c_str());

		// handle the encapsulated packets
		for(it = encap_packets->begin(); it != encap_packets->end(); it++)
		{
			counter_encap++;

			if(!compare)
			{
				// output packets in first dump
				header.len = src_link_len + (*it)->getTotalLength();
				header.caplen = header.len;
				if(!is_eth)
				{
					memcpy(output_packet, output_header, src_link_len);
				}
				memcpy(output_packet + src_link_len, (*it)->getData().c_str(),
				       (*it)->getTotalLength());
				if(src_link_len == ETHER_HDR_LEN) /* Ethernet only */
				{
					eth_header = (struct ether_header *) output_packet;
					eth_header->ether_type = htons(0x162d); /* unused Ethernet ID ? */
				}
				else if(src_link_len == LINUX_COOKED_HDR_LEN) /* Linux Cooked Sockets only */
				{
					output_packet[LINUX_COOKED_HDR_LEN - 2] = 0x16;
					output_packet[LINUX_COOKED_HDR_LEN - 1] = 0x2d;
				}
				pcap_dump((u_char *) dumper, &header, output_packet);

				// dump packet
				Data data;
				Data::iterator it_data;
				data = (*it)->getData();
				DEBUG_L2("%s packet\n", name.c_str());
				for(it_data = data.begin(); it_data != data.end(); it_data++)
					DEBUG_L2("0x%.2x ", (*it_data));
				DEBUG_L2("\n");
			}
			else
			{
				unsigned char *cmp_packet;
				struct pcap_pkthdr cmp_header;

				cmp_packet = (unsigned char *) pcap_next(encap_handle, &cmp_header);
				if(cmp_packet == NULL)
				{
					ERROR("[encap packet #%d] %s packet cannot load packet for comparison\n",
					       counter_encap, (*it)->getName().c_str());
					success = false;
					continue;
				}

				/* compare the output packets with the ones given by the user */
				if(!is_eth && cmp_header.caplen <= encap_link_len)
				{
					ERROR("[encap packet #%d] %s packet available for comparison but too small\n",
					       counter_encap, (*it)->getName().c_str());
					success = false;
					continue;
				}

				if(!compare_packets((*it)->getData().c_str(),
				                    (*it)->getTotalLength(),
				                    cmp_packet + encap_link_len,
				                    cmp_header.caplen - encap_link_len))
				{
					ERROR("[encap packet #%d] %s packet is not as attended\n",
					       counter_encap, (*it)->getName().c_str());
					success = false;
					continue;
				}
			}
		}

		len = encap_packets->length();
		packets = encap_packets;
		DEBUG("[packet #%d] decapsulate %d packets in encap contexts\n",
		      counter_src, packets->length());
		// decapsulation
		for(encap_contexts_t::reverse_iterator ctxit = encap_contexts.rbegin();
		    ctxit != encap_contexts.rend(); ++ctxit)
		{
			packets = (*ctxit)->deencapsulate(packets);
			if(packets == NULL)
			{
				ERROR("[LAN packet #%d/ %s packets] %s decapsulation failed\n",
				      counter_src, name.c_str(), (*ctxit)->getName().c_str());
				success = false;
				break;
			}
		}
		if(!packets)
		{
			continue;
		}

		DEBUG("[packet #%d] decapsulate %d encap packets in lan contexts\n",
		      counter_src, packets->length());
		for(lan_contexts_t::reverse_iterator ctxit = lan_contexts.rbegin();
		    ctxit != lan_contexts.rend(); ++ctxit)
		{
			packets = (*ctxit)->deencapsulate(packets);
			if(packets == NULL)
			{
				ERROR("[LAN packet #%d/ %s packets] %s decapsulation failed\n",
				      counter_src, name.c_str(), (*ctxit)->getName().c_str());
				success = false;
				break;
			}
		}
		if(!packets)
		{
			continue;
		}

		if(!len)
		{
			delete packets;
		}
		else
		{
			DEBUG("[packet #%d] %d %s packets => %d %s packets\n",
			      counter_src, len, name.c_str(), packets->length(),
			      packets->name().c_str());

			counter_comp = 0;
			// compare LAN packets
			for(it2 = packets->begin(); it2 != packets->end(); it2++)
			{
				unsigned char *cmp_packet;
				struct pcap_pkthdr cmp_header;

				counter_comp++;

				cmp_packet = (unsigned char *) pcap_next(comp_handle, &cmp_header);
				if(cmp_packet == NULL)
				{
					ERROR("[packet #%d] %s packet cannot load packet for comparison\n",
					      counter_comp, (*it2)->getName().c_str());
					success = false;
					continue;
				}

				/* compare the output packets with the ones given by the user */
				if(!is_eth && cmp_header.caplen <= src_link_len)
				{
					ERROR("[packet #%d] %s packet available for comparison but too small\n",
					      counter_comp, (*it2)->getName().c_str());
					success = false;
					continue;
				}

				if(!compare_packets((*it2)->getData().c_str(),
				                    (*it2)->getTotalLength(),
				                    cmp_packet + src_link_len,
				                    cmp_header.caplen - src_link_len))
				{
					ERROR("[packet #%d] %s packet is not as attended\n",
					      counter_comp, (*it2)->getName().c_str());
					success = false;
					continue;
				}
			}

			delete packets;
			packets = NULL;
		}
	}

	DEBUG("\n");

	INFO("End of %s test\n", name.c_str());

	// clean memory
close_encap:
	if(!compare)
		pcap_dump_close(dumper);
	else
		pcap_close(encap_handle);
close_comp:
	pcap_close(comp_handle);
close_src:
	pcap_close(src_handle);
error:
	return success;
}


/**
 * @brief Compare two network packets and print differences if any
 *
 * @param pkt1      The first packet
 * @param pkt1_size The size of the first packet
 * @param pkt2      The second packet
 * @param pkt2_size The size of the second packet
 * @return          Whether the packets are equal or not
 */
static bool compare_packets(const unsigned char *pkt1, int pkt1_size,
                            const unsigned char *pkt2, int pkt2_size)
{
	int valid = true;
	int min_size;
	int i, j, k;
	char str1[4][7], str2[4][7];
	char sep1, sep2;

	min_size = pkt1_size > pkt2_size ? pkt2_size : pkt1_size;

	/* do not compare more than 180 bytes to avoid huge output */
	min_size = min(180, min_size);

	/* if packets are equal, do not print the packets */
	if(pkt1_size == pkt2_size && memcmp(pkt1, pkt2, pkt1_size) == 0)
		goto skip;

	/* packets are different */
	valid = false;

	DEBUG("------------------------------ Compare ------------------------------\n");

	if(pkt1_size != pkt2_size)
	{
		DEBUG("packets have different sizes (%d != %d), compare only the %d "
		      "first bytes\n", pkt1_size, pkt2_size, min_size);
	}

	j = 0;
	for(i = 0; i < min_size; i++)
	{
		if(pkt1[i] != pkt2[i])
		{
			sep1 = '#';
			sep2 = '#';
		}
		else
		{
			sep1 = '[';
			sep2 = ']';
		}

		sprintf(str1[j], "%c0x%.2x%c", sep1, pkt1[i], sep2);
		sprintf(str2[j], "%c0x%.2x%c", sep1, pkt2[i], sep2);

		/* make the output human readable */
		if(j >= 3 || (i + 1) >= min_size)
		{
			for(k = 0; k < 4; k++)
			{
				if(k < (j + 1))
					DEBUG("%s  ", str1[k]);
				else /* fill the line with blanks if nothing to print */
					DEBUG("        ");
			}

			DEBUG("      ");

			for(k = 0; k < (j + 1); k++)
				DEBUG("%s  ", str2[k]);

			DEBUG("\n");

			j = 0;
		}
		else
		{
			j++;
		}
	}

	INFO("----------------------- packets are different -----------------------\n");

skip:
	return valid;
}

/**
 * @brief Open a PCAP file and check link layer parameters
 *
 * @param filename  The file name
 * @param handle    The pcap handler
 * @param link_len  Link layer length
 * @return          true on success, false on failure
 */
static bool open_pcap(string filename, pcap_t **handle,
                      uint32_t &link_len)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	int link_layer_type;
	bool success = false;

	*handle = pcap_open_offline(filename.c_str(), errbuf);
	if(*handle == NULL)
	{
		ERROR("failed to open the PCAP file: %s\n", errbuf);
		goto error;
	}

	/* link layer in the dump must be supported */
	link_layer_type = pcap_datalink(*handle);
	if(link_layer_type != DLT_EN10MB &&
	   link_layer_type != DLT_LINUX_SLL &&
	   link_layer_type != DLT_RAW)
	{
		ERROR("link layer type %d not supported in dump (supported = "
		      "%d, %d, %d)\n", link_layer_type, DLT_EN10MB, DLT_LINUX_SLL,
		      DLT_RAW);
		goto close_input;
	}

	if(link_layer_type == DLT_EN10MB)
		link_len = ETHER_HDR_LEN;
	else if(link_layer_type == DLT_LINUX_SLL)
		link_len = LINUX_COOKED_HDR_LEN;
	else /* DLT_RAW */
		link_len = 0;

	success = true;
	return success;

close_input:
	pcap_close(*handle);
error:
	return success;
}


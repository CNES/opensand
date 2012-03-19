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

/*
 * Simple test application for encapsulation plugins
 *
 * The application takes a flow of IP packets as input, encapsulates
 * the IP packets and then desencapsulates them to get the IP packets back.
 *
 * The application outputs encapsulated and IP packets into PCAP files
 * given as arguments. The PCAP files can be compared with references.
 *
 * Launch the application with -h to learn how to use it.
 *
 * Author: Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * Author: Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <net/ethernet.h>
#include <algorithm>

// include for the PCAP library
#include <pcap.h>

// Platine includes
#include "EncapPlugin.h"
#include "IpPacket.h"
#include "PluginUtils.h"
#include "IpPacketHandler.h"


/// The length of the Linux Cooked Sockets header
#define LINUX_COOKED_HDR_LEN  16

/// The program version
#define VERSION   "Encapsulation plugins test application, version 0.1\n"

/// The program usage
#define USAGE \
"Encapsulation plugins test application: test the encapsulation plugins with a flow of IP packets\n\n\
usage: test [-h] [-v] [-d level] [-o] [-f folder] flow\n\
  -v        print version information and exit\n\
  -d level  print debug information\n\
               - 0 error only\n\
               - 1 debug messages\n\
               - 2 dump all encapsulated packets\n\
  -h        print this usage and exit\n\
  -o        save the generated encapsulated packets for each encapsulation scheme \n\
            instead of comparing them (PCAP format)\n\
  -f folder the folder where the files will be read/written (default: '.')\n\
  flow      flow of Ethernet frames to encapsulate (PCAP format)\n\n"


/** A very simple minimum macro */
#define MIN(x, y)  (((x) < (y)) ? (x) : (y))

static unsigned int verbose;

/** DEBUG macro */
#define DEBUG(format, ...) \
	do { \
		if(verbose) \
			printf(format, ##__VA_ARGS__); \
	} while(0)

#define DEBUG_L2(format, ...) \
	do { \
		if(verbose > 1) \
			printf(format, ##__VA_ARGS__); \
	} while(0)

#define ERROR(format, ...) \
	do { \
		fprintf(stderr, format, ##__VA_ARGS__); \
	} while(0)


static bool compare_packets(const unsigned char *pkt1, int pkt1_size,
                            const unsigned char *pkt2, int pkt2_size);
static bool open_pcap(std::string filename, pcap_t **handle,
                      uint32_t &link_len);
static bool test_iter(std::string src_filename, std::string encap_filename,
                       bool compare, std::string name,
                       std::vector<EncapPlugin::EncapContext *> contexts);

static bool test_encap_and_decap(std::string src_filename,
                                 std::string folder,
                                 bool compare);

int main(int argc, char *argv[])
{
	int status = 1;
	std::string src_filename = "";
	std::string folder = "./";
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
			// get the name of the file to store the IP packets
			folder = argv[1];
			args_used++;
		}
		else if(!strcmp(*argv, "-d"))
		{
			// get the name of the file to store the IP packets
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

	if(test_encap_and_decap(src_filename, folder, compare))
	{
		// success
		status = 0;
	}

quit:
	return status;
}

static bool test_encap_and_decap(std::string src_filename,
                                 std::string folder,
                                 bool compare)
{
	IpPacketHandler *ip_hdl = new IpPacketHandler(*((EncapPlugin *)NULL));
	PluginUtils utils;
	std::map<std::string, EncapPlugin *> encap_plug;
	std::map<std::string, EncapPlugin *>::iterator plugit;
	std::vector<std::string> failure;
	bool success = false;

	// load the plugins
	if(!utils.loadEncapPlugins(encap_plug))
	{
		ERROR("cannot load the encapsulation plugins\n");
		goto error;
	}

	// test each encap context
	for(plugit = encap_plug.begin(); plugit != encap_plug.end(); ++plugit)
	{
		std::vector <EncapPlugin::EncapContext *> contexts;
		std::string name = plugit->first;
		std::string name_low = name;
		std::transform(name.begin(), name.end(),
		               name_low.begin(), ::tolower);
		unsigned int found;
		found = name_low.find("/");
		while(found != std::string::npos)
		{
			name_low.replace(found, 1, "_");
			found = name_low.find("/", found);
		}
		EncapPlugin::EncapContext *context = plugit->second->getContext();

		if(!context->setUpperPacketHandler(ip_hdl, TRANSPARENT))
		{

			DEBUG("cannot set IP as upper layer for %s context, find another one\n",
			       name.c_str());

			std::vector<std::string> upper = context->getAvailableUpperProto(TRANSPARENT);
			// try to add a supported upper layer
			for(std::vector<std::string>::iterator iter = upper.begin();
			    iter != upper.end(); ++iter)
			{
				if(encap_plug[*iter] != NULL)
				{
					if(!context->setUpperPacketHandler(
								encap_plug[*iter]->getPacketHandler(),
								TRANSPARENT))
					{
						ERROR("failed to set upper packet src_handler for "
						        "%s context\n", name.c_str());
						failure.push_back(name.c_str());
						continue;
					}

					// add IP over this context
					if(!encap_plug[*iter]->getContext()->setUpperPacketHandler(ip_hdl, TRANSPARENT))
					{
						DEBUG("%s does not support IP as upper layer either",
						      encap_plug[*iter]->getName().c_str());
						continue;
					}

					DEBUG("add %s context over %s\n",
					       encap_plug[*iter]->getName().c_str(), name.c_str());

					contexts.push_back(encap_plug[*iter]->getContext());
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


		// protocol stack
		DEBUG("Stack:\n");
		for(std::vector <EncapPlugin::EncapContext *>::iterator ctxit = contexts.begin();
		    ctxit != contexts.end(); ++ctxit)
		{
			DEBUG("   - %s\n", (*ctxit)->getName().c_str());
		}


		if(!test_iter(src_filename, folder + '/' + name_low + ".pcap",
		              compare, name, contexts))
		{
			ERROR("Test failed for %s plugin\n", name.c_str());
			failure.push_back(name.c_str());
			continue;
	  	}
	}

	if(failure.size() == 0)
	{
		DEBUG("All tests were successful\n");
		success = true;
	}
	else
	{
		ERROR("The following tests failed:\n");
		for(std::vector<std::string>::iterator iter = failure.begin();
		    iter != failure.end(); ++iter)
		{
			ERROR("  - %s\n", (*iter).c_str());
		}
		success = false;
	}

	
error:
	delete ip_hdl;
	utils.releaseEncapPlugins();
	return success;
}



/**
 * @brief Run the encapsulation and decapsulation test for a plugin
 * 
 * @param src_filename   The IP source file in PCAP format
 * @param encap_filename The encapsualted packet PCAP file for dump or comparison
 * @param compare        Whether we dump or compare the encapsualted packets
 * @param name           The name of the tested encapsulation plugin
 * @param contexts       The stack of encapsulated contexts
 * 
 * @return true on success, false otherwise
 */
static bool test_iter(std::string src_filename, std::string encap_filename,
                       bool compare, std::string name,
                       std::vector<EncapPlugin::EncapContext *> contexts)
{
	bool success = false;

	pcap_t *src_handle;
	pcap_t *comp_handle;
	pcap_t *encap_handle = NULL;
	pcap_dumper_t *dumper = NULL;
	unsigned int src_link_len;
	unsigned int encap_link_len;

	struct pcap_pkthdr header;
	unsigned char *packet;

	NetPacket *ip_packet = NULL;
	NetBurst *encap_packets;
	NetBurst *ip_packets = NULL;
	NetBurst::iterator it;
	NetBurst::iterator it2;

	std::map<long, int> time_contexts;

	unsigned int counter_src = 0;
	unsigned int counter_encap = 0;
	unsigned int counter_comp = 0;
	unsigned int len;

	static unsigned char output_header[65600];
	unsigned int header_init = 0;
	static unsigned char output_packet[65600];
	struct ether_header *eth_header;

	// open the source dump file
	DEBUG("Open source file '%s'\n", src_filename.c_str());
	if(!open_pcap(src_filename, &src_handle, src_link_len))
	{
		ERROR("failed to open the source pcap file\n");
		goto error;
	}

	// open the ip comparison dump file
	DEBUG("Open comparison file '%s'\n", src_filename.c_str());
	if(!open_pcap(src_filename, &comp_handle, src_link_len))
	{
		ERROR("failed to open the comparison pcap file\n");
		goto close_src;
	}

	// open the encap file for storage
	if(!compare)
	{
		DEBUG("Open dump file '%s'\n", encap_filename.c_str());
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
		DEBUG("Open encapsulated packets file '%s'\n", encap_filename.c_str());
		if(!open_pcap(encap_filename, &encap_handle, encap_link_len))
		{
			ERROR("failed to open the encapsulated packets pcap file\n");;
			goto close_comp;
		}
	}

	// for each packet in source dump
	success = true;
	counter_src = 0;
	while((packet = (unsigned char *) pcap_next(src_handle, &header)) != NULL)
	{
		counter_src++;

		if(!header_init)
		{
			memcpy(output_header, packet, src_link_len);
			header_init = 1;
		}

		// create IP packet from source dump
		if(IpPacket::version(packet + src_link_len,
		                     header.len - src_link_len) == 4)
		{
			ip_packet = new Ipv4Packet(packet + src_link_len,
			                           header.len - src_link_len);
		}
		else if(IpPacket::version(packet + src_link_len,
		                          header.len - src_link_len) == 6)
		{
			ip_packet = new Ipv6Packet(packet + src_link_len,
			                           header.len - src_link_len);
		}
		if(ip_packet == NULL)
		{
			ERROR("[packet #%d] failed to create IP packet\n", counter_src);
			success = false;
			continue;
		}
		((IpPacket *)ip_packet)->setSrcTalId(0x1F);
		((IpPacket *)ip_packet)->setDstTalId(0x1F);
		((IpPacket *)ip_packet)->setQos(0x07);


		DEBUG("[packet #%d] %s packet is %d-byte long\n", counter_src,
			   ip_packet->getName().c_str(), ip_packet->getTotalLength());


		// encapsulation
		encap_packets = new NetBurst();
		encap_packets->push_back(ip_packet);
		for(std::vector <EncapPlugin::EncapContext *>::iterator ctxit = contexts.begin();
			ctxit != contexts.end(); ++ctxit)
		{
			encap_packets = (*ctxit)->encapsulate(encap_packets, time_contexts);
			if(encap_packets == NULL)
			{
				ERROR("[packet #%d] %s encapsulation failed\n",
					   counter_src, (*ctxit)->getName().c_str());
				success = false;
				continue;
			}
		}


		DEBUG("[packet #%d] 1 IP packet => %d %s packets\n", counter_src,
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
				memcpy(output_packet, output_header, src_link_len);
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
				if(cmp_header.caplen <= encap_link_len)
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
		ip_packets = encap_packets;
		// decapsulation
		for(std::vector <EncapPlugin::EncapContext *>::reverse_iterator ctxit =
					contexts.rbegin(); ctxit != contexts.rend(); ++ctxit)
		{
			ip_packets = (*ctxit)->deencapsulate(ip_packets);
			if(ip_packets == NULL)
			{
				ERROR("[IP packet #%d/ %s packets] %s decapsulation failed\n",
					   counter_src, name.c_str(), (*ctxit)->getName().c_str());
				success = false;
				continue;
			}
		}


		if(!len)
		{
			delete ip_packets;
		}
		else
		{
			DEBUG("[packet #%d] %d %s packets => %d %s packets\n",
				   counter_src, len, name.c_str(), ip_packets->length(),
				   ip_packets->name().c_str());

			counter_comp = 0;
			// compare IP packets
			for(it2 = ip_packets->begin(); it2 != ip_packets->end(); it2++)
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
				if(cmp_header.caplen <= src_link_len)
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

			delete ip_packets;
			ip_packets = NULL;
		}
	}

	DEBUG("\n");



	// flush contexts
	encap_packets = (contexts.back())->flushAll();
	if(encap_packets == NULL)
	{

		DEBUG("[flush] no need to flush %s context\n",
			   (contexts.back())->getName().c_str());
		DEBUG("End of %s test\n", name.c_str());
		goto close_encap;
	}

	DEBUG("[flush] => %d %s packets\n", encap_packets->length(),
		   encap_packets->name().c_str());

	// decapsulation
	for(it = encap_packets->begin(); it != encap_packets->end(); it++)
	{
		counter_encap++;

		if(!compare)
		{
			// output packets in first dump
			header.len = src_link_len + (*it)->getTotalLength();
			header.caplen = header.len;
			memcpy(output_packet, output_header, src_link_len);
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
			if(cmp_header.caplen <= encap_link_len)
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
	ip_packets = encap_packets;
	// decapsulation
	for(std::vector <EncapPlugin::EncapContext *>::reverse_iterator ctxit =
				contexts.rbegin(); ctxit != contexts.rend(); ++ctxit)
	{
		ip_packets = (*ctxit)->deencapsulate(ip_packets);
		if(ip_packets == NULL)
		{
			ERROR("[flush] %s decapsulation failed\n",
			      (*ctxit)->getName().c_str());
			success = false;
			continue;
		}
	}

	DEBUG("[flush] %d %s packets => %d %s packets\n", len,
		   name.c_str(), ip_packets->length(),
		   ip_packets->name().c_str());

	// compare IP packets
	for(it2 = ip_packets->begin(); it2 != ip_packets->end(); it2++)
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
		if(cmp_header.caplen <= src_link_len)
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

	delete ip_packets;
	
	DEBUG("End of %s test\n", name.c_str());

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
	int valid = 1;
	int min_size;
	int i, j, k;
	char str1[4][7], str2[4][7];
	char sep1, sep2;

	min_size = pkt1_size > pkt2_size ? pkt2_size : pkt1_size;

	/* do not compare more than 180 bytes to avoid huge output */
	min_size = MIN(180, min_size);

	/* if packets are equal, do not print the packets */
	if(pkt1_size == pkt2_size && memcmp(pkt1, pkt2, pkt1_size) == 0)
		goto skip;

	/* packets are different */
	valid = 0;

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

	DEBUG("----------------------- packets are different -----------------------\n");

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
static bool open_pcap(std::string filename, pcap_t **handle,
                      uint32_t &link_len)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	int link_layer_type;
	bool success = false;

	*handle = pcap_open_offline(filename.c_str(), errbuf);
	if(*handle == NULL)
	{
		DEBUG("failed to open the PCAP file: %s\n", errbuf);
		goto error;
	}

	/* link layer in the dump must be supported */
	link_layer_type = pcap_datalink(*handle);
	if(link_layer_type != DLT_EN10MB &&
	   link_layer_type != DLT_LINUX_SLL &&
	   link_layer_type != DLT_RAW)
	{
		DEBUG("link layer type %d not supported in dump (supported = "
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


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
 * @file UleExtSecurity.cpp
 * @brief Optional Security ULE extension
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "UleExtSecurity.h"
#include <cryptopp/arc4.h>
#define DBG_PACKAGE PKG_DEFAULT
#include "platine_conf/uti_debug.h"
#include <sys/types.h>
#include <sys/time.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <iostream>
#include <string>

using namespace CryptoPP;
using namespace std;
UleExtSecurity::UleExtSecurity(): UleExt()
{
	this->is_mandatory = false; //sould be true
	/*
	 * TODO: Need to ask didier about this behavior. Could be something with
	 * hdr_len in the decode process
	 */
	this->_type = 0x10; //secure ULE type can be changed later
}

UleExtSecurity::~UleExtSecurity()
{
}

ule_ext_status UleExtSecurity::build(uint16_t ptype, Data payload)
{
	// TODO: the length of the extension is currently arbitrary choosen,
	// Length is 1 * 32 bits + 1 * 16-bit field (next payload type field)

	/* get the ULE-SID from database --manual text tab delimited text file
	 * in te future use hash structures-- quicker
	 * This should also give the necessary keys and length to fill in
	 * CLASS KEY_STORAGE get_sad_data(int ULE_SID); */
	int i;
	int ULE_SID = 4444;
	int uli_netval = htonl(ULE_SID); //convert host to network for integers
	unsigned char temp[4];
	int plen;
	byte *p;
	ARC4 rc4((unsigned char *)"12345678", 8);

	this-> _payload.clear(); //very important doesnt work without that

	for(i = 0; i < 4; i++)
	{
		temp[i]= '\0';
	}
	memcpy( temp, &uli_netval, 4);

	// this->_payload is where we append all the extra security header stuff
	// in the future like integrity check and seq numbers.
	this->_payload.append(temp,4);

	// this->_payload.append(4 * 2, 0x00);
	// here is the next payload which is the ip payload
	// add the next header/payload
	this->_payload.append(1, (ptype >> 8) & 0xff);
	this->_payload.append(1, ptype & 0xff);

	// do the encryption here
	// need to copy the the string object to a char type
	// get the key from the ULE_SID
	// now put it into the rc4 stream cipher with the length of the keys
	plen = payload.length();
	p = new byte[plen];
	memcpy(p, payload.c_str(), plen);

	rc4.ProcessString(p, plen);

	payload.clear();
	//append the encrypted output to payload object
	payload.append(p, plen);
	delete []p;

	//add the encrypted payload
	this->_payload += payload;
	//printf("\n the length of the ULE payload is %d\n", this->_payload.length());
	//
	//  build the Next Header field for this extension
	//  - 5-bit zero prefix
	//  - 3-bit H-LEN field (= 3 is choosen for the moment)
	//  - 8-bit H-Type field (= 0x10 type of Security extension)
	this->_payloadType = ((3 & 0x07) << 8) | (this->type() & 0xff);
	return ULE_EXT_OK;
}

ule_ext_status UleExtSecurity::decode(uint8_t hlen, Data payload)
{
	const char FUNCNAME[] = "[UleExtPadding::decode]";
	ARC4 rc4d((unsigned char *)"12345678",8);
	int i;
	int plen;
	byte *p;

	// extension is optional, hlen must be 1-5
	if(hlen < 1 || hlen > 5)
	{
		UTI_ERROR("%s optional extension, but hlen (0x%x) != 1-5\n",
		          FUNCNAME, hlen);
		goto error;
	}

	// check if payload is large enough
	if(payload.length() < (size_t) hlen * 2)
	{
		UTI_ERROR("%s too few data (%u bytes) for %d-byte extension\n",
		          FUNCNAME, payload.length(), hlen * 2);
		goto error;
	}

/*
 * this is where we need to first separate the payload to decrypt
 * with the security header extension
 * size of ULE_SID --always present is 4 bytes
 * if we have additonal components then we should know how long it will be
 * for example after the ULE-SID there might be 4 byte seq num followed by
 * 20 byte HMAC using SHA1 algo so the overall length of the header is
 * 4(ULE-SID)+4(seq. num) + 20 (hmac)=28
 * in this case the lines below will be
 *     this->_payloadType = (payload.at(28 - 2) << 8) |
 *                           payload.at(28 - 1);
 *     this->_payload = payload.substr(28);
 */
	printf("\n the size of the payload is %d\n", payload.length());
	this->_payloadType = (payload.at(hlen * 2 - 2) << 8) |
	                     payload.at(hlen * 2 - 1);
	                     printf("\n the payload type is %u\n",this->_payloadType);
	this->_payload = payload.substr(hlen * 2);
	// printf("\n the size of the payload is %d\n", this->_payload.length());


	// start decrypting
	// int ULE_SID=4444;
	// int uli_netval=htonl(ULE_SID); //convert host to network for integers
	// unsigned char temp[4];
	// for( int i=0;i<4;i++)
	// temp[i]= '\0';
	// memcpy(temp,&uli_netval,4);
	// this->_payload is where we append all the extra security header stuff
	// in the future like integrity check and seq numbers.
	// now put it into the rc4 stream cipher with the length of the keys

	plen = this->_payload.length();
	p = new byte[plen];
	memcpy(p, this->_payload.c_str(), plen);

	rc4d.ProcessString(p, plen);

	this->_payload.clear();
	//append the decrypted output to payload object
	this->_payload.append(p, plen);

	delete []p;

	return ULE_EXT_OK;

	error:
	return ULE_EXT_ERROR;
}

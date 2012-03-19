/**
 * @file NccPepInterface.h
 * @brief Interface between NCC and PEP components
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef NCC_PEP_ITF_H
#define NCC_PEP_ITF_H

#include "PepRequest.h"
#include <vector>


/**
 * @class NccPepInterface
 * @brief Interface between NCC and PEP components
 */
class NccPepInterface
{
 private:

	/**
	 * @brief The TCP socket that listens for a connection from a PEP
	 */
	int socket_listen;

	/**
	 * @brief The TCP socket established with a PEP
	 */
	int socket_client;

	/** Whether a PEP is connected or not */
	bool is_connected;

	/** The list of commands received from the PEP component */
	std::vector<PepRequest *> requests_list;

 public:

	/**** constructor/destructor ****/

	/* initialize the interface between NCC and PEP components */
	NccPepInterface();

	/* destroy the interface between NCC and PEP components */
	~NccPepInterface();


	/**** accessors ****/

	/* get the TCP socket that listens for incoming PEP connections */
	int getPepListenSocket();

	/* get the TCP socket connected to the the PEP component */
	int getPepClientSocket();

	/* get the type of current PEP requests */
	pep_request_type_t getPepRequestType();

	/* get the list of PEP requests */
	PepRequest * getNextPepRequest();


	/**** socket management ****/

	/* create a TCP socket that listens for incoming PEP connections */
	bool listenForPepConnections();

	/* accept a new incoming connection from a PEP component */
	int acceptPepConnection();

	/* read a set of commands sent by the connected PEP component */
	bool readPepMessage();

 private:

	/* parse a message sent by the PEP component */
	bool parsePepMessage(const char *message);

	/* parse one of the commands sent in a message by the PEP component */
	PepRequest * parsePepCommand(const char *cmd);
};

#endif


/**
 * @file     NccInterface.h
 * @author   Adrien THIBAUD / <athibaud@toulouse.viveris.com>
 * @version  1.0
 * @brief    File that describes the TCP Socket
 *
 */

#ifndef NCC_INTERFACE_H
#define NCC_INTERFACE_H

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

/**
 * @class NccInterface
 * @brief Class that describes the TCP Socket
 */
class NccInterface
{
protected:
	/**
	 * @brief The TCP socket that listens for a connection
	 */
	int socket_listen;

	/**
	 * @brief The TCP socket established
	 */
	int socket_client;

	/** Whether an element is connected or not */
	bool is_connected;

	/** Output Log */
	OutputLog *log_ncc_interface;

public:

	/**** constructor/destructor ****/
	NccInterface();
	~NccInterface();

	/**** accessors ****/
	int getSocketListen();
	int getSocketClient();
	bool getIsConnected();

	/***** settors *****/
	void setSocketClient(int socket_client);
	void setIsConnected(bool is_connected);

	/**** socket management ****/
	
	/*create a TCP socket connected to the component */
	bool initSocket(int tcp_port);
	


};
#endif


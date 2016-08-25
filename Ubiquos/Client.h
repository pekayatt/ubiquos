// A C++ socket client class
//
// Keith Vertanen 11/98, updated 12/08

#ifndef _CLIENT_H__
#define _CLIENT_H__

#include "cocos2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>

// Duplicated from winsock2.h
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#endif

#if CC_TARGET_PLATFORM == CC_PLATFORM_WINRT
	// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")

	#include <Ws2ipdef.h>
	#include <Ws2def.h>
#endif

static const int BUFF_SIZE = 64000;

USING_NS_CC;

class Client
{
	public:
		Client(int iPort);
		~Client();

		char**				GetHostIP();
				
		bool				Connect(const char* pStrHost);

		bool				Close();										// Close the socket

		bool				SendString(const char* pStr);					// Send a string to socket
		
		int					GetSocketID()	{return m_iSock;};

	protected:		
		int					m_iPort;							// Port I'm listening on
		int					m_iPortDatagram;					// Datagram port I'm listening on
		int					m_iSock;							// Socket connection
		struct sockaddr_in	m_addrRemote;						// Connector's address information
		double*				m_pBuffer;							// Reuse the same memory for buffer
		double*				m_pBuffer2;
		struct hostent*	he;
		char* 				addr_list[5];

		int udp_socket;

	private:	

		std::thread*		connectionThread;
		char				tempChar[BUFF_SIZE];
};

#endif


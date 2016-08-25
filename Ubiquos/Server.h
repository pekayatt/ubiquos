#ifndef __SERVER_H__
#define __SERVER_H__

#include "cocos2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>


#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>


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
#endif

static const int BUFF_SIZE_SERVER = 64000;

class Server
{
  public:
		Server(int iPort, bool* pResult);
		~Server();
		
		bool				StartUDPServer(int numPlayers);

		bool				connect();										// Accept a new connection
		bool				Close();										// Close the socket

		bool				sendMsg(char* pStr, int playerNo);				// Send a string to a specific player
		bool				sendMsgAll(const char* pStr, int playerNo);		// Send a string to all connected players

		const char*			recvMsg(char* pStr, int iMax, char chTerm);

	private:		
		int					m_iPort;							// The port I'm listening on

		int					m_iListen;							// Descriptor we are listening on
		int					m_iSock;							// Descriptor for the socket
		struct sockaddr_in	m_addrRemote;						// Connector's address information
		struct sockaddr_in	m_addrMe;							// My address information

		int					m_SocketPointer;
		int					m_SocketArray[5];

		char				tempChar[BUFF_SIZE_SERVER];
		
		bool				isRunning;
};

#endif




#ifndef _UDP_Client_H__
#define _UDP_Client_H__

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

#endif



class ClientUDP
{
	public:
		ClientUDP();
		~ClientUDP();
		const char*			LookForServer(const char* myIp, const char* ipRange, int numPlayers,bool isSpectator);
		
		bool Close();

	protected:		
		int					udp_socket;

	private:	
		char				msg[128];
};

#endif


#define VERBOSE 1
#define UDP_PORT 50200

#include "UDPServer.h"
#include "Ubiquos.h"
#include "cocos2d.h"

#include <chrono>
#include <thread>
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	#include <sys/ioctl.h>
#endif

ServerUDP::ServerUDP()
{
	
	isRunning = false;

#ifdef _WIN32
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) 
	{
        if (VERBOSE) CCLog("ERROR Client UDP, WSAStartup failed with error");
        return;
    }
#endif
	

	if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	{
		if (VERBOSE) CCLog("ERROR Client UDP, socket");
		return;
	}
}

bool ServerUDP::StartUDPServer(int numPlayers,bool isSpectator)
{
	receiveSpectator=false;
	hasSpectator=false;
	int playerConnected;
	if(isSpectator==false){	
		playerConnected= 1; //The server as it is.
	}else{
		playerConnected=0;
		hasSpectator=true;
		CCLog("UDP Server: is spectator");
	}
	struct sockaddr_in server, si_other, bc_add;
	char msg[128];

	strcpy(msg, "");

	
	bc_add.sin_family		= AF_INET;
	bc_add.sin_port			= htons(UDP_PORT);
	memset(&(bc_add.sin_zero), 0, 8);				// Zero the rest of the struct 
	
	
	server.sin_family		= AF_INET;
	server.sin_port			= htons(UDP_PORT);
	server.sin_addr.s_addr	= INADDR_ANY;	
	memset(&(server.sin_zero), 0, 8);				// Zero the rest of the struct 
	
	
	int broadcastPermission = 1;
	int checkCall = setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastPermission, sizeof(broadcastPermission));
	if(checkCall == -1){
        return false;
	}
	checkCall = 1;
	checkCall = setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&checkCall, sizeof(checkCall));
	if(checkCall == -1){
		CCLog("UDPServer: SO_REUSEADDR error");
        return false;
	}

    if( bind(udp_socket ,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        return false;
    }
	
	int recv_len;
    socklen_t s_len;
	int j,i;
	int wordCount = 0;
	char parseMsg[3][80] = {0};

	s_len = sizeof(si_other);

	isRunning = true;
	while (true)
	{
		if ((recv_len = recvfrom(udp_socket, msg, 80, 0, (struct sockaddr *)&si_other ,&s_len)) < 0){
			return false;
		}

		//Verifica se msg de procura de servidor
		CCLog("UDP Server: msg recv: %s", msg);

		for (i = 0;  msg[i]!='.'&& i<recv_len; i++ ){
			for (j = 0; msg[i]!=';'; j++)
			{				
				parseMsg[wordCount][j] = msg[i++];
				if ( msg[i]=='.'){
					i--;
					break;
				}
			}
			wordCount++;
		}
		wordCount = 0;
		if(strcmp(parseMsg[0], "lookingServer")==0){
			if(atoi(parseMsg[1]) == numPlayers &&
				(hasSpectator==false ||(hasSpectator==true && strcmp(parseMsg[2], "isSpectator")!=0 ))){
				//Answer client that has found
				CCLog("UDP Server: Got client looking for server");
				CCLog("UDP Server: Client IP: %s ", inet_ntoa(si_other.sin_addr));

				bc_add.sin_addr = si_other.sin_addr;
				
				memset(msg, 0, sizeof(msg));
				strcpy(msg,"serverFound.");
				sendto(udp_socket,msg, strlen(msg), 0, (struct sockaddr *)&bc_add, sizeof(bc_add));

			}
		}
		//confirms that is connecting to it
		if(strcmp(parseMsg[0], "connecting")==0 &&
				(hasSpectator==false ||(hasSpectator==true && strcmp(parseMsg[1], "isSpectator")!=0 ))){
			CCLog("UDP Server: Got client connecting");
			if(strcmp(parseMsg[1], "isSpectator")!=0){
				playerConnected++;
			}else{
				hasSpectator=true;
				CCLog("UDP Server: Got spectator");
			}

			if (playerConnected>numPlayers-1){
				CCLog("CLOSING UDP");
				break; //Stop receiving new players
			}
		}
		memset(parseMsg, 0, sizeof(parseMsg));
		memset(msg, 0, 128);
	}
	
	isRunning = false;
#ifdef _WIN32
    WSACleanup();
    closesocket(udp_socket);
#else
	shutdown(udp_socket, 2);
	close(udp_socket);
#endif
	return true;
}

ServerUDP::~ServerUDP()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

//enables the server to receive one more player before closing the connection
void ServerUDP::enableReceiveSpectator(){
	receiveSpectator=true;

}

// Shut down the socket.
bool ServerUDP::Close()
{
	if (!isRunning){
		return true;
	}
#ifndef _WIN32
	close(udp_socket);
#else
	closesocket(udp_socket);
#endif
	udp_socket = -1;

	return true;
}

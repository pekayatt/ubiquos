#define TIMEOUT 500000
#define UDP_PORT 50200
#define VERBOSE 1

#include "UDPClient.h"
#include "Ubiquos.h"
#include "cocos2d.h"

#include <chrono>
#include <thread>
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	#include <sys/ioctl.h>
#endif

ClientUDP::ClientUDP()
{

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

const char* ClientUDP::LookForServer(const char* myIp, const char* ipRange, int numPlayers,bool isSpectator){

	int recv_len;
	struct sockaddr_in bc_addr, server, si_other;
    int err;

	
	memset(&(si_other), 0, sizeof(si_other));
	si_other.sin_family			= AF_INET;
	si_other.sin_port			= htons(UDP_PORT);
	si_other.sin_addr.s_addr	= INADDR_ANY;

	server.sin_family			= AF_INET;
	server.sin_port				= htons(UDP_PORT);
	server.sin_addr.s_addr		= inet_addr(myIp);
	memset(&(server.sin_zero), 0, 8);				// Zero the rest of the struct 


	memset(&(bc_addr), 0, sizeof(bc_addr));
	bc_addr.sin_family			= AF_INET;
	bc_addr.sin_port			= htons(UDP_PORT);
	bc_addr.sin_addr.s_addr		= inet_addr(ipRange);
	
	CCLog("UDPClient: Using ipRange: %s", ipRange);

	int OptVal = 1;
	err = setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&OptVal, sizeof(OptVal));
	if(err == -1){
		CCLog("UDPClient: SO_BROADCAST error");
		return "null";
	}

	int reuseAdd = 1;
	err = setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAdd, sizeof(reuseAdd));
	if(err == -1){
		CCLog("UDPClient: SO_REUSEADDR error");
		return "null";
	}
	
	err = bind(udp_socket, (struct sockaddr *)&server, sizeof(server));
	if(err == -1){
		CCLog("UDPClient: Bind error");
		return "null";
	} 
	
	if(isSpectator==true){
		sprintf(msg, "lookingServer;%d;isSpectator.\0",numPlayers);
	}else{
		sprintf(msg, "lookingServer;%d;isPlayer.\0",numPlayers);
	}

	if (err = sendto(udp_socket, msg, strlen(msg), 0, (struct sockaddr *)&bc_addr, sizeof(bc_addr)) < 0){
		CCLog("UDPClient: sendto error");
		return "null";
	}
	memset(msg, 0, sizeof(msg));
	
	int j,i;
	int wordCount = 0;
	char parseMsg[3][80] = {0};

	struct timeval tv;
	fd_set fds ;
	tv.tv_sec = 0;  
	tv.tv_usec = TIMEOUT; 
	
	FD_ZERO(&fds) ;
	FD_SET(udp_socket, &fds) ;
	setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	
	socklen_t sizeClienAddr = sizeof(si_other);
	//Maybe need loop here
	for(int retryCount = 0; retryCount<5; retryCount++){
		// Wait until timeout or data received.
		err = select ( udp_socket, &fds, NULL, NULL, &tv ) ;
		if ( err == 0){ 
			tv.tv_sec = 0;		//Reset timer
			tv.tv_usec = TIMEOUT;

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS
			if(err =FD_ISSET(udp_socket, &fds)>=0)
			{
				CCLog("UDPClient: FD_ISSET = %d", err);
			} else 
#endif
			{
			FD_ZERO(&fds) ;
			FD_SET(udp_socket, &fds) ;
			CCLog("UDPClient: Timeout");
			continue;  
			}
		} else if( err == -1 ) {
			continue;  //Error   
		}

		recv_len = 0;
		if ((recv_len = recvfrom(udp_socket, msg, 128, 0, (struct sockaddr *)&si_other ,&sizeClienAddr)) <0){
			CCLog("UDPClient: ERROR: %d", errno );
			memset(msg, 0, sizeof(msg));
			continue;
		} 

		CCLog("UDPClient: msg recv: %s", msg);
		
		for (i = 0; msg[i]!='.'; i++ ){
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
		if(strcmp(parseMsg[0], "serverFound")==0){
			CCLog("UDPClient: Server has been Found %s", inet_ntoa(si_other.sin_addr));
			if (isSpectator==true){
				strcpy(msg,"connecting;isSpectator.");
			}else{
				strcpy(msg,"connecting;isPlayer.");
			}
			sendto(udp_socket,msg, strlen(msg), 0, (struct sockaddr *)&si_other, sizeof(bc_addr));
			
			
#ifdef _WIN32
			closesocket(udp_socket);
			WSACleanup();
#else
			shutdown(udp_socket, 2);
			close(udp_socket);
#endif

			return inet_ntoa(si_other.sin_addr);
		}
		memset(parseMsg, 0, sizeof(parseMsg));
		memset(msg, 0, sizeof(msg));
	}
		
	memset(msg, 0, 128);
	
#ifdef _WIN32
    closesocket(udp_socket);
    WSACleanup();
#else
	shutdown(udp_socket, 2);
	close(udp_socket);
#endif

	return "null";
}

ClientUDP::~ClientUDP()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

// Shut down the socket.
bool ClientUDP::Close()
{
	if (shutdown(udp_socket, SD_BOTH) == -1)
		return false;
#ifndef _WIN32
	close(udp_socket);
#endif
	return true;
}

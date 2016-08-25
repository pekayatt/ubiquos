#include "Client.h"
#include "Ubiquos.h"
#include "UDPClient.h"

#include <chrono>
#include <thread>

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	#include <sys/ioctl.h>
//#else
	//#include <iphlpapi.h>
	//#pragma comment(lib, "IPHLPAPI.lib")
#endif

#define VERBOSE 1     // turn on or off debugging output
#define TIMEOUT 2

char** Client::GetHostIP(){
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID|| CC_TARGET_PLATFORM == CC_PLATFORM_IOS)

	struct ifreq ifr;

	const char* iface = CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID?"wlan0":"en0";

	//Type of address to retrieve - IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;

	//Copy the interface name in the ifreq structure
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);

	ioctl(m_iSock, SIOCGIFADDR, &ifr);
	//display result

	addr_list[0] = inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);
	if (VERBOSE) CCLog("Host name IP %s", addr_list[0] );

	addr_list[1] = "last";
	//CCLog("List Terminator %s", addr_list[1] );

	return addr_list;

#else
	char ac[80];
    if (gethostname(ac, sizeof(ac)) == -1) {
        return nullptr;
    }
    if (VERBOSE) CCLog("Host name is %s", ac );

	if (strcmp(ac, "localhost")==0){ // ANDROID DEVICES ARE DUMB AND GET ONLY LOCALHOST
		return nullptr;
	}

    struct hostent *phe = gethostbyname(ac);
    if (phe == 0) {
		if (VERBOSE) CCLog("Yow! Bad host lookup." );
		return  nullptr;
    }
	
	struct in_addr addr;

	int i;
	
	for (i = 0; phe->h_addr_list[i] != NULL; ++i) {
		
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));

		char* inetAdd = inet_ntoa(addr);
		
		addr_list[i] =(char*) malloc(20);

		strcpy(addr_list[i],inetAdd);
		if (VERBOSE) CCLog("Address: %s", addr_list[i]);
		
	}
	addr_list[i] = "last";
	return addr_list; 

#endif
    

}


//Do the receiving of messages
void *client_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[BUFF_SIZE] = "";
	char senderID[15];
	int j = 0;
     
	
	int msgCounter=1;
	int points[8];
	points[0]=0;
	int i;
	int k=0;
	int x;
	int y;
	char parseMsg2[BUFF_SIZE] = "";
    //Receive a message from client in loop
	while( sock > 0 )
    {
		read_size = recv(sock , client_message , BUFF_SIZE , 0);
		if (read_size < 0 )
			continue;
		else if (read_size == 0){
			if (VERBOSE) CCLog("Client: connection closed");
			sprintf(client_message, "playerQuit;%d.", sock);
			Ubiquos::getInstance()->addMessage(sock, client_message);

			return 0; //Socket has been closed
		}
		if(read_size>300){
			
		//	CCLog("probably receiving image");
			
		}
        //end of string marker
		client_message[read_size] = '\0';
		
		
		
		if (VERBOSE) 
			CCLog("Client: received '%s' by %d", client_message, sock);    

		msgCounter=1;
		for (i = 0; i<read_size; i++){
			if (client_message[i]=='.'){
				points[msgCounter]=i+1;
				msgCounter++;
			}
		}

		k=0;
		for(i=0;i<msgCounter-1;i++){
			for (j = points[i]; client_message[j]!=';' && j<read_size; j++)
			{
				senderID[k] = client_message[j];
				k++;
			}

			senderID[k] = '\0'; //End string char
			k=0;

			if (atoi(senderID)!=sock){

				x=0;
				for (j = points[i]; client_message[j]!='.'; j++ ){
			
						parseMsg2[x] = client_message[j];
						//parseMsg2[x+1] = '\0';

					x++;
						
				}
				parseMsg2[x] = '.';
				if (VERBOSE) CCLog("Client: adding message %s", parseMsg2);
				Ubiquos::getInstance()->addMessage(atoi(senderID), parseMsg2);
				memset(parseMsg2, 0, sizeof(parseMsg2));
			}
			
		}

		
		//clear the message buffer
		memset(points, 0, sizeof(points));
		memset(client_message, 0, sizeof(client_message));
		read_size = 0;
    }
      
    return 0;
}

bool Client::Connect(const char* pStrHost){
	
	if (VERBOSE) 
		CCLog("Client: connecting socket to %s on port = %d\n", pStrHost, m_iPort);

	
	if ((m_iSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		
		if (VERBOSE) CCLog("ERROR Client, socket");
		return false;
	}

	if ((he = gethostbyname(pStrHost)) == NULL) 
	{
		
		if (VERBOSE) CCLog("ERROR Client, gethostbyname");
		return false;
	}

	//Connecting without timeout
	//if (connect(m_iSock, (struct sockaddr *) &m_addrRemote, sizeof(struct sockaddr)) < 0){
	//	CCLog("\n\nClient, ERROR CONNECT\n\n");
	//}
	//
	//if (VERBOSE) CCLog("\n\nClient, Connected\n\n");
	//connectionThread = new std::thread(client_handler, (void*) &m_iSock);

	//return true;

	timeval m_timeout;
    m_timeout.tv_sec = TIMEOUT;
    m_timeout.tv_usec = 0;
	

	u_long iMode = 1;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    int iResult = ioctl(m_iSock, FIONBIO, &iMode);
#else
	int iResult = ioctlsocket(m_iSock, FIONBIO, &iMode);
#endif
	
	m_addrRemote.sin_family		= AF_INET;        
	m_addrRemote.sin_port		= htons(m_iPort);      
	m_addrRemote.sin_addr		= *((struct in_addr *) he->h_addr); 
	memset(&(m_addrRemote.sin_zero), 0, 8);

	int res;
	for (int retry = 0; retry <3; retry++){
		res = connect(m_iSock, (struct sockaddr *) &m_addrRemote, sizeof(struct sockaddr));
		if (res < 0)
		{
	#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
			if (errno ==  EINPROGRESS) {
	#else
			res = WSAGetLastError();
			if (res == WSAEWOULDBLOCK){
	#endif
				fd_set Write, Err;
				FD_ZERO(&Write);
				FD_ZERO(&Err);
				FD_SET(m_iSock, &Write);
				FD_SET(m_iSock, &Err);
 
				// check if the socket is ready
				if (select(m_iSock+1,NULL,&Write,&Err,&m_timeout) > 0 ){
					m_timeout.tv_sec = TIMEOUT;
					m_timeout.tv_usec = 0; //100ms for min ping

					if(FD_ISSET(m_iSock, &Write)>0)
					{
					
						if (VERBOSE) CCLog("\n\nClient, Connected\n\n");

						connectionThread = new std::thread(client_handler, (void*) &m_iSock);
						return true;
					}
				}
			}
			if (VERBOSE) 
				CCLog("ERROR Client, connection timeout m_iSock");

		}    
	}

	return true;
}

Client::Client(int iPort)
{
	he = NULL;

	m_iPort = iPort;
	m_iSock = -1;


#ifdef _WIN32
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) 
	{
        if (VERBOSE) CCLog("ERROR Client, WSAStartup failed with error");
        return;
    }
#endif
	

	if ((m_iSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		if (VERBOSE) CCLog("ERROR Client, socket");
		return;
	}
}

Client::~Client()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

// Send a string to the socket
bool Client::SendString(const char* pStr)
{
	if (strcmp(pStr, "")==0){
		return false;
	}
	
	//char* tempChar = new char();
	//tempChar = new char();
	sprintf(tempChar, "%d;%s",m_iSock, pStr); //Adds id to message

	int resp = send(m_iSock, (char *) tempChar, strlen(tempChar), 0);
	if (resp < 0)
	{
		if (VERBOSE) CCLog("ERROR Client::SendString, send error: %d", resp);
		return false;
	}

//	CCLog("Client: sending string '%s'", tempChar);


	return true;
}

// Shut down the socket.
bool Client::Close()
{
	if (shutdown(m_iSock, SD_BOTH) == -1)
		return false;
#ifndef _WIN32
	close(m_iSock);
#endif
	
	return true;
}

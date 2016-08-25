#include "Server.h"
#include "Ubiquos.h"
#include "UDPServer.h"

#define BACKLOG 5      // How many pending connections queue will hold 
#define VERBOSE 1       // Turn on or off debugging output


Server::Server(int iPort, bool* pResult)
{
	m_iPort			= iPort;
	m_SocketPointer = 0;

	isRunning = false;
	
	m_SocketArray[0] = -1;
	m_SocketArray[1] = -1;
	m_SocketArray[2] = -1;
	m_SocketArray[3] = -1;
	m_SocketArray[4] = -1;

	m_iSock = 0;

	if (pResult)
		*pResult = false;



#ifdef _WIN32
	// For Windows, we need to fire up the Winsock DLL before we can do socket stuff.
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;

	// Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h 
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) 
	{
        // Tell the user that we could not find a usable Winsock DLL
        CCLog("ERROR, WSAStartup failed with error");
        return;
    }
#endif

	if (VERBOSE) 
		CCLog("Server: opening socket on port = %d\n", m_iPort);

	if ((m_iListen = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		CCLog("ERROR, socket");
		return;
	}

	m_addrMe.sin_family			= AF_INET;          // Host byte order 
	m_addrMe.sin_port			= htons(m_iPort);	// Short, network byte order 
	m_addrMe.sin_addr.s_addr	= INADDR_ANY;		// Auto-fill with my IP 
	memset(&(m_addrMe.sin_zero), 0, 8);				// Zero the rest of the struct 

	if (bind(m_iListen, (struct sockaddr *) &m_addrMe, sizeof(struct sockaddr)) == errno) 
	{
		// Note, this can fail if the server has just been shutdown and not enough time has elapsed.
		// See: http://www.developerweb.net/forum/showthread.php?t=2977 
		CCLog("ERROR, bind");
		return;
	}

	//err = listen(m_iListen, BACKLOG);
	if (listen(m_iListen, BACKLOG) == -1)
	{
		CCLog("ERROR, listen");// %d", WSAGetLastError());
		return;
	}


	if (pResult)
		*pResult = true;
}

Server::~Server()
{
#ifdef _WIN32
	// Windows specific socket shutdown code
	WSACleanup();
#endif

}

//Do the receiving of messages
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[BUFF_SIZE_SERVER];
     
    //Receive a message from client in loop
    while( true) 
    {		
		read_size = recv(sock , client_message , BUFF_SIZE_SERVER , 0);
		if (read_size < 0 )
			continue;
		else if (read_size == 0){
			if (VERBOSE) 
				CCLog("Server: connection closed");
			//TODO: Avisar saida do cliente
			char tempchar[80];
			sprintf(tempchar, "playerQuit;%d.", sock);
			Ubiquos::getInstance()->addMessage(sock, tempchar);
			return 0; //Socket has been closed
		}

        //end of string marker
		client_message[read_size] = '\0';
		
		
		if (VERBOSE) 
			CCLog("Server: received '%s' by %d", client_message, sock);    

		//Add message received to queue
		Ubiquos::getInstance()->addMessage(sock, client_message);


		
		//clear the message buffer
		memset(client_message, 0, BUFF_SIZE);
    }
     
         
    return 0;
}

// Wait for somebody to connect to us on our port.
bool Server::connect()
{
#ifdef _WIN32
	int iSinSize = sizeof(struct sockaddr_in);
#else
	socklen_t iSinSize = (socklen_t) sizeof(struct sockaddr_in);
#endif
	isRunning = true;

	std::thread* connectionThread;

	while( (m_iSock = accept(m_iListen, (struct sockaddr *)&m_addrRemote, &iSinSize)) )
    {
        
		if (strcmp(inet_ntoa(m_addrRemote.sin_addr), "205.205.205.205")==0 ||
				strcmp(inet_ntoa(m_addrRemote.sin_addr), "0.0.0.0")==0){ //Error Case
			return false;
		}

		if (!isRunning){
			return false;
		}
		
		if (VERBOSE) {
			CCLog("Server: got connection from %s\n", inet_ntoa(m_addrRemote.sin_addr));
		}
		m_SocketArray[m_SocketPointer++] = m_iSock;
		
		connectionThread = new std::thread(connection_handler, (void*) &m_iSock);

    }


	return true;
}


bool Server::sendMsg(char* pStr, int playerNo)
{
	if (strcmp(pStr, "")==0){
		return false;
	}

	//char tempChar[80];
	sprintf(tempChar, "%d;%s",m_iSock, pStr); //Adds id to message

	if (send(playerNo, (char *) tempChar, strlen(tempChar), 0) == -1)
	{
		if (VERBOSE) 
			CCLog("Server::SendString, send");              
		return false;
	}

	if (VERBOSE) {
		CCLog("Server: sending string '%s'\n", pStr);
	}
	return true;
}

// send a string to the socket
bool Server::sendMsgAll(const char* pStr, int playerNo)
{
	if (strcmp(pStr, "")==0){
		return false;
	}

	if (playerNo==-1)
		sprintf(tempChar, "%d;%s",m_iSock, pStr); //Adds id to message
	else
		sprintf(tempChar, "%s", pStr); //Adds id to message
	

	for (int i=0; i<m_SocketPointer; i++)
	{
		if (send(m_SocketArray[i], (char *) tempChar, strlen(tempChar), 0) == -1)
		{			
			if (VERBOSE) 
				CCLog("ERROR::sendMsgAll");              
			continue;
		}
		if (VERBOSE) CCLog("Server: sending string '%s' to ID:%d", tempChar, m_SocketArray[i]);
	}
	

	return true;
}



// Shut down the socket
bool Server::Close()
{
	isRunning = false;

	for (int i =0; i<m_SocketPointer;i++){
		
		shutdown(m_SocketArray[i], 2);// SHUT_RDWR);
#ifndef _WIN32
		close(m_SocketArray[i]);
#else
		closesocket(m_SocketArray[i]);
#endif
	}

	shutdown(m_iListen, 2);// SHUT_RDWR);

#ifndef _WIN32
	close(m_iListen);
#else
	closesocket(m_iListen);
	WSACleanup();
#endif
	m_iSock = 0;
	m_iListen = 0;
	
	return true;
}

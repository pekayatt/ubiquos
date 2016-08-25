#include "Ubiquos.h"

#define VERBOSE 1

using namespace std;

Ubiquos* Ubiquos::m_instance = NULL;

Ubiquos* Ubiquos::getInstance()
{
    if (m_instance == NULL) {
        m_instance = new Ubiquos();
    }
	
    return m_instance;
}

Ubiquos::Ubiquos(void)
{ 
	this->reset();
}


Ubiquos::~Ubiquos(void)
{
	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	if (m_server != NULL){
		m_server->Close();
	}
	if (m_client != NULL){
		m_client->Close();
	}
#endif
}


void Ubiquos::reset(){
	CCLog("Ubiquos reset");
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	m_server = NULL;
	m_client = NULL;
	udpServer = NULL;
#endif

	busy=false;

	this->m_playerID = 0;
	m_isServer = false;
	m_hasFoundServer = false;
	m_clientNo = 0;
	m_HandshakeDone = false;
	for(int i=0;i<5;i++){
		playerConnected[i]=false;
	}
}

bool Ubiquos::isConnected(){
	return m_HandshakeDone;
}

void* threadServerConnect(void* pServer)
{
	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	((Server*)pServer)->connect();
#endif
	return 0;
}
void* threadServerUDP(void* pServerUDP, void* numPlayers, void* isSpectator)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	((ServerUDP*)pServerUDP)->StartUDPServer((int)numPlayers,(bool) isSpectator);
#endif
	pServerUDP = NULL;
	return 0;
}


void Ubiquos::connect(const char* ip, int numPlayers = 4)
{	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	if (strcmp(ip, "")==0){
		this->connect(numPlayers,false);
		return;
	} else if (strcmp(ip, "s")==0){
		this->createServer(numPlayers,false);
		return;
	} else if (strcmp(ip, "c")==0){
		//CCDirector::sharedDirector()->pause(); //To avoid concorrence
		m_client = new Client(48007);

		if (m_client->Connect("192.168.1.112")){
			if (VERBOSE){
				CCLog("Has connected to %s", ip);
			}
			m_hasFoundServer=true;
		}
		return;
	} else if (strcmp(ip, "p")==0){
		//CCDirector::sharedDirector()->pause(); //To avoid concorrence
		m_client = new Client(48007);

		if (m_client->Connect("192.168.1.105")){
			if (VERBOSE){
				CCLog("Has connected to %s", ip);
			}
			m_hasFoundServer=true;
		}
		return;
	}
	
	m_client = new Client(48007);
	if (m_client->Connect(ip)){
		if(VERBOSE){
			CCLog("Has connected to %s", ip);
		}
		return;
	}
	
	//Falhou conectar
	if(VERBOSE){
		CCLog("Fail to connect %s", ip);
	}
#endif
}

void Ubiquos::connect(int numPlayers, bool isSpectator)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	m_client = new Client(48007);
	 
	const char* serverIP = this->findServer(numPlayers,isSpectator); //tries to connect to a server into my IP range
	bool bResult = false;

	if (strcmp(serverIP, "null")==0 && !m_isServer) //Could not find anyserver
	{
		this->createServer(numPlayers,isSpectator);
	} else {
		m_client->Connect(serverIP);

		m_hasFoundServer=true;

		if(VERBOSE){
			CCLog("Has connected to %s", serverIP);
		}
	}
#endif
	
	return;
}

void Ubiquos::createServer(int numPlayers, bool isSpectator){
	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	m_isServer = true;
	m_hasFoundServer=true;

	m_HandshakeDone=true;
		
	bool bResult = false;
	m_server = new Server(48007, &bResult);
	
	if (!bResult)
	{
		if(VERBOSE){
		CCLog("Failed to create Server object!\n");
		}
		this->Close();
		return;
	}
	
	//TODO: get from game
	NetPlayer addPlayer;
	addPlayer.id = m_clientNo++;
	CCLog("M_CLIENTNO INCREMENT");
	addPlayer.name = "Ordep Tayak";
	addPlayer.profileID = 3;
				
	this->playerList[addPlayer.id] = addPlayer;
	this->playerConnected[addPlayer.id] = true;

	udpServer = new ServerUDP();
	serverUDPThread = new thread(threadServerUDP,(void*)udpServer, (void*)numPlayers, (void*)isSpectator);
	serverThread = new thread(threadServerConnect, m_server);
#endif
}

void Ubiquos::sendMessage(const char* msg)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	if(VERBOSE){
		CCLog("Sending message: %s", msg);
	}
	if (m_isServer)
	{
		m_server->sendMsgAll(msg, -1);
		return;
	} 
	strcpy(tempChar, msg);
	m_client->SendString(msg);

//	if (!m_client->SendString(msg)){ // Sometimes it fails, how to handle that?
//		this->reset();
//		//TODO: Report error
//	}	
#endif
}

void Ubiquos::sendMessage(char const* msg, int playerNo)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	if (m_isServer)
	{
		m_server->sendMsg((char*)msg, playerNo);
		return;
	}
	strcpy(tempChar, msg);
	m_client->SendString(tempChar);
#endif
}

void Ubiquos::addMessage(int playerID, char const* msg)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	std::unique_lock<std::mutex> mlock(mtx);
    while (busy==true)
    {
      cv.wait(mlock);
    }
	busy=true;

	memset(recvMsgs, 0, sizeof(recvMsgs));

	int wordCount = 0;
	int i = 0;
	int j = 0;
	int msgCounter = 0;

	int msgLenght = strlen(msg);

	for (i = 0; i<msgLenght; i++){
		if (msg[i]=='.'){
			msgCounter++;
		}
		
	}

	int endChar = 0;

	for(int k=0; k< msgCounter; k++){
		for (i = endChar; msg[i]!='.'; i++ ){
			for (j = 0; msg[i]!=';'; j++)
			{				
				parseMsg[wordCount][j] = msg[i];
				parseMsg[wordCount][j+1] = '\0';
				if (msg[++i]=='.'){
					i--;
					break;
				}
			}
			wordCount++;
		}
		endChar = i+1;
		for (i=0; i<wordCount; i++){
			if (i==0){
				sprintf(recvMsgs[k], "%s\0",parseMsg[i]);
			} else if (i==wordCount-1){
				sprintf(recvMsgs[k], "%s;%s.\0",recvMsgs[k], parseMsg[i]);
			} else {
				sprintf(recvMsgs[k], "%s;%s\0",recvMsgs[k], parseMsg[i]);
			}
		}
		
		processInsideMessages(parseMsg, playerID);
		wordCount= 0;

		string* recvMsgBuff = new string(recvMsgs[k]);

		msg_queue.push(recvMsgBuff);
		if (m_isServer)
		{
			m_server->sendMsgAll(msg,playerID);
		}
	}

	busy=false;
	cv.notify_all();
#endif
}

void Ubiquos::processInsideMessages(char parseMsg[30][1024], int playerID){
	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	int i = 1;
	bool newPlayer = false;
	NetPlayer addPlayer;
	
	if (strcmp(parseMsg[i++], "newPlayer")==0){ //"newPlayer;-1;Jonh Doe; 3."
			
		if (atoi(parseMsg[i++]) == -1 && (m_HandshakeDone==true||m_isServer==true)){
			addPlayer.id = m_clientNo++;
			CCLog("M_CLIENTNO INCREMENT BECAUSE SERVER");
			
		} else {
			CCLog("M_CLIENTNO INCREMENT");
			addPlayer.id = atoi(parseMsg[i-1]);
			m_clientNo++;
		}
		addPlayer.name = new char[40];
		strcpy(addPlayer.name, parseMsg[i++]);
		addPlayer.profileID = atoi(parseMsg[i]);
				
		this->playerList[addPlayer.id] = addPlayer;
		this->playerConnected[addPlayer.id] = true;

		newPlayer = true;				
	}
	
	

	if (m_isServer)
	{
		if (newPlayer){
			sprintf(tempChar2, "gotPlayer; %d.",addPlayer.id);
			m_server->sendMsg(tempChar2,playerID);	

			//TODO: Send all other player to this player	
			for (int i = 0; i < 5; i++)
			{
				if(isPlayerConnected(i)==true){
					NetPlayer tempPlayer = this->playerList[i];
				
					sprintf(tempChar2, "newPlayer;%d;%s;%d.\0",tempPlayer.id, tempPlayer.name, tempPlayer.profileID);
					m_server->sendMsg(tempChar2, playerID);
				}
			}
		}
	} 
	else if (!m_HandshakeDone){
		if (strcmp(parseMsg[1], "gotPlayer")==0){ //"newPlayer;Jonh Doe; 3"
			this->m_playerID = atoi(parseMsg[2]);
			m_HandshakeDone = true;
			//m_clientNo++;//testing
		}
	
	}
#endif
}

const char* Ubiquos::getMessage()
{
	if (msg_queue.size() <1)
		return "";

	
	//strcpy(recvMsg, msg_queue.front());

	string* tmp_msg = msg_queue.front();

	msg_queue.pop();

	return tmp_msg->c_str();
}

char const* Ubiquos::findServer(int numPlayers, bool isSpectator)
{	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	char addr_list[5][20];
	int list_size;
	
	char* ipAddress;
	const char* serverAddress;

	char ipRaw[20];
	int pointCounter = 0;
		

	char** addr_list_temp = m_client->GetHostIP();
	if (addr_list_temp==nullptr){
		return "null";
	}

	for (list_size = 0; strcmp(addr_list_temp[list_size], "last")!=0; list_size++) {
		if (VERBOSE)
			CCLog("Copying IP:%s", addr_list_temp[list_size] );

		strcpy(addr_list[list_size],addr_list_temp[list_size]);
	}
	if (VERBOSE)
		CCLog("list of ips has been copied");
	

	for (int i = 0; i < list_size; i++) {
		
		ipAddress = addr_list[i];
		if (strcmp(ipAddress, "204.204.204.204")==0
				|| strcmp(ipAddress, "0.0.0.0")==0
				|| strcmp(ipAddress, "205.205.205.205")==0){
			return "null";
		}
		if (VERBOSE)
			CCLog("Range[%i]: %s", i, ipAddress);
		
		strcpy(ipRaw, "xxx.xxx.xxx.");

		for (int j=0; pointCounter<3 ;j++){
			ipRaw[j] = ipAddress[j];
			if (ipAddress[j] == '.'){
				pointCounter++;
			}
			ipRaw[j+1] = '\0';
		}
		pointCounter = 0;

		sprintf(ipRaw, "%s%d", ipRaw, 255);

		ClientUDP* _clientUDP = new ClientUDP();
		
		serverAddress = _clientUDP->LookForServer(addr_list[i], ipRaw, numPlayers,isSpectator);
		if (strcmp(serverAddress, "null")!=0)
			return serverAddress;
    }
	
	if (VERBOSE)
		CCLog("No server has been found");
#endif
	return "null";
}

int Ubiquos::getNumPlayersConnected(){
	return m_clientNo;
}


int Ubiquos::getPlayerAvatarNumber(int id){
	return playerList[id].profileID;
}

void Ubiquos::setPlayerAvatarNumber(int id,int number){
	playerList[id].profileID=number;
}
void Ubiquos::setServerLocalId(int id){
	
	NetPlayer addPlayer;
	addPlayer.id = id;
	addPlayer.name = playerList[0].name;
	addPlayer.profileID = playerList[0].profileID;

	m_clientNo--;
	CCLog("mclientno %d",m_clientNo);
	playerConnected[0] = false;

	//this->playerList[0]= NULL;
	this->playerList[addPlayer.id] = addPlayer;
	this->playerConnected[addPlayer.id] = true;

}
int Ubiquos::getLocalId(){
	return m_playerID;
}

bool Ubiquos::isPlayerConnected(int i){
//	CCLog("Ubiq: player %d status: %d",i,playerConnected[i]);
	return playerConnected[i];
}

bool Ubiquos::isServer(){
	return m_isServer;
}

bool Ubiquos::foundServer(){
	return m_hasFoundServer;
}


void Ubiquos::Close(){
	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	
	if (m_client != NULL){
		this->sendMessage("playerQuit.");
		m_client->Close();
	}

	if (m_server!= NULL && this->isServer()){		
		//serverThread->detach();
		//serverUDPThread->detach();
		m_server->Close();

		if (udpServer!=NULL){
			udpServer->Close();
		}
	}
	while (!msg_queue.empty()){
		msg_queue.pop();
	}
	
	this->reset();
#endif
}

void Ubiquos::enableReceiveSpectator(){
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	udpServer->enableReceiveSpectator();
#endif
}

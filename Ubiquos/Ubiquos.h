#ifndef __UBIQUOS_H__
#define __UBIQUOS_H__

#include "cocos2d.h"

#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
#include "Client.h"
#include "Server.h"
#include "UDPServer.h"
#include "UDPClient.h"
#endif
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#if (CC_TARGET_PLATFORM != CC_PLATFORM_WINRT) && (CC_TARGET_PLATFORM != CC_PLATFORM_WP8)
#include <pthread.h>
#else
#include "CCPThreadWinRT.h"
#include <ppl.h>
#include <ppltasks.h>
using namespace concurrency;
#endif

#if CC_TARGET_PLATFORM != CC_PLATFORM_IOS
typedef int socklen_t;
#endif

USING_NS_CC;

struct NetPlayer
{
	int id;
	char* name;
	int profileID;
	//CCString* profilePic;
	//const char* picURL;
	
};

class Ubiquos
{
public:
	static Ubiquos* getInstance();
	
	
	Ubiquos(void);
	~Ubiquos(void);

	void reset();

	bool isConnected();

	void connect(int numPlayers, bool isSpectator);
	void connect(const char* ip,int numPlayers);
	void createServer(int numPlayers,bool isSpectator);

	void sendMessage(const char* msg);
	void sendMessage(const char* msg, int playerNo);

	void addMessage(int playerID, const char* msg);
	
	void Close();

	const char* getMessage();
	
	int getNumPlayersConnected();
	int getPlayerAvatarNumber(int id);
	void setPlayerAvatarNumber(int id,int number);
	int getLocalId();
	void setServerLocalId(int id);
	bool isPlayerConnected(int i);
	bool isServer();
	bool foundServer();
private:
	char tempChar[1024];//120
	char tempChar2[1024];//255
	
	bool busy;
	static Ubiquos* m_instance;
	
	std::thread * serverUDPThread;
	std::thread * serverThread;
	std::thread*  clientThread;
	
#if CC_TARGET_PLATFORM != CC_PLATFORM_WINRT
	std::mutex mtx; 
	std::condition_variable cv;

	ServerUDP* udpServer;

	
	Server* m_server;
	Client* m_client;
#endif

	bool m_HandshakeDone;
	int m_socketID;

	bool m_isServer;
	bool m_hasFoundServer;
	int m_clientNo;
	
	char ipConcat[20];

	NetPlayer playerList[5];
	bool playerConnected[5];

	int m_playerID;

    std::queue<std::string*> msg_queue;

	const char* findServer(int numPlayers, bool isSpectator);
	void processInsideMessages(char parseMsg[30][1024], int playerID);
	
	char parseMsg[30][1024];//char parseMsg[30][20];
	char recvMsgs[8][1024];//char recvMsgs[8][256];

	void enableReceiveSpectator();
};
#endif


/*
* Network.hpp
*
*  Created on: Mar 4, 2016
*      Author: Matt Stone
*     Version: 1
*/

//TODO: BUG - it appears that the second TCP send will repeatedly send the same second packet.

//TODO: setup callbacks on client join using TCP send
//TODO: change arrays into vectors of smart pointers [most code will remain fine]
//-- old and perhaps not valid todos below
//TODO: WORKING: creating UDP broad casts - need to test two clients joining server
//	FIX: fix server sending messages to itself, <believe is fixed now>
//	Fix: pinging being influenced by UDP sends	<pinging is currently disabled>
//TODO: 0B: switch TCP send/receive to UDP (so ping isn't interfered)
//				-use while loop to clear until last packet
//TODO: 1A: make client smart enough to read server delay and calibrate its delay accordingly
//TODO: 1B: create a select-able ip join
//TODO: 3: calibrate server timing to be independent of number of connected clients
//TOTO: 6: SWITCH TCP/UDP pointers to smart pointers (change shutdown method)
//TODO: handle same IP connecting to server (make sure listening doesn't stop, this is how it is set now)
//or re-enable listening if the address (which matches server) leaves. -- i.e. change the flag of listen back to true.
//#ifndef NETWORK_HPP_
//#define NETWORK_HPP_

///<API - USAGE>
///
///<SETUP>
///the user must:
///		1. Provide an ip to connect to (if client, good idea to set the servers ip to self)
///		2. provide a setAsServer(true) or setAsClient(true) to tell the network object how to behave (the default is as a client)
///		3. Obtain a pointer to the mutex stored in the network object - this is done by getMutex()
///			note: This mutex must be locked before setting up input and output channels
///		4. Set the outbound packet pointer of the object to a shared_ptr<sf::packet> --  void setPacketOutboundSource(shared_ptr<sf::Packet>& outbound);
///		5. Set the inbound packet queue (which is a list of shared_ptr<sf::packet> -- void setPacketInboundList(std::list<shared_ptr<sf::Packet>>* listPtr);
///		
///		6. Provide a readyToStart() call to let the server know the thread is ready to begin broadcasting and receiving 
///     *. Provide a shared_ptr to a list of shared ptr packets for input: vector<shared_ptr<list<shared_ptr<sf::Packet>>>> tcpSyncedInQueues;
///				this must be done for every index that a player joins setTcpPacketOutboundSource(tcpOutPacketQueue[0], 0);
///     *. provide a shared_ptr to a lsit of shared_ptr packets for output: vector<shared_ptr<list<shared_ptr<sf::Packet>>>> tcpSyncedOutQueues
///             this must be done for every index that a player joins setTcpPacketInboundList(tcpInPacketQueue[0], 0);		
///		7LAST. safely shutting down the server - the user must call terminateThread() and then wait for the networkStatus() variable to return FALSE - FALSE signifys the thread has died.
#pragma once;

#include <string>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <stack>
#include <thread>
#include <mutex>
#include <chrono>
#include <exception>
#include <memory>
#include <list>



using std::string;
using std::cout;
using std::endl;
using std::lock_guard; 
using std::mutex;
using std::shared_ptr;
using std::list;
using std::vector;

namespace ee {

	class Network final {
	private:
		//State variables
		const static unsigned short MAX_PLAYERS = 8;		//commented out to try and remove a variable implmented in header
		bool killThread = false;
		bool listen = true;
		bool verboseMode = false;
		bool networkReadyToStart = false;
		bool threadSafeStartFlag = false;
		bool server = false;
		bool networkShutdown = false;

		//Internal Setup
		void initializeVars();

		//Packets
		sf::Packet pingPack[MAX_PLAYERS];//may only need 1 of these - depends on final implementation
		sf::Packet inboundPack[MAX_PLAYERS];
		sf::Packet outboundPack[MAX_PLAYERS];

		//Sockets and related variables
		sf::TcpSocket* clientTCP[MAX_PLAYERS];		//consider making these stack dynamic allocated variables
		sf::UdpSocket* clientUDP[MAX_PLAYERS];
		sf::IpAddress clientIP[MAX_PLAYERS];

		sf::TcpListener listener;
		std::stack<int> freeIndeces;

		sf::IpAddress ipServer;
		unsigned short listenPort = 33100;
		unsigned short tcpPort = 33122;
		unsigned short clientUdpPort = 33123;
		unsigned short serverUdpPort = 33124;

		//sockets belonging to this client
		sf::TcpSocket myTcpSocket;
		sf::UdpSocket myUdpSocket;
		sf::SocketSelector mySelector;
		sf::Socket::Status mySocketStatus = sf::Socket::Status::Done;
		sf::Socket::Status pingStatus = sf::Socket::Status::Done;
		bool clientConnected = false;
		bool clientUdpPortBound = false;

		//Timing
		sf::Clock lastPing;
		sf::Clock networkCycleClock;	
		sf::Time serverPauseValue = sf::milliseconds(100);		//1000 <-values tested with
		sf::Time serverRecieveDelay = sf::milliseconds(100);		//100  <-values tested with
		sf::Time clientRecieveDelay = sf::milliseconds(100);		//100  <-values tested with
		sf::Time recPingDelay = sf::milliseconds(100);
		sf::Time pingDelay = sf::milliseconds(3000);

		//Connection Checking (ping - pong)
		int clientMissedPings[MAX_PLAYERS];
		static const short missedPingThreshold = 10;
		//short missedPingThreshold = 10; //temporary statement to find head issue

		///Start - synced packets with outside class
		//UDP sources (goes to all clients)
		shared_ptr<sf::Packet> clientOutPacketSource;				//nullptr
		std::list<shared_ptr<sf::Packet>>* clientReceivedPackets;	//nullptr

		//TCP sources (setup in initialize vars)
		vector<shared_ptr<list<shared_ptr<sf::Packet>>>> tcpSyncedInQueues;		//vector holds client index, list is a queue that the client sets up themself
		vector<shared_ptr<list<shared_ptr<sf::Packet>>>> tcpSyncedOutQueues;
		vector<shared_ptr<sf::Packet>> tcpStagedOutPacket;				//nullptr
		vector<bool> tcpStagedOutPacketBusy;// = false;				//flag for staged packet
		vector<shared_ptr<sf::Packet>> tcpStagedInPacket;				//nullptr
		vector<bool> tcpStagedInPacketBusy;// = false;				//flag for staged packet
		sf::Socket::Status tcpStagedInStatus = sf::Socket::Socket::Done; //flags if packet is partial
		shared_ptr<std::mutex> mu = std::make_shared<mutex>();
		///End - synced packets with outside class

		//testing
		std::string inping;
		std::string outping;
		sf::SocketSelector connectSelect;
		sf::SocketSelector processSelect;

		//Private functions

		//Server and Client methods (ALL METHODS ASSUME LOCK_GUARD HAS ALREADY BEEN SET)
		sf::Socket::Status receiveTcpWithTimeout(sf::TcpSocket& tcpSocket,
			sf::Packet& packet, sf::Time timeout);
		sf::Socket::Status receiveUdpWithTimeout(sf::UdpSocket& udpSocket,
			sf::Packet& packet, sf::Time timeout, sf::IpAddress ip, unsigned short port); //@deprecated
		sf::Socket::Status receiveUdpWithTimeoutQueue(sf::UdpSocket& udpSocket,
			sf::Time timeout, sf::IpAddress ip, unsigned short port);
		
		//Send TCP
		sf::Socket::Status sendTcpAsThread(sf::TcpSocket& tcpSocket, char index);
		bool tcpPacketStaging(char index);
		sf::Socket::Status sendRawTcpNonBlocking(char index);

		//receive tcp
		sf::Socket::Status receiveTcpAsThread(sf::TcpSocket& tcpSocket, char index);
		bool tcpPacketUnload(char index);
		sf::Socket::Status receiveRawTcpNonBlocking(char index);

		//server
		void serverInit();
		void serverListenAndAccept(sf::Time timeout);
		void serverProcess();
		void serverPingClients();
		void serverCollectInput();
		void serverProcessInput();
		void serverPrepareOutput();
		void serverBroadcast();
		void unpackPing(int index);
		void unpack(int index);
		void pack(int index);
		void disconnectClient(int index);
		void serverBindUdp(int targetIndex);


		//clientUDP
		void joinServer(sf::IpAddress targetIP, unsigned short targetPort);
		void clientUdpBind();
		void clientSend();
		void clientPack();		//@deprecated
		void clientReceive();
		void clientUnpack();	//@deprecated
		void clientPing();
		void clientInitialize();

		//shut down server
		void shutdown();

		//Utility (helper) methods
		void clientRunVerboseReport();

		//tests
		void testSelectorListen(int type);


	public:

		Network();
		//Network(std::string ip);
		~Network();

		void waitToRun();
		void setAsServer(bool value = true);
		void setAsClient(bool value = true);

		void serverRun();
		void clientRun();
		void clientRun(string ip);

		//Interfacing outside class with network object
		void setIp(string ip);
		void setPacketOutboundSource(shared_ptr<sf::Packet>& outbound);
		void setPacketInboundList(std::list<shared_ptr<sf::Packet>>* listPtr);
		void setTcpPacketOutboundSource(shared_ptr<list<shared_ptr<sf::Packet>>> outboundPtr, char index = 0);
		void setTcpPacketInboundList(shared_ptr<list<shared_ptr<sf::Packet>>> listPtr, char index = 0);

		shared_ptr<mutex> getMutex();
		void readyToStart();

		//Other Methods
		unsigned short getMaxPlayers();
		
		//Debugging
		void constructorTest();
		void setVerboseMode(bool setValue);
		//void setNewMutex(shared_ptr<std::mutex> newMutex);

		//Ending Server
		void terminateThread();
		bool getNetworkStatus();
	};

} /* namespace EEngine */

//#endif /* NETWORK_HPP_ */

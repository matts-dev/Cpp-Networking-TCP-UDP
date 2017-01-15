/*
* Network.cpp
*
*  Created on: Mar 4, 2016
*      Author: Matt Stone
*     Version: 1
*/

#include "Network.hpp"

namespace ee {

	Network::Network() {
		this->ipServer = sf::IpAddress::getLocalAddress();
		//this->TcpPort = 33122;
		mySocketStatus = sf::Socket::Error;
		for (int i = MAX_PLAYERS - 1; i >= 0; --i) {
			freeIndeces.push(i);
			clientTCP[i] = nullptr;
			clientUDP[i] = nullptr;
		}
		initializeVars();
	}

	/*
	Network::Network(std::string ip) {
		//TODO: ip assignment may cause exception -- look this up when this constructor is starting to be used.
		this->ipServer = ip;
		//this->port = 33122;
		mySocketStatus = sf::Socket::Error;
		for (int i = MAX_PLAYERS - 1; i >= 0; --i) {
			freeIndeces.push(i);
			clientTCP[i] = nullptr;
		}
	}
	*/

	Network::~Network() {
		if (!networkShutdown) {
			for (int i = 0; i < MAX_PLAYERS; i++) {
				delete clientTCP[i];
				delete clientUDP[i];
			}
		}
	}

	///This method waits until the user has provided the network api with a "ready to start" flag.
	///Generally, the user will want to set an IP to join, and perhaps other initialization variables before
	///saying the server is ready to start. 
	/// 
	///This method is designed to be run as a thread, and it uses a mutex to check the thread-safe flag.
	void Network::waitToRun()
	{
		//Pause server running until variables are ready ( //waits for server to start, or waits until thread should die
		while (!networkReadyToStart && !killThread) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			//this loop would keep mutex locked, therefore second boolean value used (similar to master-slave flip-flop logic
			if (verboseMode) {
				cout << "waiting for networkReadyToStart boolean flag" << endl;
			}
			lock_guard<mutex> lock(*mu);
			if (threadSafeStartFlag) { networkReadyToStart = true; }
		}

		//check if client is shutting down
		if (killThread) { shutdown(); return; }

		//Launch server unless game is closing 
		if (server) {
			serverRun();
		}
		else {
			clientRun();
		}

	}

	void Network::setAsServer(bool value)
	{
		server = value;
	}

	void Network::setAsClient(bool value)
	{
		server = !value;
	}

	void Network::serverRun() {
		//Pause server running until variables are ready
		while (!networkReadyToStart) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			//this loop would keep mutex locked, therefore second boolean value used (similar to master-slave flip-flop logic
			lock_guard<mutex> lock(*mu);
			if (threadSafeStartFlag) { networkReadyToStart = true; }
		}

		serverInit();

		while (!killThread) {
			//Record Start Of Cycle
			if (verboseMode) { networkCycleClock.restart(); }

			serverListenAndAccept(sf::seconds(1.0f));
			serverProcess();

			if (verboseMode) { std::cout << "Server Running" << std::endl; }
			sf::sleep(serverPauseValue);

			//detail server timing
			if (verboseMode) {
				std::cout << "server cycle took: " << std::setprecision(5);
				std::cout << networkCycleClock.getElapsedTime().asMilliseconds();
				std::cout << "ms\n" << std::endl;
			}
		}
		shutdown();
	}

	/**
	* Avoid temptation to merge collect, process, prepare, and broadcast loops. They must be separate for ADT logic.
	*/
	void Network::serverProcess() {
		//Ping collect if ready
		//serverPingClients();		//disabled for testing

		//Collect Input
		serverCollectInput();

		//Process Input
		//serverProcessInput();	//now handled by game object

		//Prepare Output
		//serverPrepareOutput(); //now handled by game object

		//Broadcast Output
		serverBroadcast();

	}

	void Network::serverPingClients() {
		if (lastPing.getElapsedTime() > pingDelay) {
			lastPing.restart();
			if (verboseMode) {
				std::cout << "pinging clients" << std::endl;
			}
			for (int i = 0; i < MAX_PLAYERS; i++) {
				if ((clientTCP[i]) != nullptr) {
					//If client exists, check for ping
					receiveTcpWithTimeout(*clientTCP[i], pingPack[i], recPingDelay);
					unpackPing(i);
					if (clientMissedPings[i] > missedPingThreshold) {
						if (verboseMode) {
							std::cout << "client[" << i << "] has missed \"";
							std::cout << clientMissedPings[i] << "\" pings.";
							std::cout << std::endl;
						}
						disconnectClient(i);
					}
				}
			}
		}
	}

	void Network::serverCollectInput() {
		//give tcp socket selecto time to find inbound TCP packets
		mySelector.wait(sf::milliseconds(1));	//1 frame = 16.67 ms (@60fps)

		//collect input from players
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if ((clientTCP[i]) != nullptr) {
				//loads all packets from players into a queue to be added to the game world (should only contain player data)
				lock_guard<mutex> lock(*mu); //TODO: consider implications of moving this outside the loop. 
				
				//handle UDP
				receiveUdpWithTimeoutQueue(*clientUDP[i], serverRecieveDelay, clientIP[i], serverUdpPort);

				//clear out staging area
				tcpPacketUnload(i);

				//handle TCP
				if (mySelector.isReady(*clientTCP[i])) {
					receiveTcpAsThread(*clientTCP[i], i); //EXPERIMENTAL
				}
			}
		}
	}

	void Network::serverProcessInput() {
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (clientTCP[i] != nullptr) {
				unpack(i);
			}
		}
	}

	void Network::unpack(int index) {
		if (inboundPack[index] >> inping) {
			//Success in unpacking
			std::cout << inping << std::endl;
		}
	}

	void Network::unpackPing(int index) {
		if (pingPack[index] >> inping) {
			//Success in unpacking
			if (verboseMode) {
				std::cout << inping << std::endl;
				clientMissedPings[index] = 0;
			}

		}
		else {
			++clientMissedPings[index];
		}
	}

	void Network::serverPrepareOutput() {
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (clientTCP[i] != nullptr) {
				pack(i);
			}
		}
	}
	void Network::pack(int index) {
		outboundPack[index] << "server";
	}

	void Network::serverBroadcast() {
		lock_guard<mutex> lock(*mu);
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if ((clientTCP[i]) != nullptr) {
				//If client exists, do something
				clientUDP[i]->send(*clientOutPacketSource, clientIP[i], clientUdpPort);

				//If there is packed in TCP queue, send one
				if (tcpSyncedOutQueues[i] != nullptr) {
					if (tcpSyncedOutQueues[i]->size() > 0) {
						sendTcpAsThread(*clientTCP[i], i);
					}
				}
			}
		}
		//UDP broadcast complete, clear packet
		clientOutPacketSource->clear();
	}

	void Network::disconnectClient(int index) {
		//ensure valid index
		if (index < 0 || index > MAX_PLAYERS) {
			std::cout << "invalid client remove index: " << index << std::endl;
			return;
		}
		//Check Valid client
		if (clientTCP[index] != nullptr) {
			//restore listening function
			if (!listen) {
				if (clientTCP[index]->getRemoteAddress()
					== sf::IpAddress::getLocalAddress()) {
					listen = true;
					if (verboseMode) {
						std::cout << "listening re-enabled" << std::endl;
					}
				}
			}

			//Remove the client
			mySelector.remove(*clientTCP[index]);
			delete clientTCP[index]; // does this set value to null ptr or leave address?
			clientTCP[index] = nullptr;

			delete clientUDP[index];
			clientUDP[index] = nullptr;

			//reset ping count
			clientMissedPings[index] = 0;

			//restore index to viable join index
			freeIndeces.push(index);
		}
	}

	void Network::serverBindUdp(int targetIndex) {
		if (clientUDP[targetIndex]->bind(serverUdpPort) != sf::Socket::Done) {
			std::cout << "failed binding clientUDP[" << targetIndex << "] to port."
				<< std::endl;
		}
		else {
			if (verboseMode) {
				std::cout << "clientUDP[" << targetIndex << "] bound to port: "
					<< serverUdpPort << '.' << std::endl;
			}
		}
	}

	void Network::clientRun(string ip) {
		//create a default argument in the other function
		this->ipServer = ip; 	//default ip is set during constructor
		clientRun();
	}

	void Network::setIp(string ip)
	{
		try {
			ipServer = ip;
		}
		catch (std::exception e) {
			cout << "failed to change ip in: setIp(string ip)" << endl;
		}
	}

	void Network::setPacketOutboundSource(shared_ptr<sf::Packet>& outbound)
	{
		lock_guard<mutex> lock(*mu);
		this->clientOutPacketSource = outbound;
	}

	void Network::setPacketInboundList(std::list<shared_ptr<sf::Packet>>* listPtr)
	{
		clientReceivedPackets = listPtr;
	}

	///default argument for index is 0
	void Network::setTcpPacketOutboundSource(shared_ptr<list<shared_ptr<sf::Packet>>> outboundPtr, char index)
	{
		//set listPtr
		tcpSyncedOutQueues[index] = outboundPtr;
	}

	///default argument for index is 0
	void Network::setTcpPacketInboundList(shared_ptr<list<shared_ptr<sf::Packet>>> listPtr, char index)
	{
		//set listPtr
		tcpSyncedInQueues[index] = listPtr;
	}

	shared_ptr<mutex> Network::getMutex()
	{
		return mu;
	}

	///This method locks the general network mutex mu
	void Network::readyToStart()
	{
		lock_guard<mutex> lock(*mu);
		threadSafeStartFlag = true;
	}

	unsigned short Network::getMaxPlayers()
	{
		//Lock this mutex as safe measure for variable access
		//lock_guard<mutex> lock(*mu);		//left unlocked incase method that has locked mutex cals this method
		return MAX_PLAYERS;
	}




	void Network::clientRun() {
		//initialize client (blocking tcp, etc.)
		clientInitialize();

		//Pause client running until variables are ready
		while (!networkReadyToStart) {
			//this loop would keep mutex locked, therefore second boolean value used (similar to master-slave flip-flop logic)
			std::this_thread::sleep_for(std::chrono::seconds(1));
			lock_guard<mutex> lock(*mu);
			if (threadSafeStartFlag) { networkReadyToStart = true; }
		}

		//while client should run
		while (!killThread) {
			//reset clock - for cycle time reporting
			networkCycleClock.restart();

			//Determine if client should run or connect
			if (clientConnected) {
				//CLIENT CONNECTED AND RUNNING

				//Check if client should send ping pack
				//clientPing();	//disabled for testing

				//client send next data packet
				clientSend();

				//client receive next data packet
				clientReceive();
			}
			else {
				joinServer(ipServer, listenPort);
			}

			//Pause the client for short time
			sf::sleep(serverPauseValue);

			//Report client cycle status
			if (verboseMode) {
				clientRunVerboseReport();
			}
		}
		shutdown();
	}

	void Network::joinServer(sf::IpAddress targetIP, unsigned short targetPort) {
		mySocketStatus = myTcpSocket.connect(targetIP, targetPort);
		if (mySocketStatus != sf::Socket::Done) {
			//error connecting
			std::cout << "could not connect to server" << std::endl;
		}
		else {
			cout << "client: " << sf::IpAddress::getLocalAddress();
			cout << " connected to server: " << myTcpSocket.getRemoteAddress() << endl;
			clientConnected = true;
			clientUdpBind();
			mySelector.add(myTcpSocket);
		}
	}

	void Network::clientUdpBind() {
		if (myUdpSocket.bind(clientUdpPort) != sf::Socket::Status::Done) {
			cout << "error in UDP binding to port:" << clientUdpPort << endl;
		}
		else {
			clientUdpPortBound = true;
		}
	}

	void Network::clientSend() {

		lock_guard<mutex> lock(*mu);
		
		//handle UDP
		if (clientOutPacketSource != nullptr) {
			//TODO: add check for UDP packet size (there is a max)
			myUdpSocket.send(*clientOutPacketSource, ipServer, serverUdpPort);
		}

		//handle TCP (client uses index 0 to send and receive tcp)
		if (tcpSyncedOutQueues[0] != nullptr) {
			if(tcpSyncedOutQueues[0]->size() > 0){
				sendTcpAsThread(myTcpSocket, 0);
			}
		}
	}

	/*
	@deprecated - game class now handles packing packets, these packets are accessed through memory sharing and mutexes.
	*/
	void Network::clientPack() {
		outboundPack[0] << "client";
	}

	void Network::clientReceive() {
		//myTcpSocket.receive(inboundPack[0]);
		//myUdpSocket.receive(inboundPack[0], ip, UdpPort);
		//receiveUdpWithTimeout(myUdpSocket, inboundPack[0], clientRecieveDelay, ipServer, clientUdpPort);
		//clientUnpack();
		lock_guard<mutex> lock(*mu);	//lock mutex so it is safe to access data

		//handle UDP
		receiveUdpWithTimeoutQueue(myUdpSocket, clientRecieveDelay, ipServer, clientUdpPort);

		//clear out tcp staging area
		tcpPacketUnload(0);

		//handle TCP
		mySelector.wait(sf::milliseconds(1));	//1 frame (at 60fps) is 16.67 ms
		if (mySelector.isReady(myTcpSocket)) {
			receiveTcpAsThread(myTcpSocket, 0);	//0 is the index used for a self-owned client. 
		}
	}

	/*
	@deprecated - unpacking is now handled by the game class
	*/
	void Network::clientUnpack() {
		if (inboundPack[0] >> inping) {
			//Success in unpacking
			std::cout << inping << std::endl;
		}
	}

	void Network::clientPing() {
		//check if last ping packet was partial, and finish it if true
		if (pingStatus == sf::Socket::Status::Partial) {
			if (verboseMode) {
				std::cout << "partial packet, attempting to finish" << std::endl;
				pingStatus = myTcpSocket.send(pingPack[0]);
			}
		}

		//Ping if lapsed time is correct
		if (lastPing.getElapsedTime() > pingDelay) {
			pingPack[0] << "ping";
			pingStatus = myTcpSocket.send(pingPack[0]);
			lastPing.restart();
		}
	}

	void Network::clientInitialize() {
		//myTcpSocket.setBlocking(false);
		myUdpSocket.setBlocking(false);
	}

	void Network::setVerboseMode(bool setValue) {
		this->verboseMode = setValue;
	}

	void Network::terminateThread() {
		killThread = true;
	}

	/*
		Returns a value if network is still running (this is only flagged to false when shutdown() is called.
	*/
	bool Network::getNetworkStatus()
	{
		return !networkShutdown;
	}

	void Network::initializeVars()
	{
		//set TCP channels to null for every player index
		for (unsigned int i = 0; i < MAX_PLAYERS; ++i) {
			//Set up TCP out
			tcpSyncedOutQueues.push_back(nullptr);		//nullptr;
			tcpStagedOutPacket.push_back(nullptr);		//nullptr
			tcpStagedOutPacketBusy.push_back(false);	//flag for staged packet

			//Set up TCP In
			tcpSyncedInQueues.push_back(nullptr);		//nullptr
			tcpStagedInPacket.push_back(nullptr);		//nullptr
			tcpStagedInPacketBusy.push_back(false);	//flag for staged packet

		}
	}

	/*
	* Attempts to receive a packet on a given tcpSocket, but only for the given timeout duration
	*/
	sf::Socket::Status Network::receiveTcpWithTimeout(sf::TcpSocket& tcpSocket,
		sf::Packet& packet, sf::Time timeout) {
		sf::SocketSelector selector;
		selector.add(tcpSocket);
		if (selector.wait(timeout)) {
			if (selector.isReady(tcpSocket)) {
				return tcpSocket.receive(packet);
			}
			else {
				return sf::Socket::Status::NotReady;
			}
		}
		else {
			return sf::Socket::Status::NotReady;
		}
	}

	sf::Socket::Status Network::receiveUdpWithTimeout(sf::UdpSocket& udpSocket,
		sf::Packet& packet, sf::Time timeout, sf::IpAddress ip, unsigned short port) {
		sf::SocketSelector selector;	//make this a static variable?
		selector.add(udpSocket);
		if (selector.wait(timeout)) {
			if (selector.isReady(udpSocket)) {
				//TODO: loop until last packet received? ensures up-to-date
				return udpSocket.receive(packet, ip, port);
			}
			else {
				return sf::Socket::Status::NotReady;
			}
		}
		else {
			return sf::Socket::Status::NotReady;
		}
	}

	//A version of receiving UDP that uses the inbound queue system. 
	//This method does not call a lock gaurd, therefore lockguard should be called out of the scope of this method.
	sf::Socket::Status Network::receiveUdpWithTimeoutQueue(sf::UdpSocket & udpSocket, sf::Time timeout, sf::IpAddress ip, unsigned short port)
	{
		sf::SocketSelector selector;	//make this a static variable?
		selector.add(udpSocket);
		if (selector.wait(timeout)) {
			sf::Socket::Status result = sf::Socket::Status::NotReady;
			if (selector.isReady(udpSocket)) {
				//TODO: loop until last packet received? ensures up-to-date
				shared_ptr<sf::Packet> packetToAdd = std::make_shared<sf::Packet>();
				result = udpSocket.receive(*packetToAdd, ip, port);
				if (result == sf::Socket::Status::Done) {
					clientReceivedPackets->push_back(packetToAdd);
				}
			}
			return result;
		}
		else {
			return sf::Socket::Status::NotReady;
		}

	}

	///ASSUMES LOCKGAURD ALREADY MET
	bool Network::tcpPacketStaging(char index)
	{
		//check if packet is in queue
		int queueSize = tcpSyncedOutQueues[index]->size();	//get the size of the queue
		if (queueSize > 0) {
			//check if packet already staged
			if (tcpStagedOutPacketBusy[index]) { return false; }		//return false if the stagedBusyFlag is true, something's already being sent
			if (tcpStagedOutPacket[index] != nullptr) { return true; }	//return if there is already a staged packet, if it is not busy then it is about to be sent.

			///At this point:<1>an out packet is not staged, <2> there is packet in queue, <therefore> stage the front packet.
			//set the staged to the front of the queue
			tcpStagedOutPacket[index] = tcpSyncedOutQueues[index]->front();	//VECTOR[X] = SHARED<list>, list->x = packet	
																			//get rid of the front
			tcpSyncedOutQueues[index]->pop_front();		//lockguard will be released later
			return true;
		}
		return false;	//blanket false return
	}

	///This method will broadcast the next TCP msg in queue
	///ASSUMES LOCKGAURD MET
	sf::Socket::Status Network::sendTcpAsThread(sf::TcpSocket & tcpSocket, char index)
	{
		//check if staged is busy
		if (tcpStagedOutPacketBusy[index]) { return sf::Socket::Status::NotReady; }

		//Staged area is not busy, prepare sending of TCP packet
		bool readyToSend = tcpPacketStaging(index);

		//if staging returned true, then a packet should be sent
		if (readyToSend) {	//will not be flagged true if busyFlag is set
			tcpStagedOutPacketBusy[index] = true;
			sf::Socket::Status(ee::Network::*fptrSendTCP) (char index) = &ee::Network::sendRawTcpNonBlocking;
			std::thread tcpSendThread(fptrSendTCP, this, index);
			tcpSendThread.detach();	//this is free and must complete on its own -- when it is complete stagingBusy variable will become false
		}

		return sf::Socket::Status::Done;	//this is a misleading "status::done", the thread will determine when the packet is sent
	}

	///THIS SHOULD BE CALLED IN A THREAD - SENDS WILL CAUSE PAUSING UNTIL THEY'RE COMPLETE
	///The TCP staging area belongs to this method.
	sf::Socket::Status Network::sendRawTcpNonBlocking(char index)
	{
		///loop until partial is complete
		sf::Socket::Status sendStatus = sf::Socket::Status::Partial;	//NOTE: partial is only returned if in non-blocking
		int reattemptsLeft = 3;	//if an error occurs, it will try and send the packet again this many times.
		
		while (sendStatus == sf::Socket::Status::Partial || sendStatus == sf::Socket::Status::Error && reattemptsLeft > 0) {
			//send the appropriate staged packet

			//The socket sent to depends on the configuration. Client uses connections through myTcpSocket.
			if (server) { sendStatus = clientTCP[index]->send(*(tcpStagedOutPacket[index])); }
			else { sendStatus = myTcpSocket.send(*(tcpStagedOutPacket[index])); }

			if (sendStatus == sf::Socket::Status::Error) {
				cout << "there was an error flag in sending TCP, will repeat: " << --reattemptsLeft << " times." << endl;

			}
		}

		///clean up resources so that another TCP msg can be sent
		tcpStagedOutPacket[index] = nullptr;
		tcpStagedOutPacketBusy[index] = false;
		cout << "TCP thread send Done: " << index << endl;
		if (reattemptsLeft <= 0) { cout << index << " failed to send, packet was abandoned @ sendRawTcpNonBlocking(char index)" << endl; }
		return sendStatus;
	}
	/*
		This method should only be called if a socket selecter has already identified that
		the tcp socket for this index is ready to recieve a packet.

	*/
	sf::Socket::Status Network::receiveTcpAsThread(sf::TcpSocket & tcpSocket, char index)
	{
		//check if staged is busy
		if (tcpStagedInPacketBusy[index]) { return sf::Socket::Status::NotReady; }

		//Staged area is not busy, prepare sending of TCP packet
		bool readyToReceive = tcpPacketUnload(index);

		if (readyToReceive) {	//will not be flagged true if busyFlag is set
			tcpStagedInPacketBusy[index] = true;
			sf::Socket::Status(ee::Network::*fptrReceiveTCP) (char index) = &ee::Network::receiveRawTcpNonBlocking;
			std::thread tcpReceiveThread(fptrReceiveTCP, this, index);
			tcpReceiveThread.detach();	//this is free and must complete on its own -- when it is complete stagingBusy variable will become false
		}
		if (readyToReceive) { return sf::Socket::Status::Done; }	//this is a misleading "status::done", the thread will determine when the packet is sent
		else { return sf::Socket::Status::NotReady; } //socket wasn't ready, this lets the caller of the function know
	}

	/*
	method assumes mu is locked

	This method unloads any packet from the staging area into the queue for the game to process.
	@return
		returns true if the process may proceed with collecting a new packet
	*/
	bool Network::tcpPacketUnload(char index)
	{
		//if staged is busy, then there is a process already going on
		if (tcpStagedInPacketBusy[index]) { return false; }

		//if staged packet is nullptr, then return true because there is no packet to unload
		if (!tcpStagedInPacket[index]) { return true; }

		//unload the packet
		else {
			//push a sptr(packet) into vector of sptr(lists) of sptr(packets)
			//tcpSyncedInQueues[index]->push_back(tcpStagedInPacket[index]); //I expanded this out for explicity
			auto list = tcpSyncedInQueues[index];
			auto packet = tcpStagedInPacket[index];
			list->push_back(packet);

			//remove staged
			tcpStagedInPacket[index] = nullptr;

			//packet extracted
			return true;
		}
	}

	sf::Socket::Status Network::receiveRawTcpNonBlocking(char index)
	{
		///loop until partial is complete
		sf::Socket::Status receiveStatus = sf::Socket::Status::Partial;
		if (tcpStagedInPacket[index] == nullptr) { tcpStagedInPacket[index] = std::make_shared<sf::Packet>(); }

		int reattemptsLeft = 3;	//if an error occurs, it will try and send the packet again this many times.
		while (receiveStatus == sf::Socket::Status::Partial || receiveStatus == sf::Socket::Status::Error && reattemptsLeft > 0) {

			//receive the appropriate into the correct staged packet
			if (server) { receiveStatus = clientTCP[index]->receive(*(tcpStagedInPacket[index])); }  //SERVER RECEVE USES clientTCP[]
			else {receiveStatus = myTcpSocket.receive(*(tcpStagedInPacket[index]));}                 //CLIENT RECEIVE USES myTcpSocket

			if (receiveStatus == sf::Socket::Status::Error) {
				cout << "there was an error flag in sending TCP, will repeat: " << --reattemptsLeft << " times." << endl;
			}
		}

		///clean up resources so that another TCP msg can be sent
		tcpStagedInPacket[index] == nullptr;
		tcpStagedInPacketBusy[index] = false;
		if (verboseMode) { cout << "TCP receive thread done: " << index << endl; }
		if (reattemptsLeft <= 0) { cout << index << " failed to send, packet was abandoned @ receiveRawTcpNonBlocking(char index)" << endl; }

		return receiveStatus;
	}


	///If additional initialization is needed, it is done here. 
	void Network::serverInit()
	{

	}

	/**
	*
	* @precondition - freeIndeces contains index locations
	*/
	void Network::serverListenAndAccept(sf::Time timeout) {

		if (listen) {
			// Verbose state reporting
			if (verboseMode) {
				std::cout << "Listen Phase" << std::endl;
			}

			//Initialize Selector
			sf::SocketSelector acceptorSelector;
			listener.listen(listenPort);

			//Socket selector to prevent blocking - it is NOT a container.
			acceptorSelector.add(listener);

			//Check if sockets associated with selector are waiting.
			if (acceptorSelector.wait(timeout)) {

				//Check if socket waiting is the listener.
				if (acceptorSelector.isReady(listener)) {

					//Listen Success path.
					if (verboseMode) {
						std::cout << "accept phase" << std::endl;
					}

					//Pick a target socket for client from stack (pop if success later).
					int targetIndex = freeIndeces.top();

					//Clear socket (ensures empty location).
					delete clientTCP[targetIndex];
					delete clientUDP[targetIndex];

					//Initialize TCP socket.
					clientTCP[targetIndex] = new sf::TcpSocket;


					//Attempt to Accept(bind) socket. Should not block due to socketselector condition.
					if (listener.accept(*(clientTCP[targetIndex]))
						!= sf::Socket::Done) {
						//Bind failure (stack was not popped)
					}
					else {
						//Bind success
						clientIP[targetIndex] = clientTCP[targetIndex]->getRemoteAddress();
						mySelector.add(*clientTCP[targetIndex]);
						//set up UDP
						clientUDP[targetIndex] = new sf::UdpSocket;
						clientUDP[targetIndex]->setBlocking(false);
						serverBindUdp(targetIndex);

						//remove the index from list used.
						freeIndeces.pop();
						cout << "User:" << clientIP[targetIndex] << " Connected To Server" << endl;

						//Self connect check -- this may not cause an issue, but is necessary as of 3/27/16
						if (clientTCP[targetIndex]->getRemoteAddress()
							== sf::IpAddress::getLocalAddress()) {
							std::cout
								<< "Client and Server IP addresses are identical - listening disabled to prevent binding error"
								<< std::endl;
							listen = false;
							//
						}
					}
				}
				else {
					//Listen Failure path
				}
			}
			//stop listener for the next phase.
			listener.close();
		}
	}

	void Network::constructorTest() {
		std::cout << ipServer << " " << std::endl;
	}

	void Network::shutdown() {
		//Delete TCP && UDP client sockets
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			delete clientTCP[i];
			delete clientUDP[i];
		}
		networkShutdown = true;

	}

	//Utility (helper) methods
	void Network::clientRunVerboseReport() {
		std::cout << "Client Running" << std::endl;
		std::cout << "Client cycle: ";
		std::cout << networkCycleClock.getElapsedTime().asMilliseconds();
		std::cout << "ms" << std::endl;
	}

	/*
	* 0 = server, 1 = client
	*/
	void Network::testSelectorListen(int type) {
		int xControlVariable = 0;
		//SERVER
		if (type == 0) {
			int counter = 0;

			//Demonstrates that .listen() does not cause program halt
			std::cout << "listen start" << std::endl;
			listener.listen(33122);
			connectSelect.add(listener);
			std::cout << "listen initiated" << std::endl;

			while (!killThread) {
				//ListenPhase
				//std::cout<<"Type something to check the SocketSelector"<<std::endl;
				//std::cin>>xControlVariable;

				if (connectSelect.wait(sf::seconds(10.0f))) {

					if (connectSelect.isReady(listener)) {
						std::cout << "client[] to be set" << std::endl;
						clientTCP[counter] = new sf::TcpSocket;
						std::cout << "client[] is set" << std::endl;
						listener.accept(*clientTCP[counter]);
						if (counter < MAX_PLAYERS)
							counter++;
					}
				}
				else {
					//Time out
					std::cout << "false for selector.wait() function" << std::endl;
				}
				sf::sleep(sf::milliseconds(1000));
			}
			delete clientTCP[0];
			std::cout << "server dying" << xControlVariable << std::endl;
		}
		//CLIENT
		else {
			while (!killThread) {
				if (!clientConnected) {
					joinServer(ipServer, tcpPort);
				}
				std::cout << "Client Running" << std::endl;
				sf::sleep(sf::milliseconds(1000));
			}
		}

	}


}/* namespace EEngine */

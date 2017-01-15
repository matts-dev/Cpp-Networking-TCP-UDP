#include <iostream>
#include <limits>
#include <memory>
#include <SFML\System.hpp>
#include <thread>

#include "Network.hpp"
using namespace std;

int NetworkTest_main(void) {
	cout << "NetworkTest_main(void)" << endl;
	cout << "server(1) client (2) manual client(3)" << endl;
	int x = 0;
	cin >> x;

	//UDP
	shared_ptr<sf::Packet> outboundPacket = make_shared<sf::Packet>();
	list<shared_ptr<sf::Packet>> inPacketQueue;

	//TCP
	vector<shared_ptr<list<shared_ptr<sf::Packet>>>> tcpOutPacketQueue;
	vector<shared_ptr<list<shared_ptr<sf::Packet>>>> tcpInPacketQueue;

	shared_ptr<ee::Network> networkObj = make_shared<ee::Network>();
	//networkObj->setVerboseMode(true);
	networkObj->setIp("10.0.0.2");
	networkObj->setPacketOutboundSource(outboundPacket);
	networkObj->setPacketInboundList(&inPacketQueue);
	shared_ptr<mutex> mu = networkObj->getMutex();

	///Run The Client
	if (x == 2) {
		networkObj->setAsClient();

		tcpOutPacketQueue.clear(); tcpInPacketQueue.clear();	//clear the lists to enure push-backs match indeces
		//set new lists
		tcpOutPacketQueue.push_back(make_shared<list<shared_ptr<sf::Packet>>>());	//make an out list (should be at zero already;
		tcpInPacketQueue.push_back(make_shared<list<shared_ptr<sf::Packet>>>());	//make an in list

																					//add Tcp lists to the network
		networkObj->setTcpPacketOutboundSource(tcpOutPacketQueue[0], 0);
		networkObj->setTcpPacketInboundList(tcpInPacketQueue[0], 0);

		networkObj->readyToStart();

	}
	///Run The Server
	else if (x == 1) {
		networkObj->setAsServer();
		int maxPlayers = networkObj->getMaxPlayers();
		tcpOutPacketQueue.clear(); tcpInPacketQueue.clear();	//clear the lists to enure push-backs match indeces
		for (int i = 0; i < maxPlayers; ++i) {
			//setup games the TCP channels for the server.

			//set new lists
			tcpOutPacketQueue.push_back(make_shared<list<shared_ptr<sf::Packet>>>());	//make an out list
			tcpInPacketQueue.push_back(make_shared<list<shared_ptr<sf::Packet>>>());	//make an in list

			//add Tcp lists to the network
			networkObj->setTcpPacketOutboundSource(tcpOutPacketQueue[i], i);
			networkObj->setTcpPacketInboundList(tcpInPacketQueue[i], i);
		};

		networkObj->readyToStart();			//method locks mutex
	}
	///Manual IP join
	else if (x == 3) {
		cout << "enter a ip manually" << endl;
		string ip;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		getline(cin, ip);

		networkObj->setIp(ip);
		networkObj->setAsClient(true);


		tcpOutPacketQueue.clear(); tcpInPacketQueue.clear();	//clear the lists to enure push-backs match indeces
		//set new lists
		tcpOutPacketQueue.push_back(make_shared<list<shared_ptr<sf::Packet>>>());	//make an out list (should be at zero already;
		tcpInPacketQueue.push_back(make_shared<list<shared_ptr<sf::Packet>>>());	//make an in list
		//add Tcp lists to the network
		networkObj->setTcpPacketOutboundSource(tcpOutPacketQueue[0], 0);
		networkObj->setTcpPacketInboundList(tcpInPacketQueue[0], 0);

		networkObj->readyToStart();
	}
	else {
		return 0;
	}

	void (ee::Network::*networkProcess)() = &ee::Network::waitToRun; //fptor for thread obj
	thread networkThread(networkProcess, networkObj.get());			 //thread object
	networkThread.detach();

	bool runLoop = true;
	bool sendOnce = true;
	bool openOnceServ = true;
	bool openOnceClient = true;

	while (runLoop) {
		///Reset controls
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
			sendOnce = true;
			openOnceClient = true;
			openOnceServ = true;
		}

		
		///send a TCP packet to the server
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
			shared_ptr<sf::Packet> tcpTestPacket = make_shared<sf::Packet>();
			*tcpTestPacket << "Hello World - I came from a TCP packet!!!!";
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::C) && (x == 2 || x == 3) && sendOnce) { //the client will broadcast this to server
					cout << "client sending tcp " << endl;
					lock_guard<mutex> lock(*mu);
					tcpOutPacketQueue[0]->push_back(tcpTestPacket);	//[0] retrives a listptr, -> uses the pointer to pushback on the queue
					sendOnce = false;
				}
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && x == 1 && sendOnce) { //The server will broadcast this to first client
					cout << "server sending tcp " << endl;
					lock_guard<mutex> lock(*mu);
					tcpOutPacketQueue[0]->push_back(tcpTestPacket);	//[0] retrives a listptr, -> uses the pointer to pushback on the queue
					sendOnce = false;
				}
			}
		};

		///Open a TCP packet (if there is one)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O)) {			
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::C) && (x == 2 || x == 3) && openOnceClient) { //the client will open
				cout << "client opening tcp attempt" << endl;
				lock_guard<mutex> lock(*mu);
				//open if there is something now in the queue
				if (tcpInPacketQueue[0]->size() > 0){
					string msg = "";
					*(tcpInPacketQueue[0]->front()) >> msg;
					cout << msg;
					tcpInPacketQueue[0]->pop_front();
				}
				openOnceClient = false;
					
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && x == 1 && openOnceServ) { //The server open from first client
				cout << "server opening tcp attepmt" << endl;
				lock_guard<mutex> lock(*mu);
				//open if there is something now in the queue
				if (tcpInPacketQueue[0]->size() > 0) {
					string msg = "";
					*(tcpInPacketQueue[0]->front()) >> msg;
					cout << msg;
					tcpInPacketQueue[0]->pop_front();
				}
				openOnceServ = false;
			}
		}

		///send a UDP packet to the server
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U)) {};

		///Close Out the server
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
			networkObj->terminateThread();
			while (networkObj->getNetworkStatus()) {
				this_thread::sleep_for(chrono::seconds(1));
			}
			runLoop = false;	//break out of the inifinite true loop
		}

	}
	return 0;
}

#pragma once
#include<SFML/Window.hpp>
#include<SFML/Audio.hpp>
#include<SFML/Graphics.hpp>
#include<SFML/System.hpp>
#include<SFML/Network.hpp>

#include<string>
#include<sstream>
#include<iostream>
#include<vector>
#include<memory>
#include<map>
#include<utility>	//for pair
#include<list>

#include"Actor.hpp"
#include"Cell.hpp"
#include"KeyBind.hpp"
#include"Flag.hpp"
#include"Network.hpp"
#include"Player.hpp"
#include"ClientState.hpp"
#include"ServerState.hpp"


using namespace std;

/*
	The Game Class.
	-Centralized access to variables
*/
namespace ee {
	class Game
	{
	private:
		///Networking Related
		sf::Uint8 gameid = 0;
		ClientState clientState;
		ServerState serverState;
		map<string, Actor*> actorMap;
		bool server = true;		//true = server, false = client
		list<shared_ptr<sf::Packet>> inPacketQueue;				//simple broadcast model
		shared_ptr<sf::Packet> outboundPacket = make_shared<sf::Packet>();			//packet used both as server and client		
		shared_ptr<sf::Packet> clientInboundPacket;				
		bool connectedServer = false;
		
		//list<shared_ptr<sf::Packet>> outPacketQueue;			//simple broadcast model
		vector<shared_ptr<sf::Packet>> serverOutboundPackets;	//not used - for unique packet server model

		shared_ptr<ee::Network> network;
		shared_ptr<mutex> networkMutex;
		bool networkLaunched = false;

		///Window Related
		int renderWindowHeight = 600;
		int renderWindowWidth = 1200;
		bool windowShouldClose = false;

		///Actor, player(s), and view related
		shared_ptr<ee::Actor> player;
		shared_ptr<ee::Actor> controlTarget;
		unique_ptr<sf::View> controlView = make_unique<sf::View>(
			sf::FloatRect(0.0f, 0.0f, (float)renderWindowWidth, (float)renderWindowHeight));
		vector<shared_ptr<Actor>> playerList;	
		map<sf::Uint32, shared_ptr<Cell>> playerCellLookupMap;		//<id, cell>

		///IO
		unique_ptr<ee::KeyBind> kb = make_unique<KeyBind>();

		///Cell Related
		map<string, shared_ptr<Cell>> cellMap;
		shared_ptr<Cell> activeDrawCell = nullptr;
		void makeNewCell(string& gridCoords);

	public:
		Game();
		virtual ~Game();
		///Main Functions
		void gameLogic(sf::RenderWindow& window);
		void gameIO();
		void gameDraw(sf::RenderWindow& window);

		///Game Setup
		void initializeVars();
		void updateViewportToControlTarget(sf::RenderWindow& window);
		void setupNetwork();
		void setupCell();
		void setupPlayers();
		

		///Window Access
		int getWindowHeight();
		int getWindowWidth();

		///IO related
		void scanPlayerMoveKeybinds();
		void gameEscapeMenuScan();
		void systemKeyScan();

		///Logic Related
		void addPlayerCallback(int index);
		void removePlayerCallback(int index);
		void addPlayerToPlayerList(shared_ptr<Actor>& player, shared_ptr<Cell>& containingCell);
		sf::Uint8 makeNewPlayer(shared_ptr<Cell> startCell = nullptr, sf::Uint8* id = nullptr);

		///Network Related
		void sendDataToNetwork();		//organizer function
		void collectDataFromNetwork();	//organizer function

		void clientSend();				//organizer function
		void serverSend();				//organizer function
		void echoSend();				//organizer function

		void unpackInpacketQueue();		//helper function
		bool networkValid();			//helper function
		void clientSendSysMsg();		//helper function
		void clientRequestPlayerID();	//send sys msg helper function
		void clientSendPlayerData();	//send sys msg helper function

		//Sending entire game data
		friend sf::Packet& operator<< (sf::Packet& packet, ee::Game& game); //packages whole game
		friend sf::Packet& operator>> (sf::Packet& packet, ee::Game& game);

		void serverSendSysMsg();
		void serverCheckJoiningPlayers();

		void handleCellUnpacking(sf::Packet& packet);
		void handlePlayerIdRequest(sf::Packet& packet);
		void handlePlayerUnpacking(sf::Packet& packet);
		void handleServerAcceptedPlayerIdRequest(sf::Packet& packet);
		void handleClientAcceptingPlayerID(sf::Packet& packet);
		void handlePlayerJoining(sf::Packet& packet);
		void packPlayerList(sf::Packet& packet);
		void analyzePacketNumberAndRemoveExcess();

		shared_ptr<ee::Network> getNetwork();
		void forceNetworkShutdown();
		///End Network Functions


		sf::Uint8 getId();
	};

}; //end namespace ee


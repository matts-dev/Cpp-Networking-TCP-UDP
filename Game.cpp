#include "Game.hpp"




/*
TODO: untested

@param gridCoords
string in the form of "2 -10" that represents the "x y" of the cell.
*/
void ee::Game::makeNewCell(string& gridCoords)
{
	stringstream ss(gridCoords);
	int xCoord;
	int yCoord;
	ss >> xCoord;
	ss >> yCoord;

	//check if cell already exists
	if (cellMap[gridCoords] != nullptr) {
		//cell exists! do not overwrite!
		return;
	}
	else {
		//Cell did not exist, proceed with creation
		cellMap[gridCoords] = make_shared<Cell>(xCoord, yCoord);
	}
}

ee::Game::Game()
{
}


ee::Game::~Game()
{
	network->terminateThread();
}

void ee::Game::gameLogic(sf::RenderWindow& window)
{
	//Collect Data From Server
	collectDataFromNetwork();

	//Game Work
	controlTarget->rotateToMouse(window);
	gameIO();

	//Send Data To Server
	sendDataToNetwork();
}

void ee::Game::gameIO()
{
	scanPlayerMoveKeybinds();
	gameEscapeMenuScan();
	systemKeyScan();
}

void ee::Game::gameDraw(sf::RenderWindow& window)
{
	//check if window should close
	if (windowShouldClose) {
		window.close();
	}

	window.clear();

	// Pre-Drawing Actions (setting up views)
	if (controlTarget) {
		updateViewportToControlTarget(window);
	}
	//window.setView(*controlView); //disabled temporarily for network testing

	activeDrawCell->drawLayer5(window);

	window.display();
}

void ee::Game::initializeVars()
{
	//Make The Client And Server objects (this should be done first)
	setupNetwork();

	//Make the starting cell (and set it to the active cell)
	setupCell();

	//Make Players
	setupPlayers();
}

void ee::Game::updateViewportToControlTarget(sf::RenderWindow & window)
{
	if (controlTarget) {
		controlView->setCenter(controlTarget->getSpriteWeakPtr().lock()->getPosition());
	}
}

void ee::Game::setupNetwork()
{
	network = make_shared<ee::Network>();
	network->setIp("10.0.0.2");
	network->setPacketOutboundSource(outboundPacket);
	network->setPacketInboundList(&inPacketQueue);
	network->setVerboseMode(true);
	networkMutex = network->getMutex();

	//Reserve slots for outbound packets
	for (int i = 0; i < network->getMaxPlayers(); ++i) {
		serverOutboundPackets.push_back(nullptr);
	}


}

void ee::Game::setupCell()
{
	shared_ptr<Cell> startCell = make_shared<Cell>(100, 0);
	cellMap.insert({ startCell->toString(), startCell });
	activeDrawCell = startCell;
}

/*
Requires that active draw cell is not null
*/
void ee::Game::setupPlayers()
{
	if (activeDrawCell) {
		//Make The Player and add it to a cell
		player = make_shared<ee::Actor>();
		controlTarget = player;
		player->makeSprite();
		player->move(200, 200);
		activeDrawCell->addActor(player->idStr(), player);
		addPlayerToPlayerList(player, activeDrawCell);

		//Make A Shadow and add it to the cell
		shared_ptr<Actor> shadow = make_shared<Actor>();
		shadow->makeSprite();
		shadow->getSpriteWeakPtr().lock()->move(500, 500);
		activeDrawCell->addActor(shadow->idStr(), shadow);
		addPlayerToPlayerList(shadow, activeDrawCell);
	}
}

int ee::Game::getWindowHeight()
{
	return renderWindowHeight;
}

int ee::Game::getWindowWidth()
{
	return renderWindowWidth;
}



void ee::Game::scanPlayerMoveKeybinds()
{
	//check up situations
	if (kb->upPressed()
		&& !kb->downPressed()) {

		if (kb->rightPressed()
			&& !kb->leftPressed()) {
			//UP RIGHT MOVEMENT
			controlTarget->moveUR();
		}
		else if (kb->leftPressed()
			&& !kb->rightPressed()) {
			//UP LEFT MOVEMENT
			controlTarget->moveUL();
		}
		else {
			// MOVE UP
			controlTarget->moveUp();
		}
	}

	//check down situations
	else if (kb->downPressed()
		&& !kb->upPressed()) {
		if (kb->rightPressed()
			&& !kb->leftPressed()) {
			//DOWN RIGHT MOVEMENT
			controlTarget->moveDR();
		}
		else if (kb->leftPressed()
			&& !kb->rightPressed()) {
			//DOWN LEFT MOVEMENT
			controlTarget->moveDL();
		}
		else {
			// MOVE DOWN
			controlTarget->moveDown();
		}
	}

	//check right (all up and down situations already considered)
	else if (kb->rightPressed()) {
		// move right
		controlTarget->moveRight();
	}

	//check left (all up and down situations already considered)
	else if (kb->leftPressed()) {
		//move left
		controlTarget->moveLeft();
	}
}

void ee::Game::gameEscapeMenuScan()
{
	if (kb->escapePressed()) {
		windowShouldClose = true;
	}
}

void ee::Game::systemKeyScan()
{
	static bool allowOtherNetworkObj = true;
	//check if the system modifier is pressed
	if (kb->systemPressed()) {
		//check if client start button pressed in addition to system key
		if (kb->startClientPressed() && allowOtherNetworkObj && networkMutex) {
			network->setAsClient(true);
			network->readyToStart();
			networkLaunched = true;
			allowOtherNetworkObj = false;
			server = false; // flags that this game client is acting as a client only
		}
		//check if server start button is pressed in addition to system key
		if (kb->startServerPressed() && allowOtherNetworkObj && networkMutex) {
			network->setAsServer(true);
			network->readyToStart();
			networkLaunched = true;
			allowOtherNetworkObj = false;
			server = true;	//flags that this game client is acting as a server
		}
	}
}

void ee::Game::addPlayerCallback(int index)
{
	//create player with index
	shared_ptr<ee::Actor> newPlayer = make_shared<ee::Actor>(index);

	//check if player already exists in player list
	for (unsigned int i = 0; i < playerList.size(); ++i) {
		if (playerList[i] && playerList[i]->getId() == index) { //check if null, then check if player matches id
			std::cout << "ERROR - PLAYER INDEX ALREADY FOUND: OVER WRITING PREVIOUS PLAYER"
				<< "This message should never be seen, if it is please report this error" << endl;
		}
	}

	//add player to player list
	for (unsigned int i = 0; i <= playerList.size(); ++i) {
		//check if we're past the range of the playerList
		if (playerList.size() == i) {
			playerList.push_back(newPlayer);
			return;
		}
		else {
			if (!playerList[i]) {
				//playerList[i] is nullptr, let's add the player here.
				playerList[i] = newPlayer;
				return;
			}
		}
	}
}

void ee::Game::removePlayerCallback(int index)
{
	//Scan the player list for the player
	for (unsigned int i = 0; i < playerList.size(); ++i) {
		if (playerList[i] != nullptr && playerList[i]->getId() == index) {
			playerList[i] = nullptr;
			return;
		}
	}
}

void ee::Game::addPlayerToPlayerList(shared_ptr<Actor>& player, shared_ptr<Cell>& cell)
{
	//check if network is nullptr and if cell is a nullptr
	if (network && cell) {
		size_t listSize = playerList.size(); 
		size_t maxPlayers = network->getMaxPlayers();		//this method depends on mutex locking, and will wait for it to free
		//Check if there is room to add another player
		if (listSize < maxPlayers) {
			//There is room in list, add the player
			playerList.push_back(player);
			playerCellLookupMap[player->getId()] = cell;
		}
	}

	else {
		std::cout << "error, network object is null; assuming max player count is 8" << std::endl;
		if (playerList.size() < 8 && cell) {
			playerList.push_back(player);
			playerCellLookupMap[player->getId()] = cell;
		}
	}
}

sf::Uint8 ee::Game::makeNewPlayer(shared_ptr<Cell> startCell, sf::Uint8* targetId)
{
	//Host flag (host/servers have freedom to assign what ever ID they like to the new player)
	bool serverFlag = false;

	//if no id pointer was provided, then this is the server making a new player.
	if (targetId == nullptr) { serverFlag = true;}

	//Make the player pointer
	shared_ptr<Actor> newPlayer;	//is nullptr at this point

	/// This is the server's branch of making a player
	if (serverFlag) {
		//check if there is a player obj that is not in use
		for (unsigned int i = 0; i < playerList.size(); ++i) {
			if (!playerList[i]->isPlayer()) {
				newPlayer = playerList[i];
				break;
			}
		}
		///Using already made player that was never used and found in above loop
		if (newPlayer) {
			return static_cast<sf::Uint8>(newPlayer->getId());	//player id is within 8 bits, so it is okay to downcast
		}
		///using brand new player since no players were able to be recycled
		else {
			newPlayer = make_shared<Actor>();
			if (startCell == nullptr) { startCell = cellMap["0 0"]; }
			addPlayerToPlayerList(newPlayer, startCell);
			newPlayer->setId(static_cast<sf::Uint32>(playerList.size()));
			return static_cast<sf::Uint8>(playerList.size());
		}
	}
	///Client's branch for making a new player
	else {
		newPlayer = make_shared<Actor>();
		if (startCell == nullptr) { startCell = cellMap["0 0"]; }
		addPlayerToPlayerList(newPlayer, startCell);
		newPlayer->setId(*targetId);
		return static_cast<sf::Uint8>(newPlayer->getId());
	}

}

/*
	Will send data to network; if not connected, it will echo data to and reset its own packet.
	Primary purpose of this function is sorting whether to make outbound packet as:
	server, client, or not connected (echoing - acts as server but sends packets to self)
*/
void ee::Game::sendDataToNetwork()
{
	//broadcast packet
	if (networkValid()) { ///CONNECTED TO SERVER
		lock_guard<mutex> lock(*networkMutex);
		outboundPacket->clear();	//clear packet
		//Package All Data
		if (server) {
			/*SERVER*/
			serverSend();
		}
		else {
			/* CLIENT */
			clientSend();
		}
		return;	//end lockguard
	}
	else {///NOT CONNECTED TO SERVER
	 //not connected, send directly to the inbound queue for echoing
		echoSend();
	}
}

/*
Will gather data from the in-packet queue. This will use network cababilities if the network is connected.
*/
void ee::Game::collectDataFromNetwork()
{
	//Determine if packets should be from network or echoed packets
	if (networkValid()) {	///CONNECTED TO NETWORK
		//lock the mutex and then extract data from the packets
		lock_guard<mutex> lock(*networkMutex);
		unpackInpacketQueue();
	} // *END LOCK*
	else {	///NOT CONNECTED TO A NETWORK
		//echo data for testing without locking a mutex
		unpackInpacketQueue();
	}
}

void ee::Game::clientSend()
{
	//Send messages to server
	clientSendSysMsg();

	//clients only send their player data
	clientSendPlayerData();

}

void ee::Game::serverSend()
{
	//conduct server state logic
	serverState.serverStateLogic();

	//serverSendSysMsg(*outboundPacket);
	*outboundPacket << *this;
}

void ee::Game::echoSend()
{
	//Package Player Data
	if (outboundPacket != nullptr) {
		outboundPacket->clear();
		*outboundPacket << *this;		//The client will reset the packet, this will let the server know if it can send another packet

		inPacketQueue.push_back(outboundPacket);
	}
}

void ee::Game::unpackInpacketQueue()
{
	//check if too many packets (due to lag) //trim out front packets based on threshold
	analyzePacketNumberAndRemoveExcess();

	//check if size if over too big threshold, then dump packets old packets
	for (unsigned int i = 0; i < inPacketQueue.size(); ++i) {
		clientInboundPacket = inPacketQueue.front();
		if (clientInboundPacket != nullptr) {
			*clientInboundPacket >> *this;
		}
	}
	inPacketQueue.clear();
}







sf::Uint8 ee::Game::getId()
{
	return this->gameid;
}


void ee::Game::
serverSendSysMsg()
{
	serverCheckJoiningPlayers();
}

//		*ORDERING*				*METHOD*								*NOTES*
//previous				handlePlayerIdRequest()
//this					serverCheckJoiningPlayers()				// called in serverSend()\serverSendSysMsg()
//next					handleServerAcceptedPlayerIdRequest()	//changes clientState
void ee::Game::serverCheckJoiningPlayers()
{
	if (serverState.playerJoining) { *outboundPacket << static_cast<sf::Uint8>(Flag::PLAYERJOININPROCESS); }
	if (serverState.broadcastingNewPlayerInfo) {
		*outboundPacket << static_cast<sf::Uint8>(Flag::SERVERACCEPTEDREQUESTPLAYERID);
		*outboundPacket << serverState.playerJoiningKey;
		*outboundPacket << serverState.playerJoiningActorId;
	};
}

/*
This method is only called after a flag that the packet contains a cell.
This method draws the cell ID then matches that ID to a cell in the hashmap: cellMap.
If the cell exists, it will update its data; if it doesn't exist, it will create a new cell with the data passed.
*/
void ee::Game::handleCellUnpacking(sf::Packet & packet)
{
	//Get cell ID for map lookup
	string cellId;
	packet >> cellId;

	//lookup from the map using the cell id
	auto cellPtr = cellMap[cellId];

	//Cell exists in Map
	if (cellPtr != nullptr) {
		//read the data from the packet and update!
		cellPtr->unpackMobileActors(packet);
		return;
	}
	//Cell Did not exist in Map
	else {
		//Create new cell with the grid
		cellMap.erase(cellId);	//the lookup will create a new entry if there wasn't one
		makeNewCell(cellId);
		cellPtr->unpackMobileActors(packet);
	}
}

//        *Sequence*			*METHOD*								*NOTES*
//previous				clientRequestPlayerID()					//getIdPhase
//this					handlePlayerIdRequest()
//next					serverCheckJoiningPlayers()				// called in serverSend()\serverSendSysMsg()

void ee::Game::handlePlayerIdRequest(sf::Packet & packet)
{
	sf::Uint8 key = 0;

	//expect a randomized key integer value from packet
	packet >> key;	//must be done everytime to ensure integrity of packet

	//Create a player, then broad cast the actor id and key TODO: fix the player creation
	if (serverState.allowPlayerjoin()) {
		sf::Uint8 actorId = static_cast<sf::Uint8>(makeNewPlayer());	//players will have their ids below #256 so down-casting to 8 bits is fine
		serverState.playerJoining = true;				//broadcast that lets other joining players that server is busy
		serverState.broadcastingNewPlayerInfo = true;	//will set a flag to broadcast key and actorId
		serverState.playerJoiningKey = key;
		serverState.playerJoiningActorId = actorId;
		serverState.playerJoiningSequenceClock.restart();	//resets clock that is used in emergency abort (client has 30 seconds to handshake)
	}


	// -task will broadcast until an accept id msg recieved (or threshold met)
}

/*
This method should be called if a player flag is recieved. Checks actor id and updates player w/ that id
caveats:
Assumes that player's local actor ID matches the server's actor id for that player.
*/
void ee::Game::handlePlayerUnpacking(sf::Packet & packet)
{
	//Get Id for actor lookup
	sf::Uint32 actorId = 0;
	packet >> actorId;
	//since the player amounts are small, simply loop through list of players look for a match on id
	for (unsigned int i = 0; i < playerList.size(); ++i) {
		if (playerList[i]->getId() == actorId) {
			//check if it is the controlled player
			//if (actorId == player->getId()) {
			packet >> *(playerList[i]);		//packet >> actor operator
			//}
			 //else {
			//	static Actor garbageActor;
			//	packet >> garbageActor;	//throw away data the games own player 
			//}
			return;
		}
	}
}


//        *STATE*				*METHOD*								*NOTES*

//previous					serverCheckJoiningPlayers()				// called in serverSend()\serverSendSysMsg()
//this						handleServerAcceptedPlayerIdRequest()	//changes clientState
//next	(before last)		clientRequestPlayerID()					//idAcceptedPhase
///This method is called when a client picks up that the server has recognized the client's request for a player to control
void ee::Game::handleServerAcceptedPlayerIdRequest(sf::Packet & packet)
{
	sf::Uint8 key;
	sf::Uint8 actorId;
	packet >> key;
	packet >> actorId;
	
	//if the client is currently in the getIdPhase, and the key matches, then this is the actor id that the server is giving the player
	if (clientState.getIdPhase && clientState.clientKey == key) {
		//make new player on the client side
		makeNewPlayer(nullptr, &actorId);

		//alert server that the player was accepted
		clientState.idAcceptedPhase;
	}
	
}


//Assumes that the last player added to list is the unclaimed player
//Note, this will check the key to make sure it is the key that the server is broadcasting
//There may be a bug where an old key keeps broadcasting, due to a logical problem in my counters.
//If this is the case, then there is only a 1/256 chance that the old key will match the new key. 
//Once the new key is accepted, the server should stop broadcasting that a player is joining.
//This will allow both the new key, and the broadcasting of the old key to hault once a threshold is met (10 packets of no playerJoining flag).
//
///	previous: clientRequestPlayerID
/// this:This is the last method in the handshaking of a player joining. The client should be doing a clientState.stateCheck() everytime the client
/// next:
///
/// broadcasts its "clientAcceptedPlayerId" msg. Since this method stops playerJoining msg flag, the client will stop broadcasting its msg after 10 packets. 
/// 10 packets was used incase lag/network issues corrupted a single packet to make it appear that playerJoining flag was not met.

void ee::Game::handleClientAcceptingPlayerID(sf::Packet & packet)
{
	sf::Uint8 key;
	packet >> key;

	//check if this is a fresh key - matching what is broadcasting means this is a new key. Also checks that server is in a playerjoining state.
	if (serverState.playerJoining && key == serverState.playerJoiningKey) { 
		serverState.playerJoining = false;	//will allow new players to join
		serverState.broadcastingNewPlayerInfo = false;
		//playerList[playerList.size() - 1]->setPlayer(true);	//may assume last player added is the most recently claimed actor
	}
}

void ee::Game::handlePlayerJoining(sf::Packet & packet)
{
	if (clientState.waitForEntryPhase) { clientState.timesWaitedToStartJoin--; }	//prevents client from attempting to join
	if (clientState.idAcceptedPhase) { clientState.timesToKeepBroadcastingAcceptedMsg++; }	//this int always counting down, so this packets increments to pause countdown
}

/*
Packages player data to send over the network. Player clients will be responsible for sorting this data
(ie not updating their position based where the server thinks the player is [it will be slightly lagged])
*/
void ee::Game::packPlayerList(sf::Packet & packet)
{
	for (unsigned int i = 0; i < playerList.size(); ++i) {
		packet << static_cast<sf::Uint8>(Flag::PLAYER);
		packet << playerList[i]->getId();	//sf::Uint32
		packet << *playerList[i];
	}
}


/*
Analyzes packets in queue, if there is surplus due to lag it will remove all but the last few packets in que.
*/
void ee::Game::analyzePacketNumberAndRemoveExcess()
{
	if (inPacketQueue.size() > 20) {
		for (unsigned int i = 0; i < inPacketQueue.size() - 10; ++i) {
			//pop off the front until 10 remain
			inPacketQueue.pop_front();
		}
	}
}

/* Method checks that all variables for server/client transmissio nare not null*/
bool ee::Game::networkValid()
{
	bool returnFlag = true;
	if (!network) { return false; }
	if (!networkMutex) { return false; }
	if (!networkLaunched) { return false; };

	return returnFlag;
}


void ee::Game::clientSendSysMsg()
{
	//If just connecting, setup an id on the server [playerID is same as ID on server]
	//clientRequestPlayerID();
}



///@DEPRECATED - TCP functionality added, this ping-ponging is no longer necessary
///These methods will be removed in future versions 
///
///This method initiates back and forth messages between the server to get a player ID.
///This method is designed for UDP transmission as TCP is not currently implemented.
///A swtich to TCP may be made, but as of right now this is how joining players will be handled.
///
///The method order is the following
///        *STATE*				*METHOD*								*NOTES*
///wait						clientState.stateCheck()				//will wait while under a certain threshold, 
///request					clientRequestPlayerID()					//getIdPhase
///set pjoin & accept		handlePlayerIdRequest()					//Tells server to broadcast that player is joining, and that the player was accepted (ie giving actor's id)
///use pjoin & accept		serverCheckJoiningPlayers()				//broadcasts above msgs. this is called in serverSend()\serverSendSysMsg()
///clnt see serv accpt		handleServerAcceptedPlayerIdRequest()	//changes clientState
///clnt brdcst accpt recvd	clientRequestPlayerID()					//idAcceptedPhase
///serv stop pjoin msg		handleClientAcceptingPlayerID()			//stops playerJoining broadcast on server
void ee::Game::clientRequestPlayerID()
{
	if (clientState.waitForEntryPhase) { clientState.stateCheck(); return; }
	if (clientState.getIdPhase) {
		//Send a randomized integer as key (only once)
		if (!clientState.keyRandomized) { clientState.randomizeClientKey(); }

		//let server know of request
		*outboundPacket << static_cast<sf::Uint8>(Flag::REQUESTPLAYERID);
		*outboundPacket << clientState.clientKey;
	}
	if (clientState.idAcceptedPhase) {	
		//this state will be true as long as the server keeps broadcasting that the id is accepted 
		//server will stop when it gets packet saying key accepted send flag idAccepted, send integer number
		*outboundPacket << static_cast<sf::Uint8>(Flag::CLIENTACCEPTEDPLAYERID);
		*outboundPacket << clientState.clientKey;
		clientState.stateCheck();	//checks if counter to stop broadcasting this message has been met
	}
}

void ee::Game::clientSendPlayerData()
{
	if (player != nullptr) {
		*outboundPacket << static_cast<sf::Uint8>(Flag::PLAYER);
		*outboundPacket << *player;	//will package an actor
		*outboundPacket << static_cast<sf::Uint8>(Flag::END);
	}
}

shared_ptr<ee::Network> ee::Game::getNetwork()
{
	return network;
}

void ee::Game::forceNetworkShutdown()
{
	//Kill the network thread
	network->terminateThread();

	//sleep until the network thread is shutdown (and it is safely ready to be closed witout crash)
	while (network->getNetworkStatus()) {
		this_thread::sleep_for(chrono::seconds(1));
	}
}




/*
Packets work by sending a flag, then data, flag, then data. This repeats until an ending flag
*/
sf::Packet & ee::operator<< (sf::Packet & packet, ee::Game& game)
{
	//Send Flag Identifying Player data follows
	packet << static_cast<sf::Uint8>(Flag::CELL);
	game.activeDrawCell->packMobileActors(packet);
	game.packPlayerList(packet);
	packet << static_cast<sf::Uint8>(Flag::END);

	return packet;
}

sf::Packet & ee::operator >> (sf::Packet & packet, ee::Game& game)
{
	//Variables
	sf::Uint8 rawFlag = 0;
	Flag convertedFlag = Flag::END;
	bool continueUnpacking = true;
	int missedFlagsRemainingBeforeAbort = 3;

	while (continueUnpacking) {
		if (packet >> rawFlag) {
			//Convert Flag
			convertedFlag = static_cast<Flag>(rawFlag);

			//Check For System-level Messages
			if (convertedFlag == Flag::SYSMSG) {

			}

			if (convertedFlag == Flag::PLAYERJOININPROCESS) {
				game.handlePlayerJoining(packet);
			}

			if (convertedFlag == Flag::REQUESTPLAYERID) {
				game.handlePlayerIdRequest(packet);
			}

			if (convertedFlag == Flag::SERVERACCEPTEDREQUESTPLAYERID) {
				//client will see this flag
				game.handleServerAcceptedPlayerIdRequest(packet);
			}

			if (convertedFlag == Flag::CLIENTACCEPTEDPLAYERID) {
				game.handleClientAcceptingPlayerID(packet);
			}

			if (convertedFlag == Flag::PLAYER) {
				game.handlePlayerUnpacking(packet);
			}

			//Handle Cell
			if (convertedFlag == Flag::CELL) {
				game.handleCellUnpacking(packet);
			}

			//End The unpacking process
			if (convertedFlag == Flag::END) {
				continueUnpacking = false; //break unpacking loop
			}
		}
		else {
			//ERROR - flag wasn't recieved when it should have been!
			cout << "error - expecting packet flag but failed to load flag string, attempts remaining" << --missedFlagsRemainingBeforeAbort << endl;
			if (missedFlagsRemainingBeforeAbort == 0)
				break;
		}
	}
	return packet;
}

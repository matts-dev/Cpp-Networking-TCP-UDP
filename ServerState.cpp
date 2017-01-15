#include "ServerState.hpp"

void ee::ServerState::serverStateLogic()
{
	serverStateLogicIncrements();
	serverStateLogicEmergencyAborts();
}

///1. Decrements the wait counter between players joining
void ee::ServerState::serverStateLogicIncrements()
{
	if (!playerJoining 
		&& !broadcastingNewPlayerInfo
		&& waitBeforeNextPlayerCanJoin >= -10) 
	{
		waitBeforeNextPlayerCanJoin--;
	}
}

/// 1. Player has 30 seconds to handshake before server aborts the process.
void ee::ServerState::serverStateLogicEmergencyAborts()
{
	static sf::Time timeThreshold = sf::seconds(30.0f);
	//Checks for situation where client may have abandoned joining admist the playerJoining logic.
	if (playerJoining) {
		//if client hasn't responded in a certain threshold, the player joining is aborted.
		if (playerJoiningSequenceClock.getElapsedTime() > timeThreshold) {	resetPlayerJoiningState();	}
	}
}

///This method determines whether a server will accept an incoming player.
///This method is designed to delay two players being able to join sequentially in a short amount of time.
///After joining player, the player client requires that "playerJoining" stop being broadcasting for 10 packets - this signals that the server acknowledged the client got the player.
///if another player were to join before this 10 packet threshold, then the system would fail and the first client would keep broadcasting that it accepted the player.
///This method relies on another method (serverStateLogicIncrements()) that waits for 50 packets before allowing another player to join.
bool ee::ServerState::allowPlayerjoin()	//method should be called in the serverSend method
{
	//check if max players reached (shouldn't have to because client will not allow more in if max players reached)

	//check if server has waited enough since last player joined (last player will need time to see to stop its own broadcasting of player accepted)
	if (waitBeforeNextPlayerCanJoin < 0) {return true;}
	else {return false;	}
}

void ee::ServerState::resetPlayerJoiningState()
{
	playerJoining = false;
	broadcastingNewPlayerInfo = false;

	playerJoiningKey = 0;
	playerJoiningActorId = 0;
	waitBeforeNextPlayerCanJoin = 50;
}

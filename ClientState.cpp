#include "ClientState.hpp"

void ee::ClientState::stateCheck()
{
	if (waitForEntryPhase) {waitForOtherPlayersPhase();}
	if (idAcceptedPhase) { broadCastCountdownCheck(); }

}

void ee::ClientState::waitForOtherPlayersPhase() {
	if (timesWaitedToStartJoin < 10) {
		//Client has not waited long enough
		timesWaitedToStartJoin++;
	}
	else {
		waitForEntryPhase = false;
		getIdPhase = true;
	}
}

void ee::ClientState::randomizeClientKey()
{
	srand(static_cast<unsigned int>(time(nullptr)));
	unsigned short int rawValue = static_cast<unsigned short int>(rand());
	clientKey = static_cast<sf::Uint8>(rawValue);
	keyRandomized = true;
}

void ee::ClientState::broadCastCountdownCheck()
{
	if (timesToKeepBroadcastingAcceptedMsg < 0) {
		idAcceptedPhase = false;
	}
	else{
		timesToKeepBroadcastingAcceptedMsg--;
	}
}

void ee::ClientState::resetState()
{
	waitForEntryPhase = true;
	getIdPhase = false;
	idAcceptedPhase = false;
	clientKey = 0;
	keyRandomized = false;
	timesWaitedToStartJoin = 0;
	timesToKeepBroadcastingAcceptedMsg = 10;
}

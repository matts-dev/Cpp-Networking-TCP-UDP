#pragma once
#include<SFML\Network.hpp>
#include<vector>
#include<list>
#include<memory>
#include<SFML/System.hpp>

namespace ee {
	struct ServerState {
		bool playerJoining = false;
		bool broadcastingNewPlayerInfo = false;
		sf::Clock playerJoiningSequenceClock;

		sf::Uint8 playerJoiningKey = 0;
		sf::Uint8 playerJoiningActorId = 0;
		char waitBeforeNextPlayerCanJoin = 50;
		void serverStateLogic();
		void serverStateLogicIncrements();
		void serverStateLogicEmergencyAborts();
		bool allowPlayerjoin();
		void resetPlayerJoiningState();
	};
} //end namespace ee
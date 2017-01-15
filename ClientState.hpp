#pragma once
#include <SFML\System.hpp>
#include <ctime>
#include <random>
namespace ee {
	struct ClientState {
		/// PHASES - keep phases in order
		bool waitForEntryPhase = true;
		bool getIdPhase = false;
		bool idAcceptedPhase = false;

		///LOGIC AND VARIABLES
		//randomizing key
		sf::Uint8 clientKey = 0;
		bool keyRandomized = false;

		char timesWaitedToStartJoin = 0;
		char timesToKeepBroadcastingAcceptedMsg = 10;

		void stateCheck();
		void emergencyAborts();
		void waitForOtherPlayersPhase();
		void randomizeClientKey();
		void broadCastCountdownCheck();
		void resetState();
	};
} //end namespace ee
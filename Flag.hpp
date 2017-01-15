//Provides small byte size flags for network transmission
#pragma once
#include<SFML\System.hpp>
namespace ee {
	//Class uses sf::Uint8 as underlying type because it is safe to send over network; ie it maintains endianness etc..
	enum class Flag : sf::Uint8 {
		GAME, SYSMSG,
		CELL, ACTOR, PLAYER,
		REQUESTPLAYERID, PLAYERJOININPROCESS, SERVERACCEPTEDREQUESTPLAYERID, CLIENTACCEPTEDPLAYERID,
		INVALID, STOP, END
	};
} //end namespace ee;
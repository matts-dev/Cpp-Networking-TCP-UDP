#include "Cell.hpp"


ee::Cell::Cell(int xCoordinate, int yCoordinate) : gridX(xCoordinate), gridY(yCoordinate)
{
}

ee::Cell::~Cell()
{
}

sf::Int32 ee::Cell::getXCoord()
{
	return gridX;
}

sf::Int32 ee::Cell::getYCoord()
{
	return gridY;
}

string ee::Cell::toString()
{
	stringstream ss("");
	ss << this->getXCoord();
	ss << " ";
	ss << this->getYCoord();
	return ss.str();
}

/*
Will return a pointer to the actor with the key'd string.
If it does not exist, then method returns nullptr.
*/
shared_ptr<ee::Actor> ee::Cell::getActor(string & str)
{
	auto actorPtr = actorsInCell[str];
	if (!actorPtr) {
		actorsInCell.erase(str);
		return nullptr;
	}
	return actorPtr;
}

bool ee::Cell::addActor(string & str, shared_ptr<Actor>& ptr)
{
	//check if actor already exists
	auto ptrCheck = getActor(str);
	if (ptrCheck) {
		return validateSamePointer(ptr, ptrCheck);
	}

	//add actor if it does not already exists
	actorsInCell[str] = ptr;
	return true;
}


bool ee::Cell::validateSamePointer(shared_ptr<Actor> first, shared_ptr<Actor> second)
{
	//Value already exists, check if same memory location
	if (first.get() == second.get()) {
		//Actor already exists in container, simple return
		return true;
	}
	else {
		//error - trying to add two different objects under same key
		return false;
	}
}


/*
Removes an actor form the underlying map container.
@param str is the string key for the actor.
*/
bool ee::Cell::removeActor(string & str)
{
	//Verify that actor in cell exists
	auto pointer = getActor(str);
	if (pointer) {
		//actor exists, attempt to erase
		actorsInCell.erase(str);
		return true;
	}
	return false;
}

bool ee::Cell::transferActor(string& str, Cell& newCell)
{
	auto actorPtr = this->getActor(str);
	newCell.addActor(str, actorPtr);
	this->removeActor(str);
	return false;
}

void ee::Cell::drawLayer5(sf::RenderWindow & window)
{
	for (auto pair : actorsInCell) {
		auto shrptr = pair.second;
		auto spriteptr = shrptr->getSpriteWeakPtr().lock();
		window.draw(*spriteptr);
	}
}

void ee::Cell::packMobileActors(sf::Packet & packet)
{
	//Send Cell Id
	packet << this->toString();

	//iterator through all of the actors in the cell's actor list
	for (auto pair : actorsInCell) {
		auto ptr = pair.second;
		sf::Uint8 rawFlag = static_cast<sf::Uint8>(ee::Flag::ACTOR);
		sf::Uint32 actorID = ptr->getId();
		packet << rawFlag;
		packet << actorID;
		packet << *ptr;
	}
	//send a stop unpacking actor flag
	packet << static_cast<sf::Uint8>(Flag::STOP);
}

void ee::Cell::unpackMobileActors(sf::Packet & packet)
{
	sf::Uint8 rawFlag = 0;
	ee::Flag convertedFlag = ee::Flag::INVALID;
	bool keepUnpacking = true;
	//set the flag variable, and keep looping until a stop looping flag occurs
	while (keepUnpacking && packet >> rawFlag) {
		//Convert Flag
		convertedFlag = static_cast<Flag>(rawFlag);

		if(convertedFlag == Flag::STOP){
			keepUnpacking = false; //breaks the unpacking loop
		}

		else if (convertedFlag == Flag::ACTOR){
			unpackSingleActor(packet);
		}
		else {	//unidentified flag, return 
			return;
		}
	}


}


/*
	Unloads a single actor from packet. If actor exists, its data will be updated.
	if actor does not exist, a new one will be created and its data will be updated.
*/
void ee::Cell::unpackSingleActor(sf::Packet & packet)
{
	//Get Id for actor lookup
	sf::Uint32 actorId = 0;
	packet >> actorId;

	//Convert id to string
	stringstream ss;
	ss << actorId;
	string key = ss.str();

	//look up using string key
	auto ptr = getActor(key);

	//actor found - update that instance
	if (ptr) {
		packet >> *ptr;
	}
	//no actor found - create instance of actor with that id
	else {
		removeActor(key);	//remove empty key

		//make new actor
		shared_ptr<Actor> newActor = make_shared<Actor>();
		newActor->makeSprite();

		//load the new actor with stats
		packet >> *newActor;
		newActor->setId(actorId);
		bool sucess = addActor(key, newActor);
		if (!sucess) { cout << "failure adding actor in unpackSingleActor(); actorId: " << actorId << endl; }
	}
}

	sf::Packet & ee::operator<<(sf::Packet & packet, Cell cell)
	{
		packet << static_cast<sf::Uint8>(Flag::CELL);
		cell.packMobileActors(packet);
		//STOP FLAG - packMobileActors provides a stop flag at end
		return packet;
	}

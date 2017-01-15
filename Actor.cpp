#include "Actor.hpp"

//Static Variables
unsigned int ee::Actor::instanceCount = 0;
shared_ptr<sf::Texture> ee::Actor::actorSharedTexture = nullptr;


ee::Actor::Actor()
{
	//Update ID
	actorID = instanceCount;
	instanceCount++;
}

ee::Actor::Actor(int index)
{
	actorID = index;
}

ee::Actor::~Actor()
{
}

void ee::Actor::setPlayer(bool flag)
{
	this->player = flag;
}

bool ee::Actor::isPlayer()
{
	return this->player;
}


void ee::Actor::makeSprite()
{
	actorSprite = make_shared<sf::Sprite>();
	applyTextureToSprite();
}

void ee::Actor::applyTextureToSprite()
{
	//initialize texture if this is the first actor created
	if (actorSharedTexture == nullptr) {
		initializeStaticTexture();
	}
	//make sure the initialization passed
	if (actorSharedTexture != nullptr) {
		actorSprite->setTexture(*actorSharedTexture);
		setOriginToTextureCenter();
	}
}

// Only valid if texture contains a single sprite image, rather than a sheet of sprites.
void ee::Actor::setOriginToTextureCenter() {
	sf::Vector2u size = actorSharedTexture->getSize();	//only works with single sprite on sheet
	actorSprite->setOrigin((float)size.x / 2, (float)size.y / 2);
}

weak_ptr<sf::Sprite> ee::Actor::getSpriteWeakPtr()
{
	return actorSprite;
}


void ee::Actor::move(float x, float y)
{
	actorSprite->move(x, y);
}

void ee::Actor::moveUp()
{
	actorSprite->move(0, -actorSpeed);
}

void ee::Actor::moveDown()
{
	actorSprite->move(0, actorSpeed);
}

void ee::Actor::moveLeft()
{
	actorSprite->move(-actorSpeed, 0);
}

void ee::Actor::moveRight()
{
	actorSprite->move(actorSpeed, 0);
}

void ee::Actor::moveUL()
{
	float angleSpeed = calculateAngularSpeed();
	actorSprite->move(-angleSpeed, -angleSpeed);
}

void ee::Actor::moveUR()
{
	float angleSpeed = calculateAngularSpeed();
	actorSprite->move(angleSpeed, -angleSpeed);
}

void ee::Actor::moveDL()
{
	float angleSpeed = calculateAngularSpeed();
	actorSprite->move(-angleSpeed, angleSpeed);
}

void ee::Actor::moveDR()
{
	float angleSpeed = calculateAngularSpeed();
	actorSprite->move(angleSpeed, angleSpeed);
}

float ee::Actor::calculateAngularSpeed()
{
	float angularSpeed = sqrt((actorSpeed * actorSpeed) / 2);
	return angularSpeed;
}

void ee::Actor::rotateToMouse(sf::RenderWindow& window)
{
	//Get window coordinates of mouse
	sf::Vector2i mousePositionInWindow = sf::Mouse::getPosition(window);

	//Get Game World coordinates of mouse
	sf::Vector2f mouseCoords = window.mapPixelToCoords(mousePositionInWindow);

	//Get Game World coordinates of player
	sf::Vector2f playerCoords = actorSprite->getPosition();

	//Get the relative coordinates
	float relX = mouseCoords.x - playerCoords.x;
	float relY = mouseCoords.y - playerCoords.y;

	//Calculate rotation, and convert from radias to degrees
	float playerRotation = static_cast<float>((atan(relY/relX) * (180/3.141)));

	//Correct for location
	if (relX > 0) {
		actorSprite->setRotation(playerRotation + correctSpriteAngleOffset);
	}
	else if (relX < 0) {
		actorSprite->setRotation(playerRotation - correctSpriteAngleOffset);
	}
}

sf::Uint32 ee::Actor::getId()
{
	return actorID;
}

void ee::Actor::setId(sf::Uint32 newId)
{
	this->actorID = newId;
}

string ee::Actor::idStr()
{
	stringstream ss;
	ss << actorID;
	string key = ss.str();
	return key;
}

void ee::Actor::initializeStaticTexture()
{
	if (actorSharedTexture != nullptr) {
		return;
	}
	actorSharedTexture = make_shared<sf::Texture>();
	if (!(actorSharedTexture->loadFromFile(texturePath + defaultSpriteName))) {
		//could not load texture from file
		actorSharedTexture == nullptr;
		cout << "failure loading texture" << endl;
	}

}

sf::Packet & ee::operator<<(sf::Packet & packet, ee::Actor & actor)
{
	//Prepare data for packaging
	auto ptr = actor.getSpriteWeakPtr().lock();
	auto posBoth = ptr->getPosition();
	float posX = posBoth.x;
	float posY = posBoth.y;
	float rotat = ptr->getRotation();	

	//Pack data
	packet << posX;
	packet << posY;
	packet << rotat;

	return packet; //returns the packet for further loading
}

sf::Packet & ee::operator >> (sf::Packet & packet, ee::Actor & actor)
{
	//unpack information
	float posX = 0.0f;
	float posY = 0.0f;
	float rotat = 0.0f;
	packet >> posX >> posY >> rotat;

	//set variables
	auto ptr = actor.getSpriteWeakPtr().lock();
	ptr->setPosition(posX, posY);
	ptr->setRotation(rotat);
	
	return packet; //returns the packet for further extraction
}

#include"KeyBind.hpp"

void ee::KeyBind::populateKeysForUp()
{
	upKeyBinds.push_back(sf::Keyboard::W);
	upKeyBinds.push_back(sf::Keyboard::Up);
}

void ee::KeyBind::populateKeysForDown()
{
	downKeyBinds.push_back(sf::Keyboard::S);
	downKeyBinds.push_back(sf::Keyboard::Down);
}

void ee::KeyBind::populateKeysForLeft()
{
	leftKeyBinds.push_back(sf::Keyboard::A);
	leftKeyBinds.push_back(sf::Keyboard::Left);
}

void ee::KeyBind::populateKeysForRight()
{
	rightKeyBinds.push_back(sf::Keyboard::D);
	rightKeyBinds.push_back(sf::Keyboard::Right);
}

void ee::KeyBind::populateKeysEscapeMenu()
{
	escapeMenuKeyBinds.push_back(sf::Keyboard::Escape);
}

void ee::KeyBind::populateKeysSystemCommandKeys()
{
	systemCommandKeys.push_back(sf::Keyboard::Key::Tilde);
}

void ee::KeyBind::populateKeysStartServerKeys()
{
	startServerKeys.push_back(sf::Keyboard::Key::S);
}

void ee::KeyBind::populateKeysStartClientKeys()
{
	startClientKeys.push_back(sf::Keyboard::Key::C);
}

ee::KeyBind::KeyBind()
{
	populateKeysForUp();
	populateKeysForDown();
	populateKeysForLeft();
	populateKeysForRight();
	populateKeysEscapeMenu();

	populateKeysSystemCommandKeys();
	populateKeysStartServerKeys();
	populateKeysStartClientKeys();
}

ee::KeyBind::~KeyBind()
{
}

bool ee::KeyBind::upPressed()
{
	//loop through all the up keys and see if one is pressed
	for (unsigned int i = 0; i < upKeyBinds.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(upKeyBinds[i])) {
			return true;
		}
	} 
	return false;
}

bool ee::KeyBind::downPressed()
{
	//loop through all the down keys and see if one is pressed
	for (unsigned int i = 0; i < downKeyBinds.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(downKeyBinds[i])) {
			return true;
		}
	}
	return false;
}

bool ee::KeyBind::rightPressed()
{
	//loop through all the right keys and see if one is pressed
	for (unsigned int i = 0; i < rightKeyBinds.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(rightKeyBinds[i])) {
			return true;
		}
	}
	return false;
}

bool ee::KeyBind::leftPressed()
{
	//loop through all the left keys and see if one is pressed
	for (unsigned int i = 0; i < leftKeyBinds.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(leftKeyBinds[i])) {
			return true;
		}
	}
	return false;
}

bool ee::KeyBind::escapePressed()
{
	//loop through all the escape keys (controllers etc.) and see if one is pressed
	for (unsigned int i = 0; i < escapeMenuKeyBinds.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(escapeMenuKeyBinds[i])) {
			return true;
		}
	}
	return false;
}

bool ee::KeyBind::systemPressed()
{
	//loop through all the escape keys (controllers etc.) and see if one is pressed
	for (unsigned int i = 0; i < systemCommandKeys.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(systemCommandKeys[i])) {
			return true;
		}
	}
	return false;
}

bool ee::KeyBind::startServerPressed()
{
	//loop through all the escape keys (controllers etc.) and see if one is pressed
	for (unsigned int i = 0; i < startServerKeys.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(startServerKeys[i])) {
			return true;
		}
	}
	return false;
}

bool ee::KeyBind::startClientPressed()
{
	//loop through all the escape keys (controllers etc.) and see if one is pressed
	for (unsigned int i = 0; i < startClientKeys.size(); ++i) {
		if (sf::Keyboard::isKeyPressed(startClientKeys[i])) {
			return true;
		}
	}
	return false;
}


//This class provides an interface for keybinding certain game defined actions. 
//This class allows flexible keybind adding for the actions possible in the game

#pragma once
#include<vector>

//#include"SFML\System.hpp"
#include"SFML\Window.hpp"

using namespace std;
namespace ee {
	class KeyBind {
	private:
		vector<sf::Keyboard::Key> upKeyBinds;
		vector<sf::Keyboard::Key> downKeyBinds;
		vector<sf::Keyboard::Key> rightKeyBinds;
		vector<sf::Keyboard::Key> leftKeyBinds;
		vector<sf::Keyboard::Key> escapeMenuKeyBinds;

		vector<sf::Keyboard::Key> systemCommandKeys;
		vector<sf::Keyboard::Key> startServerKeys;
		vector<sf::Keyboard::Key> startClientKeys;

		//Method within constructor for constructor
		void populateKeysForUp();
		void populateKeysForDown();
		void populateKeysForLeft();
		void populateKeysForRight();
		void populateKeysEscapeMenu();

		void populateKeysSystemCommandKeys();
		void populateKeysStartServerKeys();
		void populateKeysStartClientKeys();
	protected:
	public:
		KeyBind();
		~KeyBind();

		bool upPressed();
		bool downPressed();
		bool rightPressed();
		bool leftPressed();
		bool escapePressed();

		bool systemPressed();
		bool startServerPressed();
		bool startClientPressed();
	};
};
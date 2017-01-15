#include <SFML/Graphics.hpp>
#include <thread>
#include <mutex>
#include "Game.hpp"
#include "Flag.hpp"
#include "Network.hpp"

//testing
int NetworkTest_main();

//Global Variables

int main()
{
	//testing
	//NetworkTest_main();
	//return 0;	//prevent loading the game after network test


	//Setup Game Variables
	ee::Game game;
	game.initializeVars();

	sf::RenderWindow window(sf::VideoMode(game.getWindowWidth(), game.getWindowHeight()), "10 Parsecs");
	window.setFramerateLimit(60);
	
	//Function pointers for threads
	void (ee::Network::*networkProcess)() = &ee::Network::waitToRun;
	//thread object

	thread networkThread(networkProcess, game.getNetwork().get());
	//detaching threads from this thread (requirement for threads is to detach or join before main completion).
	networkThread.detach();


	while (window.isOpen())
	{
		//Event handling
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		//handle logic
		game.gameLogic(window);


		//Draw Commands
		game.gameDraw(window);
	}

	game.forceNetworkShutdown();	
	return 0;
}
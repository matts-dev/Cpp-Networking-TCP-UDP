/*
Thie class provides a container cell for game related objects.
The game world is divided up into cells. 
This divides up the processing necessary to run the game; The game will run only process the active cells.
Cells will contain evertying from actor objects, to static non-motile objects, to visitble planets. 
Cells should be designed to be large scale and potentially divided up into sub-scales.
*/
#pragma once

#include<iostream>
#include<string>
#include<memory>
#include<sstream>
#include<vector>

#include"SFML\Graphics.hpp"
#include"SFML\Window.hpp"
#include"SFML\System.hpp"
#include"SFML\Audio.hpp"
#include"SFML\Network.hpp"
#include"Actor.hpp"
#include"Flag.hpp"
#include"Object.hpp"

using namespace std;

namespace ee {
	class Cell : public object
	{
	private:
		//Grid Location
		sf::Int32 gridX;
		sf::Int32 gridY;

		//Boundary Values
		sf::Int32 xMax;
		sf::Int32 xMin;
		sf::Int32 yMax;
		sf::Int32 yMin;

		map<string, shared_ptr<Actor>> actorsInCell;
	protected:
	public:
		Cell(int xCoordinate, int yCoordinate);
		~Cell();

		//Cell State Properties
		sf::Int32 getXCoord();
		sf::Int32 getYCoord();
		string toString();

		//Cell contents
		shared_ptr<Actor> getActor(string& str);
		bool addActor(string& str, shared_ptr<Actor>& ptr);
		bool validateSamePointer(shared_ptr<Actor> first, shared_ptr<Actor> second);
		bool removeActor(string& str); 
		bool transferActor(string& str, Cell& newCell);

		//Drawing
		void drawLayer1(sf::RenderWindow& window);		// -bottom layer- (space, planets)
		void drawLayer2(sf::RenderWindow& window);
		void drawLayer3(sf::RenderWindow& window);		// ship/building interiors
		void drawLayer4(sf::RenderWindow& window);		// ship/building exteriors
		void drawLayer5(sf::RenderWindow& window);		// -top layer- (players, trees, etc)
		
		//Transfering Cell Contents
		friend sf::Packet& operator<< (sf::Packet& packet, Cell cell);
		void packMobileActors(sf::Packet& packet);
		void unpackMobileActors(sf::Packet& packet);
		void unpackSingleActor(sf::Packet& packet);

	};
} // end namespace ee

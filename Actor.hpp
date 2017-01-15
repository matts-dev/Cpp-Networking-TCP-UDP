#pragma once
#include<SFML/Window.hpp>
#include<SFML/Audio.hpp>
#include<SFML/Graphics.hpp>
#include<SFML/System.hpp>
#include<SFML/Network.hpp>

#include<iostream>
#include<string>
#include<sstream>
#include<vector>
#include<memory>
#include<cmath>

#include "Object.hpp"

using namespace std;

namespace ee {
	class Actor : public object {
	protected:
		static unsigned int instanceCount;
		sf::Uint32 actorID = 100;
		shared_ptr<object> ownerCell = nullptr;
		bool player = false;


		//Sprite and Texture Related
		shared_ptr<sf::Sprite> actorSprite = nullptr;
		static shared_ptr<sf::Texture> actorSharedTexture;
		string texturePath = "Resources\\Images\\";
		string defaultSpriteName = "GenericActorSprite.png";

		//Action Related
		float actorSpeed = 10.0f;

		//Rotation
		float correctSpriteAngleOffset = 90.0f;
		sf::Vector2i mousePositionInWindow = { 0, 0 };
		sf::Vector2f mouseCoords = { 0, 0 };
		sf::Vector2f playerCoords = { 0, 0 };
		float relX = 0.0f;
		float relY = 0.0f;
		float playerRotation = 0.0f;

	public:
		Actor();
		Actor(int index);
		virtual ~Actor();

		//General Actor Functions
		void setPlayer(bool flag);
		bool isPlayer();

		//Actor Sprite and Texture Creation Functions
		virtual void makeSprite();
		virtual void applyTextureToSprite();
		virtual void initializeStaticTexture();
		virtual void setOriginToTextureCenter();

		//Access
		virtual weak_ptr<sf::Sprite> getSpriteWeakPtr();

		//Sprite Manipulation
		void move(float x, float y);
		void moveUp();
		void moveDown();
		void moveLeft();
		void moveRight();
		void moveUL();
		void moveUR();
		void moveDL();
		void moveDR();
		float calculateAngularSpeed();
		void rotateToMouse(sf::RenderWindow& window);

		//Networking Overloads
		friend sf::Packet& operator<< (sf::Packet& packet, ee::Actor& actor);
		friend sf::Packet& operator>> (sf::Packet& packet, ee::Actor& actor);
		sf::Uint32 getId();
		void setId(sf::Uint32 newId);
		string idStr();

		

	};
}; //end namespace ee
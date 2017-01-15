#pragma once
#include"Object.hpp"
#include"Actor.hpp"
#include"Cell.hpp"

//using std::shared_ptr;

namespace ee {
	class Player : public Actor {
	protected:
		shared_ptr<Cell> containingCell = nullptr;
	public:
		Player(shared_ptr<Cell>& parentCell);
		virtual ~Player();
		void changeContainerCell(shared_ptr<Cell> containerCell);
	};
};
#include "Player.hpp"

ee::Player::Player(shared_ptr<Cell>& parentCell)
{
	containingCell = parentCell;
}

ee::Player::~Player()
{
}

void ee::Player::changeContainerCell(shared_ptr<Cell> containerCell)
{
	this->containingCell = containerCell;
}


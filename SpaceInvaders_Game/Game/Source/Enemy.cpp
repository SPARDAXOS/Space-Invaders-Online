#include "Enemy.hpp"

Enemy::Enemy() {
	KillPoints = 0;
	OriginalPosition = sf::Vector2f(0.0f, 0.0f);
	CurrentSwayDirection = SwayDirection::RIGHT;

	Active = false;

	SetupBody();
}


void Enemy::Deactivate() {
	Active = false;
}


void Enemy::SetupBody() {
	Body.setSize(sf::Vector2f(20.0f, 20.0f)); //Needs to be same as server!
	Body.setFillColor(sf::Color::Yellow); 
}
void Enemy::RegisterMovementInput(float deltaTimeAsSeconds) {
	sf::Vector2f Direction;
	if(CurrentSwayDirection == SwayDirection::LEFT)
		Direction = sf::Vector2f(-1.0f, 0.0f);
	else if (CurrentSwayDirection == SwayDirection::RIGHT)
		Direction = sf::Vector2f(1.0f, 0.0f);

	sf::Vector2f Velocity = Direction * Speed * deltaTimeAsSeconds;
	sf::Vector2f CurrentPosition = Body.getPosition();
	sf::Vector2f NewPosition = CurrentPosition + Velocity;

	Body.setPosition(NewPosition);

	sf::Vector2f RightBoundryPosition = sf::Vector2f(OriginalPosition.x + SwayAmount, OriginalPosition.y);
	sf::Vector2f LeftBoundryPosition = sf::Vector2f(OriginalPosition.x - SwayAmount, OriginalPosition.y);

	if (NewPosition.x >= RightBoundryPosition.x) { //Right
		Body.setPosition(RightBoundryPosition);
		CurrentSwayDirection = SwayDirection::LEFT;
	}
	if (NewPosition.x <= LeftBoundryPosition.x) { //left
		Body.setPosition(LeftBoundryPosition);
		CurrentSwayDirection = SwayDirection::RIGHT;
	}
}


void Enemy::Render(sf::RenderWindow& window) {
	window.draw(Body);
}
void Enemy::Update(sf::Time deltaTime) {
	RegisterMovementInput(deltaTime.asSeconds());
}
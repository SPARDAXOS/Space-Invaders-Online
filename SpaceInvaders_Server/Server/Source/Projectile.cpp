#include "Projectile.hpp"


Projectile::Projectile() {
	// x = left		y = right
	HorizontalWorldBoundries = sf::Vector2f(0.0f, 0.0f);
	// x = up		y = down
	VerticalWorldBoundries = sf::Vector2f(0.0f, 0.0f);

	OwnerIndex = 0; //Default

	SetupBody();

	Active = false;
}


void Projectile::Shoot(sf::Vector2f position, uint32 ownerIndex) {
	Active = true;
	Body.setPosition(position);
	SetOwnerIndex(ownerIndex);
}
void Projectile::Deactivate() {
	Active = false;
	OwnerIndex = 0;
}


void Projectile::SetupBody() {
	Body.setSize(sf::Vector2f(10.0f, 10.0f));
}


void Projectile::CheckForBoundries() {
	sf::Vector2f CurrentPosition = Body.getPosition();

	bool Left = CurrentPosition.x <= HorizontalWorldBoundries.x;
	bool Right = CurrentPosition.x >= HorizontalWorldBoundries.y;
	bool Up = CurrentPosition.y <= VerticalWorldBoundries.x;
	bool Down = CurrentPosition.y >= VerticalWorldBoundries.y;

	if (Left || Right || Up || Down)
		Deactivate();
}
void Projectile::UpdateMovement(float deltaTimeAsSeconds) {
	sf::Vector2f Velocity = Trajectory * Speed * deltaTimeAsSeconds;
	sf::Vector2f CurrentPosition = Body.getPosition();
	sf::Vector2f NewPosition = CurrentPosition + Velocity;

	Body.setPosition(NewPosition);
	CheckForBoundries();
}


void Projectile::Update(sf::Time deltaTime) {
	UpdateMovement(deltaTime.asSeconds());
}
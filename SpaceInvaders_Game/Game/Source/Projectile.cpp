#include "Projectile.hpp"


Projectile::Projectile() {
	// x = left		y = right
	HorizontalWorldBoundries = sf::Vector2f(0.0f, 0.0f);
	// x = up		y = down
	VerticalWorldBoundries = sf::Vector2f(0.0f, 0.0f);

	OwnerIndex = 0; //Default

	if (!SetupBody()) {
		printf("Failed to setup projectile body\n");
	}

	Active = false;
}


void Projectile::Shoot(sf::Vector2f position, sf::Color color, uint32 ownerIndex) {
	Active = true;
	Body.setPosition(position);
	Body.setFillColor(color);
	OwnerIndex = ownerIndex;
}
void Projectile::Deactivate() {
	Active = false;
	OwnerIndex = 0;
}


bool Projectile::SetupBody() {
	Body.setSize(sf::Vector2f(10.0f, 10.0f)); //Needs to be same as server!
	Body.setFillColor(sf::Color::Green);
	Body.setOutlineColor(sf::Color::Cyan);
	Body.setOutlineThickness(0.5f);

	return true;
}
void Projectile::RegisterMovementInput(float deltaTimeAsSeconds) {
	sf::Vector2f Velocity = Trajectory * Speed * deltaTimeAsSeconds;
	sf::Vector2f CurrentPosition = Body.getPosition();
	sf::Vector2f NewPosition = CurrentPosition + Velocity;

	Body.setPosition(NewPosition);

	CheckForBoundries();
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


void Projectile::Update(sf::Time deltaTime) {
	RegisterMovementInput(deltaTime.asSeconds());
}
void Projectile::Render(sf::RenderWindow& window) {
	window.draw(Body);
}
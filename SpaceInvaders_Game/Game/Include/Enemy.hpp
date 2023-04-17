#pragma once
#include "Network.hpp"
#include "SFML/Graphics.hpp"

class Enemy {
public:
	Enemy();

	enum class SwayDirection {
		LEFT,
		RIGHT
	};

public:
	void Render(sf::RenderWindow& window);
	void Update(sf::Time deltaTime);

public:
	inline void   SetKillPoints(uint32 points) { KillPoints = points; };
	inline void   SetStatus(bool status) { Active = status; };
	inline void   SetStartingSwayDirection(SwayDirection direction) { CurrentSwayDirection = direction; };
	inline void   SetPosition(sf::Vector2f position) { Body.setPosition(position); };

public:
	inline uint32 GetKillPoints() const { return KillPoints; };
	inline bool   GetStatus() const { return Active; };
	inline sf::FloatRect GetGlobalBounds() const { return Body.getGlobalBounds(); };
	inline sf::Vector2f GetPosition() const { return Body.getPosition(); };

public:
	void Deactivate();

private:
	void SetupBody();
	void RegisterMovementInput(float deltaTimeAsSeconds);

private:
	sf::RectangleShape Body;
	sf::Vector2f       OriginalPosition;
	uint32		       KillPoints;

	SwayDirection CurrentSwayDirection;

	bool Active;

private:
	float         SwayAmount{ 100.0f };
	float         Speed{ 120.0f };
};
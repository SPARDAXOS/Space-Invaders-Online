#pragma once
#include "SFML/Graphics.hpp"
#include "Network.hpp"


class Projectile {
public:
	Projectile();

	inline bool GetStatus() const { return Active; };
	inline uint32 GetOwnerIndex() const { return OwnerIndex; };
	inline sf::FloatRect GetGlobalBounds() const { return Body.getGlobalBounds(); };

	inline void SetStatus(bool status) { Active = status; };
	inline void SetTrajectory(sf::Vector2f trajectory) { Trajectory = trajectory; };
	inline void SetSpeed(float speed) { Speed = speed; };
	inline void SetHorizontalWorldBoundries(sf::Vector2f boundries) { HorizontalWorldBoundries = boundries; };
	inline void SetVerticalWorldBoundries(sf::Vector2f boundries) { VerticalWorldBoundries = boundries; };
	inline void SetOwnerIndex(uint32& index) { OwnerIndex = index; };
	inline void SetPosition(sf::Vector2f position) { Body.setPosition(position); };

public:
	void Update(sf::Time deltaTime);

public:
	void Shoot(sf::Vector2f position, uint32 ownerIndex);
	void Deactivate();

private:
	void SetupBody();

private:
	void UpdateMovement(float deltaTimeAsSeconds);
	void CheckForBoundries();

private:
	sf::RectangleShape Body;
	uint32 OwnerIndex;
	sf::Vector2f HorizontalWorldBoundries;
	sf::Vector2f VerticalWorldBoundries;

	bool Active;

private:
	sf::Vector2f Trajectory{ 0.0f, -1.0f }; //Up;
	float Speed{ 450.0f };
};
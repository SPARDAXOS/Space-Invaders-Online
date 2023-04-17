#pragma once
#include "SFML/Graphics.hpp"
#include "Projectile.hpp"
#include <vector>
#include "Utility.hpp"

class ProjectilesManager final {
public:
	ProjectilesManager();

	void Update(sf::Time deltaTime);
	void Render(sf::RenderWindow& window);

public:
	bool SpawnProjectile(sf::Vector2f position, sf::Color color, uint32 ownerIndex);
	inline void GetProjectilesPool(std::vector<Projectile>*& pointer) { pointer = &Pool; };

private:
	void CreatePool();
	bool GetUnactiveProjectile(Projectile*& pointer);

private:
	std::vector<Projectile> Pool;

private:
	const sf::Vector2f HorizontalWorldBoundries{0.0f, 800.0f}; //x = left		y = right
	const sf::Vector2f VerticalWorldBoundries{ 0.0f, 600.0f }; //x = up			y = down

	const uint32 PoolSize{ 10 };
};
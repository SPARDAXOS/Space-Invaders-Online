#pragma once
#include "Network.hpp"
#include "SFML/Graphics.hpp"
#include "Projectile.hpp"
#include <vector>


class ProjectilesManager final {
public:
	ProjectilesManager();

	void Update(sf::Time deltaTime);

public:
	inline void GetProjectilesPool(std::vector<Projectile>*& pointer) { pointer = &Pool; };

public:
	bool SpawnProjectile(sf::Vector2f position, uint32 ownerIndex);
	void Reset();

private:
	void CreatePool();
	bool GetUnactiveProjectile(Projectile*& pointer);

private:
	std::vector<Projectile> Pool;

private:
	const sf::Vector2f HorizontalWorldBoundries{ 0.0f, 800.0f }; //x = left		y = right
	const sf::Vector2f VerticalWorldBoundries{ 0.0f, 600.0f }; //x = up			y = down

	const uint32 PoolSize{ 25 };
};
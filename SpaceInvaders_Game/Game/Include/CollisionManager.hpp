#pragma once
#include "GameMode.hpp"
#include "ProjectilesManager.hpp"
#include "EnemiesManager.hpp"
#include <vector>

class CollisionManager final {
public:
	CollisionManager() = default;
	
public:
	void Update();

public:
	inline void SetProjectileManager(ProjectilesManager& manager) { ProjectilesManagerRef = &manager; };
	inline void SetEnemiesManager(EnemiesManager& manager) {EnemiesManagerRef = &manager; };

public:
	void SetupPoolsReferences();

private:
	void CheckEnemiesProjectilesCollision();

private:
	ProjectilesManager* ProjectilesManagerRef;
	EnemiesManager* EnemiesManagerRef;

	std::vector<Projectile>* ProjectilesPool;
	std::vector<Enemy>* EnemiesPool;
};
#include "ProjectilesManager.hpp"


ProjectilesManager::ProjectilesManager() {
	CreatePool();
}


bool ProjectilesManager::SpawnProjectile(sf::Vector2f position, sf::Color color, uint32 ownerIndex) {
	Projectile* ProjectilePointer = nullptr;
	if (GetUnactiveProjectile(ProjectilePointer) && ProjectilePointer != nullptr) {
		ProjectilePointer->Shoot(position, color, ownerIndex);
		return true;
	}
	return false;
}


void ProjectilesManager::CreatePool() {
	Pool.reserve(PoolSize);
	for (uint32 i = 0; i < PoolSize; i++) {
		Projectile NewProjectile;
		NewProjectile.SetHorizontalWorldBoundries(HorizontalWorldBoundries);
		NewProjectile.SetVerticalWorldBoundries(VerticalWorldBoundries);
		Pool.emplace_back(NewProjectile);
	}
}
bool ProjectilesManager::GetUnactiveProjectile(Projectile*& pointer) {
	for (auto& x : Pool) {
		if (!x.GetStatus()) {
			pointer = &x;
			return true;
		}
	}
	return false;
}


void ProjectilesManager::Update(sf::Time deltaTime) {
	for (auto& x : Pool) {
		if (x.GetStatus())
			x.Update(deltaTime);
	}
}
void ProjectilesManager::Render(sf::RenderWindow& window) {
	for (auto& x : Pool) {
		if (x.GetStatus())
			x.Render(window);
	}
}
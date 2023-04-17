#include "CollisionManager.hpp"

void CollisionManager::SetupPoolsReferences() {
	if (ProjectilesManagerRef == nullptr) {
		printf("Error Projectiles manager reference is not setup correctly at collision manager\n");
		return;
	}
	if (EnemiesManagerRef == nullptr) {
		printf("Error Enemies manager reference is not setup correctly at collision manager\n");
		return;
	}

	ProjectilesManagerRef->GetProjectilesPool(ProjectilesPool);
	EnemiesManagerRef->GetEnemiesPool(EnemiesPool);
}
void CollisionManager::CheckEnemiesProjectilesCollision() {
	if (ProjectilesPool == nullptr || EnemiesPool == nullptr) {
		printf("Error pools are not setup correctly at collision manager\n");
		return;
	}


	sf::FloatRect ProjectileGlobalBounds;
	sf::FloatRect EnemyGlobalBounds;

	for (auto& projectile : *ProjectilesPool) {
		if (projectile.GetStatus()) {
			ProjectileGlobalBounds = projectile.GetGlobalBounds();

			for (auto& enemy : *EnemiesPool) {
				if (enemy.GetStatus()) {
					EnemyGlobalBounds = enemy.GetGlobalBounds();
					if (ProjectileGlobalBounds.intersects(EnemyGlobalBounds)) {
						projectile.Deactivate();
						//Rest is decided by server!
					}
				}
			}
		}
	}
}
void CollisionManager::Update() {
	CheckEnemiesProjectilesCollision();
}
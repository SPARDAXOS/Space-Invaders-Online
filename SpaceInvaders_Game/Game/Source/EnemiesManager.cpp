#include "EnemiesManager.hpp"


EnemiesManager::EnemiesManager() {
	ActiveEnemiesAmount = 0;
	CurrentActiveEnemiesBits = 0;
	InterpolatorReference = nullptr;
	InterpoaltorTimer = 0.0f;

	CreatePool();
}


void EnemiesManager::ApplyEnemiesAmountBits(uint32 amount) {
	//Dangerous if i got a higher amount of enemies.

	uint32 Counter = 0;
	uint32 BitMask = 1;

	for (uint32 i = 0; i < 25; i++) {
		if ((BitMask & amount) > 0) {
			Pool[i].SetStatus(true);
			Counter++;
		}
		else
			Pool[i].SetStatus(false);
		BitMask <<= 1;
	}

	ActiveEnemiesAmount = Counter;
	CurrentActiveEnemiesBits = amount;
}
void EnemiesManager::ApplyPositions(uint32* xPositions, uint32* yPositions) {
	sf::Vector2f Position;
	uint32 BitMask = 1;

	for (uint32 i = 0; i < 25; i++) {
		if ((BitMask & CurrentActiveEnemiesBits) > 0) {
			Position.x = (float)xPositions[i];
			Position.y = (float)yPositions[i];
			TargetPositions[i] = Position;
		}
		BitMask <<= 1;
	}
}
void EnemiesManager::Reset() {
	for (uint32 i = 0; i < Pool.size(); i++) {
		TargetPositions[i] = sf::Vector2f(0.0f, 0.0f);
		Pool[i].SetStatus(false);
	}
	ActiveEnemiesAmount = 0;
}


void EnemiesManager::CreatePool() {
	uint32 Total = Rows * Columns;
	Pool.reserve(Total);

	float XPosition = WaveOriginPoint.x;
	float YPosition = WaveOriginPoint.y;

	Enemy::SwayDirection RowSwayDirection = Enemy::SwayDirection::LEFT;

	for (uint32 i = 0; i < Columns; i++) {
		for (uint32 j = 0; j < Rows; j++) {
			Enemy NewEnemy;
			NewEnemy.SetKillPoints(1);
			NewEnemy.SetPosition(sf::Vector2f(XPosition, YPosition));
			NewEnemy.SetStartingSwayDirection(RowSwayDirection);
			Pool.emplace_back(NewEnemy);
			XPosition += HorizontalPadding;
			//Go right
		}
		YPosition += VerticalPadding;
		XPosition = WaveOriginPoint.x;

		if (RowSwayDirection == Enemy::SwayDirection::LEFT)
			RowSwayDirection = Enemy::SwayDirection::RIGHT;
		else if (RowSwayDirection == Enemy::SwayDirection::RIGHT)
			RowSwayDirection = Enemy::SwayDirection::LEFT;
		//Go down
	}
}
void EnemiesManager::UpdateActiveEnemies(sf::Time deltaTime) {
	if (InterpolatorReference == nullptr) {
		printf("Error Interpolator reference is null at enemies manager\n");
		return;
	}

	InterpoaltorTimer += deltaTime.asSeconds();
	if (InterpoaltorTimer >= (1.0f / InterpolatorTickRate)) {
		InterpoaltorTimer -= (1.0f / InterpolatorTickRate);

		for (uint32 i = 0; i < Pool.size(); i++) {
			if (Pool[i].GetStatus()) {
				sf::Vector2f InterpolatedPosition
					= InterpolatorReference->Interpolate(Pool[i].GetPosition(), TargetPositions[i]);
				Pool[i].SetPosition(InterpolatedPosition);
			}
		}
	}
}


void EnemiesManager::Update(sf::Time deltaTime) {
	if (ActiveEnemiesAmount > 0)
		UpdateActiveEnemies(deltaTime);
}
void EnemiesManager::Render(sf::RenderWindow& window) {
	for (auto& x : Pool) {
		if (x.GetStatus())
			x.Render(window);
	}
}
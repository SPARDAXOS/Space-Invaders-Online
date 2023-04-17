#include "EnemiesManager.hpp"


EnemiesManager::EnemiesManager() {
	CurrentWaveFinished = true;
	CurrentWaveCount = 0;
	CurrentActiveEnemies = 0;
	CurrentActiveEnemiesBits = 0;

	CreatePool();
}


uint32 EnemiesManager::GetActiveEnemiesBits() {
	uint32 Counter = 0;
	uint32 BitMask = 1;

	for (uint32 i = 0; i < 25; i++) {
		if (Pool[i].GetStatus())
			Counter |= BitMask;
		BitMask <<= 1;
	}

	CurrentActiveEnemiesBits = Counter;

	return Counter;
}
bool EnemiesManager::GetActiveEnemiesPositions(uint32*& xPositions, uint32*& yPositions) {
	uint32 Counter = 0;
	sf::Vector2f Position;
	
	//No protection - Pool size and container have to be 25 or less
	uint32 XPositions[25]{ 0 };
	uint32 YPositions[25]{ 0 };
	xPositions = XPositions;
	yPositions = YPositions;

	uint32 BitMask = 1;

	for (uint32 i = 0; i < 25; i++) {
		if ((BitMask & CurrentActiveEnemiesBits) > 0) {
			Position = Pool[i].GetPosition();
			XPositions[i] = (uint32)Position.x;
			YPositions[i] = (uint32)Position.y;
		}
		else{
			Position = sf::Vector2f(0.0f, 0.0f);
			XPositions[i] = (uint32)Position.x;
			YPositions[i] = (uint32)Position.y;
		}
		BitMask <<= 1;
	}

	return true;
}


void EnemiesManager::StartWave() {
	float XPosition = WaveOriginPoint.x;
	float YPosition = WaveOriginPoint.y;
	Enemy::SwayDirection RowSwayDirection = Enemy::SwayDirection::LEFT;

	for (uint32 i = 0; i < 25; i++) {
		Pool[i].SetPosition(sf::Vector2f(XPosition, YPosition));
		Pool[i].SetStartingSwayDirection(RowSwayDirection);
		if (!Pool[i].GetStatus())
			Pool[i].SetStatus(true);

		XPosition += HorizontalPadding;
		if (((i + 1) % 5) == 0) {
			YPosition += VerticalPadding;
			XPosition = WaveOriginPoint.x;

			if (RowSwayDirection == Enemy::SwayDirection::LEFT)
				RowSwayDirection = Enemy::SwayDirection::RIGHT;
			else if (RowSwayDirection == Enemy::SwayDirection::RIGHT)
				RowSwayDirection = Enemy::SwayDirection::LEFT;
		}
	}

	CurrentWaveFinished = false;
	CurrentWaveCount++;
	CurrentActiveEnemies = 25;
}
void EnemiesManager::EndWave() {
	for (auto& x : Pool) {
		if (x.GetStatus())
			x.SetStatus(false);
	}

	CurrentWaveFinished = true;
	CurrentWaveCount = 0;
	CurrentActiveEnemies = 0;
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
	uint32 Counter = 0;
	for (auto& x : Pool) {
		if (x.GetStatus()) {
			x.Update(deltaTime);
			Counter++;
		}
	}
	CurrentActiveEnemies = Counter;
}
void EnemiesManager::UpdateWaveState() {
	for (auto& x : Pool) {
		if (x.GetStatus()) {
			CurrentWaveFinished = false;
			return;
		}
	}
	CurrentWaveFinished = true;
}


void EnemiesManager::Update(sf::Time deltaTime) {
	UpdateActiveEnemies(deltaTime);
	UpdateWaveState();
}
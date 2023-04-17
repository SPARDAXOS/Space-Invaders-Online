#pragma once
#include "Network.hpp"
#include "SFML/Graphics.hpp"
#include "Enemy.hpp"
#include <vector>


class EnemiesManager final {
public:
	EnemiesManager();

public:
	void Update(sf::Time deltaTime);

public:
	inline void   GetEnemiesPool(std::vector<Enemy>*& pointer) { pointer = &Pool; };
	inline uint32 GetCurrentWaveCount() const { return CurrentWaveCount; };
	inline bool   IsCurrentWaveFinished() const { return CurrentWaveFinished; };

public:
	uint32 GetActiveEnemiesBits();
	inline uint32 GetActiveEnemiesAmount() const { return CurrentActiveEnemies; };
	bool		  GetActiveEnemiesPositions(uint32*& xPositions, uint32*& yPositions);

public:
	void StartWave();
	void EndWave();

private:
	void CreatePool();
	void UpdateActiveEnemies(sf::Time deltaTime);
	void UpdateWaveState();

private:
	std::vector<Enemy> Pool;

	bool   CurrentWaveFinished;
	uint32 CurrentWaveCount;
	uint32 CurrentActiveEnemies;
	uint32 CurrentActiveEnemiesBits;

private:
	const sf::Vector2f WaveOriginPoint{ 115.0f, 80.0f };
	// 5*5=25 enemies
	const uint8 Rows{ 5 };
	const uint8 Columns{ 5 };

	const float HorizontalPadding{ 140.0f };
	const float VerticalPadding{ 60.0f };
};
#pragma once
#include "SFML/Graphics.hpp"
#include "Enemy.hpp"
#include <vector>
#include "Interpolator.hpp"
#include "Network.hpp"


class EnemiesManager final {
public:
	EnemiesManager();

public:
	void Update(sf::Time deltaTime);
	void Render(sf::RenderWindow& window);

public:
	void ApplyEnemiesAmountBits(uint32 amount);
	void ApplyPositions(uint32* xPositions, uint32* yPositions);
	void Reset();


public:
	inline void SetInterpolatorReference(Interpolator& ref) { InterpolatorReference = &ref; };
	inline void GetEnemiesPool(std::vector<Enemy>*& pointer) { pointer = &Pool; };

private:
	void CreatePool();
	void UpdateActiveEnemies(sf::Time deltaTime);


private:
	std::vector<Enemy> Pool;
	uint32 ActiveEnemiesAmount;
	uint32 CurrentActiveEnemiesBits;
	sf::Vector2f TargetPositions[25];
	Interpolator* InterpolatorReference;
	float InterpoaltorTimer;

private:
	const sf::Vector2f WaveOriginPoint{ 115.0f, 80.0f };

	const uint8 Rows{ 5 };
	const uint8 Columns{ 5 };

	const float HorizontalPadding{ 140.0f };
	const float VerticalPadding{ 60.0f };

	const float InterpolatorTickRate{ 60.0f };
};
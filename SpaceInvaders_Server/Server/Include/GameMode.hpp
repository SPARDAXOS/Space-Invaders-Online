#pragma once
#include "Network.hpp"
#include "SFML/Graphics.hpp"
#include "EnemiesManager.hpp"
#include "ProjectilesManager.hpp"


class GameMode final {
public:
	GameMode();

	enum class GameResults {
		NONE,
		WIN,
		LOSE,
		DRAW,
	};

public:
	void Update();

public:
	inline void SetPlayer1OwnerIndex(uint32 ownerIndex) { Player1OwnerIndex = ownerIndex; };
	inline void SetPlayer2OwnerIndex(uint32 ownerIndex) { Player2OwnerIndex = ownerIndex; };
	inline void SetEnemiesManager(EnemiesManager& manager) { EnemiesManagerRef = &manager; };
	inline void SetProjectilesManager(ProjectilesManager& manager) { ProjectilesManagerRef = &manager; };

public:
	bool GetPlayerScore(uint32 ownerIndex, uint32*& scorePointer);
	GameResults GetGameResults(uint32 ownerIndex);

public:
	inline bool IsGameRunning() const { return GameRunning; };

public:
	void AddScore(uint32 ownerIndex, uint32 amount);

public:
	void StartGame();
	void StopGame();
	void FinishGame();

private:
	void UpdateGameState();

private:
	uint32 Player1OwnerIndex;
	uint32 Player2OwnerIndex;

	uint32 Player1Score;
	uint32 Player2Score;

private:
	EnemiesManager* EnemiesManagerRef;
	ProjectilesManager* ProjectilesManagerRef;

private:
	bool GameRunning;

private:
	uint32 EndWave{ 3 };
};

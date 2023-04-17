#include "GameMode.hpp"


GameMode::GameMode() {
	Player1OwnerIndex = 0;
	Player2OwnerIndex = 0;

	Player1Score = 0;
	Player2Score = 0;

	EnemiesManagerRef = nullptr;
	ProjectilesManagerRef = nullptr;

	GameRunning = false;
}


bool GameMode::GetPlayerScore(uint32 ownerIndex, uint32*& scorePointer) {
	if (ownerIndex == Player1OwnerIndex) {
		scorePointer = &Player1Score;
		return true;
	}
	else if (ownerIndex == Player2OwnerIndex) {
		scorePointer = &Player2Score;
		return true;
	}
	return false;
}
GameMode::GameResults GameMode::GetGameResults(uint32 ownerIndex) {
	if (ownerIndex == Player1OwnerIndex) {
		if (Player1Score > Player2Score)
			return GameResults::WIN;
		else if (Player1Score < Player2Score)
			return GameResults::LOSE;
		else
			return GameResults::DRAW;
	}
	else if (ownerIndex == Player2OwnerIndex) {
		if (Player1Score > Player2Score)
			return GameResults::LOSE;
		else if (Player1Score < Player2Score)
			return GameResults::WIN;
		else
			return GameResults::DRAW;
	}

	return 	GameResults::DRAW;
}


void GameMode::AddScore(uint32 ownerIndex, uint32 amount) {
	if (ownerIndex == Player1OwnerIndex)
		Player1Score += amount;
	else if (ownerIndex == Player2OwnerIndex)
		Player2Score += amount;
}


void GameMode::StartGame() {
	if (EnemiesManagerRef == nullptr) {
		printf("Error EnemiesManager reference is not set at gamemode\n");
		return;
	}
	if (ProjectilesManagerRef == nullptr) {
		printf("Error ProjectilesManager reference is not set at gamemode\n");
		return;
	}

	Player1Score = 0;
	Player2Score = 0;

	GameRunning = true;

	EnemiesManagerRef->StartWave();
}
void GameMode::StopGame() {
	if (EnemiesManagerRef == nullptr) {
		printf("Error EnemiesManager reference is not set at gamemode\n");
		return;
	}
	if (ProjectilesManagerRef == nullptr) {
		printf("Error ProjectilesManager reference is not set at gamemode\n");
		return;
	}

	Player1Score = 0;
	Player2Score = 0;
;
	GameRunning = false;
	EnemiesManagerRef->EndWave();
	ProjectilesManagerRef->Reset();
}
void GameMode::FinishGame() {
	GameRunning = false;
	EnemiesManagerRef->EndWave();
	ProjectilesManagerRef->Reset();
}


void GameMode::UpdateGameState() {
	if (EnemiesManagerRef == nullptr) {
		printf("Error EnemiesManager reference is not set at gamemode\n");
		return;
	}
	if (ProjectilesManagerRef == nullptr) {
		printf("Error ProjectilesManager reference is not set at gamemode\n");
		return;
	}

	if (EnemiesManagerRef->IsCurrentWaveFinished()) {
		if (EnemiesManagerRef->GetCurrentWaveCount() == EndWave)
			FinishGame();
		else
			EnemiesManagerRef->StartWave();
	}
}


void GameMode::Update() {
	if (GameRunning)
		UpdateGameState();
}
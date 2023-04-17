#include "GameMode.hpp"


GameMode::GameMode() {
	CurrentConnectionStatus = nullptr;

	Player1Score = 0;
	Player2Score = 0;

	Player1GameResults = GameResults::NONE;

	Player1ReadyCheck = false;
	Player2ReadyCheck = false;

	MainFont = nullptr;

	GameRunning = false;
	ShowResults = false;
	ShowError = false;
}


void GameMode::SetPlayer1Score(uint32 amount) {
	Player1Score = amount;
	if(Player1Score > 0)
	  UpdatePlayer1ScoreText();
}
void GameMode::SetPlayer2Score(uint32 amount) {
	Player2Score = amount;
	UpdatePlayer2ScoreText();
}
void GameMode::SetPlayer1ReadyCheck(bool confirmation) {
	Player1ReadyCheck = confirmation;
	UpdateReadyCheckTexts();
}
void GameMode::SetPlayer2ReadyCheck(bool confirmation) {
	Player2ReadyCheck = confirmation;
	UpdateReadyCheckTexts();
}
void GameMode::SetupTexts() {
	//Player1
	Player1Text.setFont(*MainFont);
	Player1Text.setCharacterSize(25);
	Player1Text.setPosition(5.0f, 545.0f);
	Player1Text.setString(sf::String("Player1"));

	//Player2
	Player2Text.setFont(*MainFont);
	Player2Text.setCharacterSize(25);
	Player2Text.setPosition(695.0f, 545.0f);
	Player2Text.setString(sf::String("Player2"));

	//Player1Score
	Player1ScoreText.setFont(*MainFont);
	Player1ScoreText.setCharacterSize(25);
	Player1ScoreText.setPosition(45.0f, 570.0f);
	Player1ScoreText.setString(sf::String("0"));

	//Player2Score
	Player2ScoreText.setFont(*MainFont);
	Player2ScoreText.setCharacterSize(25);
	Player2ScoreText.setPosition(735.0f, 570.0f);
	Player2ScoreText.setString(sf::String("0"));

	//Results
	ResultsText.setFont(*MainFont);
	ResultsText.setCharacterSize(50);
	ResultsText.setPosition(310.0f, 250.0f);
	ResultsText.setString(sf::String("Results"));

	//Confirmations
	ReadyCheckText.setFont(*MainFont);
	ReadyCheckText.setCharacterSize(35);
	ReadyCheckText.setPosition(310.0f, 150.0f);
	ReadyCheckText.setString(sf::String("Ready: ?/2"));

	//Error 
	ErrorText.setFont(*MainFont);
	ErrorText.setCharacterSize(40);
	ErrorText.setPosition(110.0f, 250.0f);
	ErrorText.setString(sf::String("Session was interrupted"));
}


void GameMode::StartGame() {
	Player1Score = 0;
	Player2Score = 0;

	UpdatePlayer1ScoreText();
	UpdatePlayer2ScoreText();

	GameRunning = true;
	ShowResults = false;
	ShowError = false;
}
void GameMode::StopGame() {
	Player1Score = 0;
	Player2Score = 0;

	UpdatePlayer1ScoreText();
	UpdatePlayer2ScoreText();

	GameRunning = false;
	ShowResults = false;
	ShowError = true;
}
void GameMode::FinishGame(GameResults results) {
	switch (results) {
	case GameResults::NONE: {
		ResultsText.setString("Error");
	}break;
	case GameResults::WIN: {
		ResultsText.setString("You Win!");
	}break;
	case GameResults::LOSE: {
		ResultsText.setString("You Lose!");
	}break;
	case GameResults::DRAW: {
		ResultsText.setString("Draw!");
	}break;
	}

	GameRunning = false;
	ShowResults = true;
	ShowError = false;
}


void GameMode::UpdateReadyCheckTexts() {
	uint32 ReadyPlayers = 0;
	if (Player1ReadyCheck)
		ReadyPlayers++;
	if (Player2ReadyCheck)
		ReadyPlayers++;

	ReadyCheckText.setString(sf::String("Ready: " + std::to_string(ReadyPlayers) + "/2"));
}
void GameMode::UpdatePlayer1ScoreText() {
	Player1ScoreText.setString(std::to_string(Player1Score));
}
void GameMode::UpdatePlayer2ScoreText() {
	Player2ScoreText.setString(std::to_string(Player2Score));
}

void GameMode::Render(sf::RenderWindow& window) {
	if (GameRunning) {
		window.draw(Player1Text);
		window.draw(Player2Text);
		window.draw(Player1ScoreText);
		window.draw(Player2ScoreText);
	}
	if (*CurrentConnectionStatus == ConnectionStatus::CONNECTED && !GameRunning)
		window.draw(ReadyCheckText);
	if (ShowResults)
		window.draw(ResultsText);
	if (ShowError)
		window.draw(ErrorText);
}
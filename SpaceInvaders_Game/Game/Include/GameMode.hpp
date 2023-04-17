#pragma once
#include "Utility.hpp"
#include "SFML/Graphics.hpp"
#include "EnemiesManager.hpp"
#include "Network.hpp"


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
	void Render(sf::RenderWindow& window);

public:
	inline void SetCurrentConnectionStatus(ConnectionStatus& status) { CurrentConnectionStatus = &status; };
	inline void SetFont(sf::Font& font) { MainFont = &font; };
	void SetPlayer1Score(uint32 amount);
	void SetPlayer2Score(uint32 amount);
	void SetPlayer1ReadyCheck(bool confirmation);
	void SetPlayer2ReadyCheck(bool confirmation);
	void SetupTexts();

public:
	void StartGame();
	void StopGame();
	void FinishGame(GameResults results);

public:
	inline bool IsGameRunning() const { return GameRunning; };

private:
	void UpdatePlayer1ScoreText();
	void UpdatePlayer2ScoreText();
	void UpdateReadyCheckTexts();


private:
	ConnectionStatus* CurrentConnectionStatus;

	uint32 Player1Score;
	uint32 Player2Score;

	GameResults Player1GameResults; //You only care about yours

	bool Player1ReadyCheck;
	bool Player2ReadyCheck;

	sf::Font* MainFont;

	sf::Text Player1Text;
	sf::Text Player2Text;
	sf::Text Player1ScoreText;
	sf::Text Player2ScoreText;
	sf::Text ReadyCheckText;
	sf::Text ResultsText;
	sf::Text ErrorText;

	bool GameRunning;
	bool ShowResults;
	bool ShowError;
};
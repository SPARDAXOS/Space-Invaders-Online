#pragma once
#include "SFML/Graphics.hpp"
#include "Utility.hpp"
#include "Network.hpp"


class Inputinator final {
public:
	Inputinator();

public:
	void RegisterInputRequest(MovementInputRequestType input);

	MovementInputRequestType GetLastMovementInputRequest();
	sf::Vector2f GetLastMovementInputPrediction();
	uint32 GetLastInputTick() const { return LastMovementPredictionTick; };

public:
	void PredictMovementInput(sf::Vector2f current, MovementInputRequestType type);
	sf::Vector2f CompareResults(sf::Vector2f current, sf::Vector2f results, uint32 tick);

public:
	inline void SetCurrentTickReference(uint32& ref) { CurrentTickReference = &ref; };
	inline uint32 GetPredictionsAmount() { return PredictionsAmount; };
	inline uint32 GetMispredictionsAmount() { return MispredictionsAmount; };
	void ResetMispredictionsAmount();

private:
	void RegisterMovementInputPrediction(sf::Vector2f prediction, MovementInputRequestType type);

private:
	MovementInputRequestType InputsRequests[100]{ MovementInputRequestType::NONE };
	uint32 InputRequestsIndex;
	MovementInputRequestType LastInputRequest;

private:
	sf::Vector2f MovementPredictions[100];
	uint32 MovementPredictionsTimestamps[100];
	MovementInputRequestType MovementRequestsTypeHistory[100];
	uint32 MovementPredictionIndex;
	sf::Vector2f LastMovementPrediction;
	uint32 LastMovementPredictionTick;

private:
	uint32 PredictionsAmount;
	uint32 MispredictionsAmount;

private:
	uint32* CurrentTickReference;

private:
	float PositiveXWorldBound{ 780.0f };
	float NegativeXWorldBound{ 0.0f };

	uint16 CorrectionMargin{ 5 };
	uint16 MovementSpeed{ 4 }; //Make sure its the same server side for better predictions
};
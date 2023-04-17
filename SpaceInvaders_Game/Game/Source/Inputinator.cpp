#include "Inputinator.hpp"

Inputinator::Inputinator() {
	InputRequestsIndex = 0;
	MovementPredictionIndex = 0;
					  
	PredictionsAmount = 0;
	MispredictionsAmount = 0;

	LastInputRequest = MovementInputRequestType::NONE;
	LastMovementPrediction = sf::Vector2f(0.0f, 0.0f);

	LastMovementPredictionTick = 0;

	CurrentTickReference = nullptr;
}


void Inputinator::RegisterInputRequest(MovementInputRequestType input) {
	InputsRequests[InputRequestsIndex] = input;
	InputRequestsIndex = (InputRequestsIndex + 1) % 100;
	LastInputRequest = input;
}


MovementInputRequestType Inputinator::GetLastMovementInputRequest() {
	return LastInputRequest;
}
sf::Vector2f Inputinator::GetLastMovementInputPrediction() {
	return LastMovementPrediction;
}


void Inputinator::PredictMovementInput(sf::Vector2f current, MovementInputRequestType type) {
	if (type == MovementInputRequestType::NONE) {
		RegisterMovementInputPrediction(current, type);
		return;
	}

	sf::Vector2f Direction;
	sf::Vector2f Velocity;
	sf::Vector2f CurrentPosition = current;
	sf::Vector2f NewPosition;

	if (type == MovementInputRequestType::LEFT)
		Direction = sf::Vector2f(-1.0f, 0.0f);
	else if (type == MovementInputRequestType::RIGHT)
		Direction = sf::Vector2f(1.0f, 0.0f);

	Velocity = Direction * (float)MovementSpeed;
	NewPosition = CurrentPosition + Velocity;

	if (NewPosition.x >= PositiveXWorldBound)
		NewPosition.x = PositiveXWorldBound;
	else if (NewPosition.x <= NegativeXWorldBound)
		NewPosition.x = NegativeXWorldBound;

	PredictionsAmount++;

	RegisterMovementInputPrediction(NewPosition, type);
}
sf::Vector2f Inputinator::CompareResults(sf::Vector2f current, sf::Vector2f results, uint32 tick) {
	uint32 TickIndex = tick % 100;
	assert((MovementPredictionsTimestamps[TickIndex] == tick, "Error in movement predicion timestamps"));

	sf::Vector2f Prediction = MovementPredictions[TickIndex];

	sf::Vector2f Difference = Rose::Abs(Prediction - results);
	float Results = Rose::Length(Difference);

	if (CorrectionMargin > Results)
		return current;

	MispredictionsAmount++;

	MovementPredictions[TickIndex] = results;


	uint32 LastPredictionIndex;
	if (MovementPredictionIndex == 0)
		LastPredictionIndex = 99;
	else
		LastPredictionIndex = MovementPredictionIndex - 1;


	//TickIndex == MovementPredictionIndex will never happen but i put it in just in case
	if (TickIndex == LastPredictionIndex || TickIndex == MovementPredictionIndex)
		return results;


	uint32 CorrectionIndex;
	if (TickIndex == 99)
		CorrectionIndex = 0;
	else
		CorrectionIndex = TickIndex + 1;


	sf::Vector2f BaseForCorrection = MovementPredictions[TickIndex];
	sf::Vector2f FinalPosition = sf::Vector2f(0.0f, 0.0f);


	while (CorrectionIndex != MovementPredictionIndex) {
		MovementInputRequestType Type = MovementRequestsTypeHistory[CorrectionIndex];
		sf::Vector2f Direction;

		if (Type == MovementInputRequestType::LEFT)
			Direction = sf::Vector2f(-1.0f, 0.0f);
		else if (Type == MovementInputRequestType::RIGHT)
			Direction = sf::Vector2f(1.0f, 0.0f);
		else if (Type == MovementInputRequestType::NONE) {
			MovementPredictions[CorrectionIndex] = BaseForCorrection;

			//Just to be explicit
			BaseForCorrection = MovementPredictions[CorrectionIndex];

			FinalPosition = MovementPredictions[CorrectionIndex];

			CorrectionIndex++;
			if (CorrectionIndex == 100)
				CorrectionIndex = 0;

			continue;
		}

		sf::Vector2f Velocity = Direction * (float)MovementSpeed;
		sf::Vector2f CorrectedPosition = BaseForCorrection + Velocity;

		MovementPredictions[CorrectionIndex] = CorrectedPosition;

		FinalPosition = MovementPredictions[CorrectionIndex];

		BaseForCorrection = MovementPredictions[CorrectionIndex];


		CorrectionIndex++;
		if (CorrectionIndex == 100)
			CorrectionIndex = 0;
	}

	return FinalPosition;
}


void Inputinator::ResetMispredictionsAmount() {
	MispredictionsAmount = 0;
}


void Inputinator::RegisterMovementInputPrediction(sf::Vector2f prediction, MovementInputRequestType type) {
	MovementPredictions[MovementPredictionIndex] = prediction;
	MovementPredictionsTimestamps[MovementPredictionIndex] = *CurrentTickReference;
	MovementRequestsTypeHistory[MovementPredictionIndex] = type;

	MovementPredictionIndex = (MovementPredictionIndex + 1) % 100;
	LastMovementPrediction = prediction;
	LastMovementPredictionTick = *CurrentTickReference;
}
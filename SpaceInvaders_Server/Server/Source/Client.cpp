#include "Client.hpp"

Client::Client() {
	uint32 RandomNumber = (uint32)rand();
	OwnerIndex = RandomNumber;

	CurrentConnectionStatus = ConnectionStatus::DISCONNECTED;
	Position = sf::Vector2f{ 0.0f, 0.0f };
	LastReceivedSequenceNumber = 0;
	LastReceivedPacketTimestamp = 0;
	LastProcessedInputTick = 0;
	TimeoutCounter = 0.0f;
	ShootRequest = false;
	ReadyCheckState = false;
	CalculatedRTTsIndex = 0;
	RTTCalculationsAmount = 0;
	AverageRTT = 0;
}


void Client::AddRTTCalculation(uint32 rtt) {
	CalculatedRTTs[CalculatedRTTsIndex] = rtt;
	CalculatedRTTsIndex = (CalculatedRTTsIndex + 1) % 10;
}
void Client::CalculateAverageRTT() {
	uint32 Amount = 0;
	uint32 Accumulation = 0;
	for (auto& x : CalculatedRTTs) {
		Accumulation += x;
		Amount++;
	}
	float Results = float(Accumulation / Amount);
	AverageRTT = (uint32)floor(Results);
}
void Client::ResetRTTData() {
	for (uint32 i = 0; i < 10; i++)
		CalculatedRTTs[i] = 0;
	CalculatedRTTsIndex = 0;
	AverageRTT = 0;
}
bool Client::IsAverageRTTAtThreshold(uint32 threshold) {
	uint32 Counter = 0;
	for (auto& x : CalculatedRTTs) {
		if (x >= threshold)
			Counter++;
	}
	if (Counter >= 10)
		return true;
	else
		return false;
}
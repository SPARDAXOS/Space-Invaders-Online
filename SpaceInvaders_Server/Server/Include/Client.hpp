#pragma once
#include "Network.hpp"
#include "Protocol.hpp"
#include "SFML/Graphics.hpp"
#include <vector>
#include <random>


class Client {
public:
	Client();

public:
	inline void SetStatus(ConnectionStatus status) { CurrentConnectionStatus = status; };
	inline void SetAddress(IP_Address address) { Address = address; };
	inline void SetPosition(sf::Vector2f position) { Position = position; };
	inline void SetLastReceivedSequence(uint16 sequence) { LastReceivedSequenceNumber = sequence; };
	inline void SetLastReceivedPacketTimestamp(uint32 timestamp) { LastReceivedPacketTimestamp = timestamp; };
	inline void SetShootRequest(bool order) { ShootRequest = order; };
	inline void SetReadyCheckState(bool check) { ReadyCheckState = check; };
	inline void SetLastProcessedInputTick(uint32 tick) { LastProcessedInputTick = tick; };

public:
	inline void UpdateTimeoutCounter(float deltaTimeAsSeconds) { TimeoutCounter += deltaTimeAsSeconds; };
	inline void ConfirmConnection() { TimeoutCounter = 0.0f; };
	inline void ResetTimeoutCounter() { TimeoutCounter = 0.0f; };
	inline void QueueInput(MovementInputRequestType input) { QueuedInputs.push_back(input); };
	inline void QueueInputTimestamp(uint32 tick) { QueuedInputsTimestamps.push_back(tick); };
	inline void ClearQueuedInputs() { QueuedInputs.clear(); };
	inline void ClearQueuedInputsTimestamps() { QueuedInputsTimestamps.clear(); };
	void AddRTTCalculation(uint32 rtt);
	void CalculateAverageRTT();
	bool IsAverageRTTAtThreshold(uint32 threshold);
	void ResetRTTData();


public:
	inline ConnectionStatus GetStatus() const { return CurrentConnectionStatus; };
	inline IP_Address GetAddress() const { return Address; };
	inline sf::Vector2f GetPosition() const { return Position; };
	inline uint16 GetLastReceivedSequence() const { return LastReceivedSequenceNumber; };
	inline uint32 GetLastReceivedPacketTimestamp() const { return LastReceivedPacketTimestamp; };
	inline float GetTimeoutCounter() const { return TimeoutCounter; };
	inline bool GetShootRequest() const { return ShootRequest; };
	inline bool GetReadyCheckState() const { return ReadyCheckState; };
	inline uint16 GetMovementSpeed() const { return MovementSpeed; };
	inline uint32 GetLastProcessedInputTick() const { return LastProcessedInputTick; };
	inline std::vector<MovementInputRequestType>& GetQueuedInputs() { return QueuedInputs; };
	inline std::vector<uint32>& GetQueuedInputsTimestamps() { return QueuedInputsTimestamps; };

	inline uint32 GetAverageRTT() const { return AverageRTT; }; 
	inline uint32 GetOwnerIndex() const { return OwnerIndex; };

private:
	ConnectionStatus CurrentConnectionStatus;
	IP_Address		 Address;
	uint32			 OwnerIndex;

private:
	sf::Vector2f	 Position;
	uint16			 LastReceivedSequenceNumber;
	uint32			 LastReceivedPacketTimestamp;
	uint32           LastProcessedInputTick;

private:
	float			 TimeoutCounter;
	bool			 ShootRequest;
	bool			 ReadyCheckState;

private:
	std::vector<MovementInputRequestType> QueuedInputs;
	std::vector<uint32>		              QueuedInputsTimestamps;

private:
	uint32           CalculatedRTTs[10]{ 0 };
	uint32           CalculatedRTTsIndex;
	uint32           RTTCalculationsAmount;
	uint32			 AverageRTT;

private:
	const uint16			 MovementSpeed{ 4 }; //Needs to be a full number
};
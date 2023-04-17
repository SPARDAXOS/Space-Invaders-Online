#pragma once
#include "SFML/Graphics.hpp"
#include "Network.hpp"
#include "Protocol.hpp"
#include "Client.hpp"
#include "Session.hpp"
#include "Utility.hpp"
#include <random>


class Server final {
public:
	Server() = default;

	void Run();

private:
	bool Start();
	void End();
	bool Update();
	void ProcessTick();

private:
	bool Receive();
	bool Send(IP_Address address, ConnectionPacket type);
	bool Send(IP_Address address, DisconnectionPacket type);
	bool Send(IP_Address address, PayloadPacket type);

private:
	void ProcessConnectionPacket(IP_Address& address, ConnectionPacket packet);
	void ProcessDisconnectionPacket(IP_Address& address, DisconnectionPacket packet);
	void ProcessPayloadPacket(IP_Address& address, PayloadPacket packet);

private:
	inline void IncrementPayloadSequenceNumber();
	inline void RegisterSentPayloadPacket(uint16 sequenceNumber);
	bool        WasPayloadPacketSent(uint16 sequenceNumber, uint32& timestamp);


private:
	void   DisconnectClient(IP_Address address, uint32 sessionID, DisconnectionReason reason);
	void   DisconnectClient(IP_Address address, DisconnectionReason reason);
	void   SendDisconnectionPacket(IP_Address address, DisconnectionReason reason);
	void   SendConnectionConfirmationPacket(Session& session, Client& client);

private:
	bool   DoesSessionExist(uint16 ID, Session*& ptr);
	uint32 GetConnectedClientsAmount() const;

private:
	void ConfirmActionRequests(Client& clients);
	void ProcessInputRequest(Client& client);

private:
	void UpdateRunningSessions();
	void CheckSessionsStates();
	void SimulateClients();

private:
	void UpdatePayloadTransmission();
	void CheckForTimeouts();
	void CheckClientsLatency();

private:
	inline bool IsProtocolIDValid(uint32 id) const;
	inline bool IsProtocolVersionValid(uint32 version) const;
	inline bool IsServerFull() const;

private:
	inline void PrintMessage(const char* message) const;
	inline void PrintMessage(const char* message, uint32 num) const;
	inline void PrintMessage(const char* message, IP_Address address) const;
	inline void PrintLastCriticalError() const;
	inline void PrintDisconnectionMessage(IP_Address address, DisconnectionReason reason) const;


private:
	uint32 InputTick;
	float  TimeBetweenEachTick;
	float  TickRate{ 60.0f };
	float  TickTimer;

private:
	float  TimeoutThreshold{ 5.0f };
	uint32 LatencyThreshold{ 1000 };

private:
	float PositiveXWorldBound{ 780.0f };
	float NegativeXWorldBound{ 0.0f };

private:
	uint32 CurrentConnectedClients;
	float  ClientsUpdateRate{ 0.1f };
	float  ClientsUpdateTimer;

private:
	uint16 CurrentPayloadSequenceNumber;
	uint16 LastConnectionPacketSequence;

private:
	uint16 SentPayloadsHistory[100];
	uint32 SentPayloadsTimestampsHistory[100];
	uint32 SentPayloadsIndex;

private:
	sf::Clock        Clock;
	sf::Time         RunningTime;
	sf::Time         DeltaTime;

private:
	Network          MainNetwork;
	UDP_Socket       Socket;
	IP_Address       OpenAddress;

private:
	bool                 Running;
	std::vector<Session> Sessions{ 2 };
	const uint32         SessionsLimit{ 2 };
	bool				 ShowPacketsLog{ false };
};
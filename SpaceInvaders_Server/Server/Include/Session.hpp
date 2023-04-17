#pragma once
#include "Client.hpp"
#include "Network.hpp"
#include "ProjectilesManager.hpp"
#include "EnemiesManager.hpp"
#include "CollisionManager.hpp"
#include "GameMode.hpp"


class Session final {
public:
	Session();

	void UpdateSession(sf::Time deltaTime);

public:
	bool AddClient(IP_Address address);
	bool RemoveClient(IP_Address address);

public:
	bool IsSessionFull() const;
	bool IsSessionEmpty() const;
	bool IsSessionRunning() const;

public:
	bool DoesClientExist(IP_Address address, Client*& client);

public:
	uint16 GetSessionID() const;
	void GetClients(Client*& clients);
	bool GetOtherClient(Client& client, Client*& otherClient);

public:
	uint32 GetActiveEnemiesBits();
	bool GetActiveEnemiesPositions(uint32*& xPositions, uint32*& yPositions);
	bool GetPlayerScore(Client& client, uint32*& scorePointer);
	GameMode::GameResults GetGameResults(uint32 ownerIndex) { return MainGameMode.GetGameResults(ownerIndex); };

public:
	bool SpawnProjectile(sf::Vector2f position, uint32 ownerIndex);

public:
	void StartSession();
	void StopSession();

private:
	ProjectilesManager MainProjectileManager;
	EnemiesManager MainEnemiesManager;
	CollisionManager MainCollisionManager;
	GameMode MainGameMode;

private:
	unsigned int ConnectedClientsAmount{ 0 };

private:
	bool Running{ false };
	bool Empty{ true };
	const uint32 ClientsLimit{ 2 };
	Client Clients[2];
	uint16 SessionID{ 0x00000000 }; //Default invalid ID
};


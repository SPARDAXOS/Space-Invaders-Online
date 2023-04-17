#include "Session.hpp"

Session::Session() {
	uint16 RandomNumber = (uint16)rand();
	SessionID = RandomNumber;
	
	
	MainCollisionManager.SetGameMode(MainGameMode);
	MainCollisionManager.SetEnemiesManager(MainEnemiesManager);
	MainCollisionManager.SetProjectileManager(MainProjectileManager);
	MainCollisionManager.SetupPoolsReferences();

	MainGameMode.SetEnemiesManager(MainEnemiesManager);
	MainGameMode.SetProjectilesManager(MainProjectileManager);
	MainGameMode.SetPlayer1OwnerIndex(Clients[0].GetOwnerIndex());
	MainGameMode.SetPlayer2OwnerIndex(Clients[1].GetOwnerIndex());
}


bool Session::AddClient(IP_Address address) {
	if (ConnectedClientsAmount >= ClientsLimit) {
		printf("Error attempted to add client to full session");
		return false;
	}

	for (auto& x : Clients) {
		if (x.GetStatus() == ConnectionStatus::DISCONNECTED) {
			x.SetAddress(address);
			x.SetStatus(ConnectionStatus::CONNECTING);
			ConnectedClientsAmount++;
			if (ConnectedClientsAmount > 0)
				Empty = false;
			return true;
		}
	}
	return false;
}
bool Session::RemoveClient(IP_Address address) {
	Client* FoundClient = nullptr;

	if (DoesClientExist(address, FoundClient)) {
		FoundClient->SetStatus(ConnectionStatus::DISCONNECTED);
		FoundClient->SetAddress({});
		FoundClient->SetPosition(sf::Vector2f(0.0f, 0.0f));
		FoundClient->SetLastReceivedSequence(0);
		FoundClient->SetLastReceivedPacketTimestamp(0);
		FoundClient->SetLastProcessedInputTick(0);
		FoundClient->SetShootRequest(false);
		FoundClient->SetReadyCheckState(false);
		FoundClient->ResetTimeoutCounter();
		FoundClient->ClearQueuedInputs();
		FoundClient->ClearQueuedInputsTimestamps();
		FoundClient->ResetRTTData();


		ConnectedClientsAmount--;

		if (ConnectedClientsAmount <= 0)
			Empty = true;
		return true;
	}
	else {
		printf("Error attempted to remove a client that doesnt exist!\n");
		return false;
	}
}


bool Session::IsSessionFull() const {
	return ConnectedClientsAmount >= ClientsLimit;
}
bool Session::IsSessionEmpty() const {
	return Empty;
}
bool Session::IsSessionRunning() const {
	return Running;
}


bool Session::DoesClientExist(IP_Address address, Client*& client) {
	for (auto& x : Clients) {
		if (x.GetAddress() == address) {
			client = &x;
			return true;
		}
	}
	return false;
}


uint16 Session::GetSessionID() const {
	return SessionID;
}
void Session::GetClients(Client*& clients) {
	clients = Clients;
}
bool Session::GetOtherClient(Client& client, Client*& otherClient) {
	if (ConnectedClientsAmount <= 1)
		return false;

	for (auto& x : Clients) {
		if (x.GetAddress() != client.GetAddress()) {
			otherClient = &x;
			return true;
		}
	}
	return false;
}


uint32 Session::GetActiveEnemiesBits() {
	return MainEnemiesManager.GetActiveEnemiesBits();
}
bool Session::GetActiveEnemiesPositions(uint32*& xPositions, uint32*& yPositions) {
	return MainEnemiesManager.GetActiveEnemiesPositions(xPositions, yPositions);
}
bool Session::GetPlayerScore(Client& client, uint32*& scorePointer) {
	return MainGameMode.GetPlayerScore(client.GetOwnerIndex(), scorePointer);
}


bool Session::SpawnProjectile(sf::Vector2f position, uint32 ownerIndex) {
	return MainProjectileManager.SpawnProjectile(position, ownerIndex);
}


void Session::StartSession() {
	if (Running) //Protection
		return;

	MainGameMode.StartGame();
	Running = true;
}
void Session::StopSession() {
	if (!Running) //Protection
		return;

	MainGameMode.StopGame();
	Running = false;
}


void Session::UpdateSession(sf::Time deltaTime) {

	MainCollisionManager.Update();
	MainProjectileManager.Update(deltaTime);
	MainEnemiesManager.Update(deltaTime);
	MainGameMode.Update();

	if (Running) {
		if (!MainGameMode.IsGameRunning())
			Running = false;
	}

}
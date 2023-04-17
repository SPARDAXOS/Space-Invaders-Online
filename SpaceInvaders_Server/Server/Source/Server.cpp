#include "Server.hpp"


void Server::Run() {
	if (!Start()) {
		PrintMessage("Could not start the server.");
		return;
	}
	while (Running) {
		if (!Update())
			Running = false;
	}
	End();
}
bool Server::Start() {
	InputTick = 0;
	TimeBetweenEachTick = 1.0f / TickRate;
	TickTimer = 0.0f;

	CurrentConnectedClients = 0;
	ClientsUpdateTimer = 0.0f;

	CurrentPayloadSequenceNumber = 0;
	LastConnectionPacketSequence = 0;

	SentPayloadsIndex = 0;

	Running = true;

	OpenAddress = {0, 0, 0, 0, DefaultServerPort};
	if (Socket.Open(OpenAddress))
		PrintMessage("Server Online at", OpenAddress);
	else
		PrintLastCriticalError();

	return true;
}
void Server::End() {
	PrintMessage("Server Offline");
	Socket.Close();
}


bool Server::Receive() {
	BitStream NewBitStream{};
	IP_Address SenderIP_Address;

	if (Socket.Receive(SenderIP_Address, NewBitStream)) {
		BitStreamReader NewBitStreamReader{ NewBitStream };

		switch (NewBitStreamReader.Peek()) {
		case 0: {
			if (ShowPacketsLog)
				PrintMessage("Connection packet received", SenderIP_Address);

			ConnectionPacket ReceivedConnectionPacket;
			if (!ReceivedConnectionPacket.Read(NewBitStreamReader))
				PrintMessage("Error reading connection packet");

			ProcessConnectionPacket(SenderIP_Address, ReceivedConnectionPacket);
		}break;
		case 1: {
			if (ShowPacketsLog)
				PrintMessage("Disconnection packet received", SenderIP_Address);

			DisconnectionPacket ReceivedDisconnectionPacket;
			if (!ReceivedDisconnectionPacket.Read(NewBitStreamReader))
				PrintMessage("Error reading disconnection packet");

			ProcessDisconnectionPacket(SenderIP_Address, ReceivedDisconnectionPacket);
		}break;
		case 2: {
			if (ShowPacketsLog)
				PrintMessage("Payload packet received", SenderIP_Address);

			PayloadPacket ReceivedPayloadPacket;
			if (!ReceivedPayloadPacket.Read(NewBitStreamReader))
				PrintMessage("Error reading payload packet");

			ProcessPayloadPacket(SenderIP_Address, ReceivedPayloadPacket);
		}break;
		default:
			PrintMessage("Unknown packet type received", SenderIP_Address);
		}

		return true;
	}
	return false;
}
bool Server::Send(IP_Address address, ConnectionPacket packet) {
	BitStream NewBitStream{};
	BitStreamWriter NewBitStreamWriter{ NewBitStream };

	if (!packet.Write(NewBitStreamWriter)) {
		PrintMessage("Failed to write connection packet data");
		return false;
	}
	if (!Socket.Send(address, NewBitStream)) {
		PrintLastCriticalError();
		return false;
	}

	if (ShowPacketsLog)
		PrintMessage("Connection packet sent", address);

	return true;
}
bool Server::Send(IP_Address address, DisconnectionPacket packet) {
	BitStream NewBitStream{};
	BitStreamWriter NewBitStreamWriter{ NewBitStream };

	if (!packet.Write(NewBitStreamWriter)) {
		PrintMessage("Failed to write disconnection packet data");
		return false;
	}
	if (!Socket.Send(address, NewBitStream)) {
		PrintLastCriticalError();
		return false;
	}

	if (ShowPacketsLog)
		PrintMessage("Disconnection packet sent", address);

	return true;
}
bool Server::Send(IP_Address address, PayloadPacket packet) {
	BitStream NewBitStream{};
	BitStreamWriter NewBitStreamWriter{ NewBitStream };

	if (!packet.Write(NewBitStreamWriter)) {
		PrintMessage("Failed to write payload packet data");
		return false;
	}
	if (!Socket.Send(address, NewBitStream)) {
		PrintLastCriticalError();
		return false;
	}

	RegisterSentPayloadPacket(CurrentPayloadSequenceNumber);
	IncrementPayloadSequenceNumber();

	if (ShowPacketsLog)
		PrintMessage("Payload packet sent", address);

	return true;
}


void Server::ProcessConnectionPacket(IP_Address& address, ConnectionPacket packet) {
	if (IsServerFull()) {
		SendDisconnectionPacket(address, DisconnectionReason::SERVER_FULL);
		return;
	}
	if (!IsProtocolIDValid(packet.ProtocolID.Data)) {
		SendDisconnectionPacket(address, DisconnectionReason::INVALID_PROTOCOL_ID);
		return;
	}
	if (!IsProtocolVersionValid(packet.ProtocolVersion.Data)) {
		SendDisconnectionPacket(address, DisconnectionReason::INVALID_PROTOCOL_VERSION);
		return;
	}

	Client* FoundClient = nullptr;
	for (auto& x : Sessions)
		if (x.DoesClientExist(address, FoundClient) && FoundClient != nullptr) {
			PrintMessage("Connection packet received from connected client", address);
			return;
		}

	ConnectionPacket ConnectionConfimationPacket;
	ConnectionConfimationPacket.ProtocolID.Data = ProtocolID;
	ConnectionConfimationPacket.ProtocolVersion.Data = ProtocolVersion;

	//Reconnection or connection to specific session
	Session* FoundSession = nullptr;
	if (DoesSessionExist((uint16)packet.SessionID.Data, FoundSession)) {
		//Check if there is a vacant spot in the session 
		if (!FoundSession->IsSessionFull()) {
			FoundSession->AddClient(address);
			ConnectionConfimationPacket.SessionID.Data = FoundSession->GetSessionID();
			Send(address, ConnectionConfimationPacket);
			PrintMessage("Client reconnecting...", address);
			return;
		}
	}
	//Connecting to new session
	for (auto& x : Sessions) {
		if (!x.IsSessionFull()) {
			x.AddClient(address);
			ConnectionConfimationPacket.SessionID.Data = x.GetSessionID();
			Send(address, ConnectionConfimationPacket);
			PrintMessage("Client connecting...", address);
			return;
		}
	}
}
void Server::ProcessDisconnectionPacket(IP_Address& address, DisconnectionPacket packet) {
	DisconnectClient(address, packet.SessionID.Data, (DisconnectionReason)packet.Reason.Data);
}
void Server::ProcessPayloadPacket(IP_Address& address, PayloadPacket packet) {
	Session* FoundSession = nullptr;
	if (DoesSessionExist(uint16(packet.SessionID.Data), FoundSession) && FoundSession != nullptr) {
		Client* FoundClient = nullptr;
		if (FoundSession->DoesClientExist(address, FoundClient) && FoundClient != nullptr) {
			//Last receive sequence and timestamp for processing time
			FoundClient->SetLastReceivedSequence(uint16(packet.SequenceNumber.Data));
			FoundClient->SetLastReceivedPacketTimestamp(RunningTime.asMilliseconds());

			//RTT Calculation - Only when connected
			if (FoundClient->GetStatus() != ConnectionStatus::CONNECTING) {
				uint32 PacketTimestamp = 0;
				if (WasPayloadPacketSent((uint16)packet.AcknowledgeNumber.Data, PacketTimestamp)) {
					uint32 Delta = RunningTime.asMilliseconds() - PacketTimestamp;
					uint32 RTT = Delta - packet.ProcessingTime.Data;
					FoundClient->AddRTTCalculation(RTT);
					FoundClient->CalculateAverageRTT();
				}
			}


			//Timeout
			FoundClient->ConfirmConnection();
			//Shoot
			FoundClient->SetShootRequest(packet.Player1ActionShoot.Data);
			//Ready check - Always confirms it
			FoundClient->SetReadyCheckState(packet.Player1ReadyCheck.Data);
			//Position
			sf::Vector2f Position = sf::Vector2f((float)packet.Player1PositionX.Data, (float)packet.Player1PositionY.Data);
			FoundClient->SetPosition(Position);
			//Input queues
			FoundClient->QueueInput((MovementInputRequestType)packet.InputRequest.Data);
			FoundClient->QueueInputTimestamp(packet.InputTick.Data);

			//Confirm connection if connecting
			if (FoundClient->GetStatus() == ConnectionStatus::CONNECTING)
				SendConnectionConfirmationPacket(*FoundSession, *FoundClient);
		}
		else {
			PrintMessage("Error client was not registered in session while processing payload data", address);
			return;
		}
	}
	else {
		PrintMessage("Invalid session ID received in payload from", address);
		DisconnectClient(address, DisconnectionReason::INVALID_SESSION_ID);
		return;
	}
}


inline void Server::IncrementPayloadSequenceNumber() {
	CurrentPayloadSequenceNumber++;
}
inline void Server::RegisterSentPayloadPacket(uint16 sequenceNumber) {
	SentPayloadsHistory[SentPayloadsIndex] = sequenceNumber;
	SentPayloadsTimestampsHistory[SentPayloadsIndex] = RunningTime.asMilliseconds();
	SentPayloadsIndex = (SentPayloadsIndex + 1) % 100;
}
bool Server::WasPayloadPacketSent(uint16 sequenceNumber, uint32& timestamp) {
	for (uint32 i = 0; i < 100; i++) {
		if (SentPayloadsHistory[i] == sequenceNumber) {
			timestamp = SentPayloadsTimestampsHistory[i];
			return true;
		}
	}
	return false;
}


void Server::DisconnectClient(IP_Address address, uint32 sesssionID, DisconnectionReason reason) {
	Session* FoundSession = nullptr;
	if (DoesSessionExist((uint16)sesssionID, FoundSession) && FoundSession != nullptr) {
		if (FoundSession->RemoveClient(address)) {
			CurrentConnectedClients--;
			SendDisconnectionPacket(address, reason);
			return;
		}
		else
			PrintMessage("Disconnection request was ignored - Reason: Client was not found in session", address);
	}

	PrintMessage("Disconnection request was ignored - Reason: Session was not found", address);
}
void Server::DisconnectClient(IP_Address address, DisconnectionReason reason) {
	Client* FoundClient = nullptr;
	for (auto& x : Sessions) {
		if (x.DoesClientExist(address, FoundClient) && FoundClient != nullptr) {
			x.RemoveClient(address);
			CurrentConnectedClients--;
			SendDisconnectionPacket(address, reason);
			return;
		}
	}
	PrintMessage("Disconnection request was ignored - Reason: Client was not found in any session", address);
}
void Server::SendDisconnectionPacket(IP_Address address, DisconnectionReason reason) {
	DisconnectionPacket NewDisconnectionPacket;
	NewDisconnectionPacket.Reason.Data = (uint32)reason;

	PrintDisconnectionMessage(address, reason);
	Send(address, NewDisconnectionPacket);
}
void Server::SendConnectionConfirmationPacket(Session& session, Client& client) {
	client.SetStatus(ConnectionStatus::CONNECTED);
	
	PayloadPacket ConnectionConfirmationPacket;
	ConnectionConfirmationPacket.SessionID.Data = session.GetSessionID();
	ConnectionConfirmationPacket.SequenceNumber.Data = (uint32)CurrentPayloadSequenceNumber;
	ConnectionConfirmationPacket.AcknowledgeNumber.Data = (uint32)client.GetLastReceivedSequence();


	//Processing Time
	ConnectionConfirmationPacket.ProcessingTime.Data
		= RunningTime.asMilliseconds() - client.GetLastReceivedPacketTimestamp();

	//Position
	sf::Vector2f Client1Position = client.GetPosition();
	ConnectionConfirmationPacket.Player1PositionX.Data = (uint32)Client1Position.x;
	ConnectionConfirmationPacket.Player1PositionY.Data = (uint32)Client1Position.y;

	//Input Tick
	ConnectionConfirmationPacket.InputTick.Data = client.GetLastProcessedInputTick();


	
	//Data from other client if connected!
	if (session.IsSessionFull()) {
		Client* OtherClient = nullptr;
		if (session.GetOtherClient(client, OtherClient) && OtherClient != nullptr) {
			sf::Vector2f OtherClientPosition = OtherClient->GetPosition();
			ConnectionConfirmationPacket.Player2PositionX.Data = (uint32)OtherClientPosition.x;
			ConnectionConfirmationPacket.Player2PositionY.Data = (uint32)OtherClientPosition.y;
			ConnectionConfirmationPacket.IsPlayer2Connected.Data = true;
			ConnectionConfirmationPacket.Player2ReadyCheck.Data = OtherClient->GetReadyCheckState();
			uint32* PlayerScore = nullptr;
			if (session.GetPlayerScore(*OtherClient, PlayerScore) && PlayerScore != nullptr)
				ConnectionConfirmationPacket.Player2Score.Data = *PlayerScore;
		}
	}
	
	PrintMessage("Client connected", client.GetAddress());
	if (Send(client.GetAddress(), ConnectionConfirmationPacket))
		CurrentConnectedClients++;
	else
		PrintLastCriticalError();
}


bool Server::DoesSessionExist(uint16 ID, Session*& ptr) {
	for (auto& x : Sessions) {
		if (x.GetSessionID() == ID) {
			ptr = &x;
			return true;
		}
	}
	return false;
}
uint32 Server::GetConnectedClientsAmount() const {
	return CurrentConnectedClients;
}


void Server::ConfirmActionRequests(Client& clients) {
	Client* Clients = &clients;
	if (Clients == nullptr)
		return;

	for (uint32 i = 0; i < 2; i++) {
		if (Clients[i].GetShootRequest())
			Clients[i].SetShootRequest(false);
	}
}
void Server::ProcessInputRequest(Client& client) {
	std::vector<MovementInputRequestType>* Inputs = &client.GetQueuedInputs();
	std::vector<uint32>* InputsTimestamps = &client.GetQueuedInputsTimestamps();

	while (Inputs->size() > 0) {

		sf::Vector2f Direction;
		sf::Vector2f Velocity;
		sf::Vector2f CurrentPosition = client.GetPosition();
		sf::Vector2f NewPosition;

		if ((*Inputs)[0] == MovementInputRequestType::LEFT)
			Direction = sf::Vector2f(-1.0f, 0.0f);
		else if ((*Inputs)[0] == MovementInputRequestType::RIGHT)
			Direction = sf::Vector2f(1.0f, 0.0f);
		else if ((*Inputs)[0] == MovementInputRequestType::NONE) {
			Inputs->erase(Inputs->begin()); //Pop front basically
			if ((*InputsTimestamps).size() == 1)
				client.SetLastProcessedInputTick((*InputsTimestamps)[0]);
			InputsTimestamps->erase(InputsTimestamps->begin());
			return;
		}


		Velocity = Direction * (float)client.GetMovementSpeed();
		NewPosition = CurrentPosition + Velocity;


		if (NewPosition.x >= PositiveXWorldBound)
			NewPosition.x = PositiveXWorldBound;
		else if (NewPosition.x <= NegativeXWorldBound)
			NewPosition.x = NegativeXWorldBound;

		client.SetPosition(NewPosition);
		Inputs->erase(Inputs->begin()); //Pop front basically
		if ((*InputsTimestamps).size() == 1)
			client.SetLastProcessedInputTick((*InputsTimestamps)[0]);

		InputsTimestamps->erase(InputsTimestamps->begin());
	}
}


void Server::CheckSessionsStates() {
	for (auto& x : Sessions) {
		if (x.IsSessionRunning()) {
			Client* Clients = nullptr;
			x.GetClients(Clients);
			if (Clients != nullptr) {
				for (uint32 i = 0; i < 2; i++) {
					if (Clients[i].GetStatus() != ConnectionStatus::CONNECTED)
						x.StopSession();
				}
			}
		}
	}
}
void Server::UpdateRunningSessions() {
	for (auto& x : Sessions) {
		if (x.IsSessionRunning())
			x.UpdateSession(DeltaTime);
	}
}
void Server::SimulateClients() {
	Client* Clients = nullptr;
	for (auto& x : Sessions) {
		if (!x.IsSessionEmpty()) {
			x.GetClients(Clients);
			if (Clients != nullptr) {
				for (uint32 i = 0; i < 2; i++) {
					if (Clients[i].GetStatus() == ConnectionStatus::CONNECTED)
						ProcessInputRequest(Clients[i]);
				}
			}
		}
	}
}


void Server::UpdatePayloadTransmission() {
	if (CurrentConnectedClients > 0) {
		ClientsUpdateTimer += DeltaTime.asSeconds();
		if (ClientsUpdateTimer >= ClientsUpdateRate) {
			ClientsUpdateTimer = 0.0f;

			Client* Clients = nullptr;
			for (auto& x : Sessions) {
				x.GetClients(Clients);
				for (uint32 i = 0; i < 2; i++)
					if (Clients[i].GetStatus() == ConnectionStatus::CONNECTED) {
						PayloadPacket NewPayloadPacket;
						NewPayloadPacket.SequenceNumber.Data = (uint32)CurrentPayloadSequenceNumber;
						NewPayloadPacket.AcknowledgeNumber.Data = (uint32)Clients[i].GetLastReceivedSequence();
						NewPayloadPacket.SessionID.Data = x.GetSessionID();

						//Processing Time
						NewPayloadPacket.ProcessingTime.Data
							= RunningTime.asMilliseconds() - Clients[i].GetLastReceivedPacketTimestamp();

						//Position
						sf::Vector2f Client1Position = Clients[i].GetPosition();
						NewPayloadPacket.Player1PositionX.Data = (uint32)Client1Position.x;
						NewPayloadPacket.Player1PositionY.Data = (uint32)Client1Position.y;

						//Enemies Manager
						NewPayloadPacket.ActiveEnemiesBits.Data = x.GetActiveEnemiesBits();
						uint32* XPositions = nullptr;
						uint32* YPositions = nullptr;
						if (x.GetActiveEnemiesPositions(XPositions, YPositions)) {
							if (XPositions != nullptr && YPositions != nullptr) {
								for (uint32 j = 0; j < 25; j++) {
									NewPayloadPacket.EnemiesXPositions[j].Data = XPositions[j];
									NewPayloadPacket.EnemiesYPositions[j].Data = YPositions[j];
								}
							}
						}
						else {
							PrintMessage("Error failed to get enemies positions");
							return;
						}

						//Projectiles Manager
						if (Clients[i].GetShootRequest()) {
							NewPayloadPacket.Player1ActionShoot.Data
								= x.SpawnProjectile(Clients[i].GetPosition(), Clients[i].GetOwnerIndex());
						}

						//Ready Check
						NewPayloadPacket.Player1ReadyCheck.Data = (uint32)Clients[i].GetReadyCheckState();

						//Score
						uint32* Player1ScorePointer = nullptr;
						if (x.GetPlayerScore(Clients[i], Player1ScorePointer) && Player1ScorePointer != nullptr)
							NewPayloadPacket.Player1Score.Data = *Player1ScorePointer;

						//Input Tick
						NewPayloadPacket.InputTick.Data = Clients[i].GetLastProcessedInputTick();

						//Player 2 Data
						Client* OtherClient = nullptr;
						if (x.GetOtherClient(Clients[i], OtherClient) && OtherClient != nullptr)
							if (OtherClient->GetStatus() == ConnectionStatus::CONNECTED) {
								//Player2 position - To get last visible location in case of disconnect
								sf::Vector2f Client2Position = OtherClient->GetPosition();
								NewPayloadPacket.Player2PositionX.Data = (uint32)Client2Position.x;
								NewPayloadPacket.Player2PositionY.Data = (uint32)Client2Position.y;

								//Score
								uint32* Player2ScorePointer = nullptr;
								if (x.GetPlayerScore(*OtherClient, Player2ScorePointer) && Player2ScorePointer != nullptr)
									NewPayloadPacket.Player2Score.Data = *Player2ScorePointer;

								//Shoot
								NewPayloadPacket.Player2ActionShoot.Data = OtherClient->GetShootRequest();

								//Is Player 2 Connected
								NewPayloadPacket.IsPlayer2Connected.Data = true;
								//Is Player 2 Ready
								NewPayloadPacket.Player2ReadyCheck.Data = OtherClient->GetReadyCheckState();

								//Game Start!
								if (OtherClient->GetReadyCheckState() && Clients[i].GetReadyCheckState())
									x.StartSession();
							}

						//Game Start
						NewPayloadPacket.StartGame.Data = x.IsSessionRunning();

						//Game Results
						NewPayloadPacket.GameResults.Data = (uint32)x.GetGameResults(Clients[i].GetOwnerIndex());

						Send(Clients[i].GetAddress(), NewPayloadPacket);
					}
				ConfirmActionRequests(*Clients);
			}
		}
	}
}
void Server::CheckForTimeouts() {
	Client* Clients = nullptr;
	for (auto& x : Sessions) {
		x.GetClients(Clients);
		for (uint32 i = 0; i < 2; i++) {
			if (Clients[i].GetStatus() == ConnectionStatus::CONNECTED) {
				Clients[i].UpdateTimeoutCounter(DeltaTime.asSeconds());
				if (Clients[i].GetTimeoutCounter() >= TimeoutThreshold)
					DisconnectClient(Clients[i].GetAddress(), x.GetSessionID(), DisconnectionReason::TIMEOUT);
			}
		}
	}
}
void Server::CheckClientsLatency() {
	Client* Clients = nullptr;
	for (auto& x : Sessions) {
		x.GetClients(Clients);
		for (uint32 i = 0; i < 2; i++) {
			if (Clients[i].GetStatus() == ConnectionStatus::CONNECTED) {
				if (Clients[i].IsAverageRTTAtThreshold(LatencyThreshold))
					DisconnectClient(Clients[i].GetAddress(), x.GetSessionID(), DisconnectionReason::HIGH_LATENCY);
			}
		}
	}
}


inline bool Server::IsServerFull() const {
	return CurrentConnectedClients >= SessionsLimit * 2;
}
inline bool Server::IsProtocolIDValid(uint32 id) const {
	if (ProtocolID == id)
		return true;
	return false;
}
inline bool Server::IsProtocolVersionValid(uint32 version) const {
	if (ProtocolVersion == version)
		return true;
	return false;
}


inline void Server::PrintMessage(const char* message) const {

	printf("[ %dms ] %s\n", RunningTime.asMilliseconds(), message);
}
inline void Server::PrintMessage(const char* message, uint32 num) const {
	printf("[ %dms ] %s %d\n", RunningTime.asMilliseconds(), message, num);
}
inline void Server::PrintMessage(const char* message, IP_Address address) const {
	//HORRIBLE but it works
	printf("[ %dms ] %s - %d.%d.%d.%d:%d\n", RunningTime.asMilliseconds(), message, address.a(), address.b(), address.c(), address.d(), address.m_port);
}
inline void Server::PrintLastCriticalError() const {
	auto Error = Network::get_last_error();
	if (Error.is_critical())
		PrintMessage(Error.as_string());
}
inline void Server::PrintDisconnectionMessage(IP_Address address, DisconnectionReason reason) const {
	switch (reason) {
	case DisconnectionReason::UKNOWN: {
		PrintMessage("Connection rejected - Reason: Unkown", address);
		return;
	}
	case DisconnectionReason::INVALID_PROTOCOL_ID: {
		PrintMessage("Connection rejected - Reason: Invalid protocol ID"), address;
		return;
	}
	case DisconnectionReason::INVALID_PROTOCOL_VERSION: {
		PrintMessage("Connection rejected - Reason: Invalid protocol Version"), address;
		return;
	}
	case DisconnectionReason::SERVER_FULL: {
		PrintMessage("Connection rejected - Reason: Server is full"), address;
		return;
	}
	case DisconnectionReason::TIMEOUT: {
		PrintMessage("Client has been disconnected - Reason: Timed out"), address;
		return;
	}
	case DisconnectionReason::INVALID_SESSION_ID: {
		PrintMessage("Client has been disconnected - Reason: Invalid session ID"), address;
		return;
	}
	case DisconnectionReason::USER_CHOICE: {
		PrintMessage("Client has been disconnected - Reason: User choice"), address;
		return;
	}
	case DisconnectionReason::HIGH_LATENCY: {
		PrintMessage("Client has been disconnected - Reason: High latency", address);
		return;

	}
	}
}


void Server::ProcessTick() {
	TickTimer += DeltaTime.asSeconds();
	if (TickTimer >= TimeBetweenEachTick) {
		TickTimer -= TimeBetweenEachTick;

		SimulateClients();

		InputTick++;
	}
}
bool Server::Update() {
	DeltaTime = Clock.restart();
	RunningTime += DeltaTime;

	Receive();

	CheckForTimeouts();
	CheckClientsLatency();

	CheckSessionsStates();
	UpdateRunningSessions();

	UpdatePayloadTransmission();
	ProcessTick();

	return true;
}
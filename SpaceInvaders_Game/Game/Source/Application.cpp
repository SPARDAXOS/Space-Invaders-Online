#include "application.hpp"


void Application::Run() {
    const sf::VideoMode Mode{ 800, 600 };
    const sf::Uint32 Flags = sf::Style::Titlebar | sf::Style::Close;
    Window.create(Mode, "Space Invaders", Flags);
    if (!Window.isOpen() || !Start()) {
        return;
    }

    Window.setKeyRepeatEnabled(false);
    while (Window.isOpen()) {
        sf::Event event;
        while (Window.pollEvent(event)) {
            if (event.type == sf::Event::MouseMoved) {
                OnMouseMove(event.mouseMove.x, event.mouseMove.y);
            }
            else if (event.type == sf::Event::MouseButtonPressed) {
                OnButtonPressed(event.mouseButton.button);
            }
            else if (event.type == sf::Event::MouseButtonReleased) {
                OnButtonReleased(event.mouseButton.button);
            }
            else if (event.type == sf::Event::KeyPressed) {
                OnKeyPressed(event.key.code);
            }
            else if (event.type == sf::Event::KeyReleased) {
                OnKeyReleased(event.key.code);
            }
            else if (event.type == sf::Event::Closed) {
                Window.close();
            }
        }

        if (!Update()) {
            Window.close();
        }

        Render();
    }

    End();
}
bool Application::Start() {
    AverageRTT = 0;
    UploadRate = 0;
    DownloadRate = 0;
    SendingPPS = 0;
    ReceivingPPS = 0;
    SendingPPSAccumlator = 0;
    ReceivingPPSAccumlator = 0;
    TotalBytesSent = 0;
    TotalBytesReceived = 0;
    TotalPacketsSent = 0;
    TotalPacketsReceived = 0;
    TotalMessagesSent = 0;
    TotalMessagesReceived = 0;
    TotalInputMispredictions = 0;


    PayloadTransmissionRateTimer = 0.0f;
    TimeoutThresholdTimer = 0.0f;
    PPSCalculationRateTimer = 0.0f;
    ServerSearchDurationTimer = 0.0f;
    ServerSearchPPSRateTimer = 0.0f;
    ShootingRateTimer = ShootingRate; 


    IsPlayer2Connected = false;
    Player1ShootRequest = false;
    ReadyCheckRequest = false;
    Player1ShootConfirm = false;
    Player2ShootConfirm = false;


    SentPayloadsIndex = 0;
    CurrentPayloadSequenceNumber = 0;
    LastReceivedPayloadSequence = 0;

    CalculatedRTTsIndex = 0;


    InputTick = 0;
    TickTimer = 0.0f;
    TimeBetweenEachTick = 1.0f / TickRate;


    CurrentConnectionStatus = ConnectionStatus::DISCONNECTED;
    Running = true;
    

    SetupNetworkOverlayData();

    SetupPlayer1();
    SetupPlayer2();


    //Collision manager - Visual only
    MainCollisionManager.SetEnemiesManager(MainEnemiesManager);
    MainCollisionManager.SetProjectileManager(MainProjectileManager);
    MainCollisionManager.SetupPoolsReferences();

    //Inputinator
    MainInputinator.SetCurrentTickReference(InputTick);

    //GameMode 
    MainGameMode.SetCurrentConnectionStatus(CurrentConnectionStatus);
    MainGameMode.SetFont(MainFont); //After SetupNetworkOverlayData() so the font is loaded
    MainGameMode.SetupTexts();

    //EnemiesManager
    MainEnemiesManager.SetInterpolatorReference(MainInterpolator);

    OwnAddress = FindOwnIPAddress();
    PrintMessage("Own IP-address is ", OwnAddress);
    BroadcastAddress = IP_Address{ OwnAddress.a(), OwnAddress.b(), OwnAddress.c() , 255, DefaultServerPort }; //Depends on subnet mask
    Socket.Open({});

    return true;
}
void Application::End() {
    Running = false;
    Socket.Close();
}


void Application::Connect() {
    ConnectionStatus Status1 = ConnectionStatus::CONNECTED;
    ConnectionStatus Status2 = ConnectionStatus::CONNECTING;
    if (CurrentConnectionStatus == Status1 || CurrentConnectionStatus == Status2) //Quick bail
        return;

    BitStream NewBitStream{};
    BitStreamWriter NewBitStreamWriter{ NewBitStream };
    ConnectionPacket NewConnectionPacket = CreateConnectionPacket();

    if (!NewConnectionPacket.Write(NewBitStreamWriter)) {
        PrintMessage("Error writing connection packet");
        return;
    }
    if (!Socket.Send(BroadcastAddress, NewBitStream)) {
        PrintLastCriticalError();
        return;
    }

    PrintMessage("Searching for a server...");
}
void Application::Disconnect() {
    if (CurrentConnectionStatus != ConnectionStatus::CONNECTED) //Quick bail
        return;

    BitStream NewBitStream{};
    BitStreamWriter NewBitStreamWriter{ NewBitStream };
    DisconnectionPacket NewDisconnectionPacket = CreateDisconnectionPacket(DisconnectionReason::USER_CHOICE);

    if (!NewDisconnectionPacket.Write(NewBitStreamWriter)) {
        PrintMessage("Error writing connection packet");
        return;
    }
    if (!Socket.Send(ConnectionAddress, NewBitStream)) {
        PrintLastCriticalError();
        return;
    }

    ResetConnectionData();

    CurrentConnectionStatus = ConnectionStatus::DISCONNECTING;
    PrintMessage("Disconnecting...");
}
void Application::Timeout() {
    ResetConnectionData();
    CurrentConnectionStatus = ConnectionStatus::DISCONNECTED;
    PrintDisconnectionMessage(DisconnectionReason::TIMEOUT);
}


bool Application::Receive() {
    BitStream NewBitStream{};
    IP_Address SenderAddress;

    if (Socket.Receive(SenderAddress, NewBitStream)) {
        BitStreamReader NewBitStreamReader{ NewBitStream };

        ReceivingPPSAccumlator++;

        TotalBytesReceived += NewBitStream.Size;

        TotalPacketsReceived++;

        switch (NewBitStreamReader.Peek()) {
        case 0: {
            if (CurrentConnectionStatus == ConnectionStatus::SEARCHING) {
                ConnectionPacket ReceivedConnectionPacket;
                ReceivedConnectionPacket.Read(NewBitStreamReader);

                if (ShowPacketsLog)
                    PrintMessage("Connection packet received from ", SenderAddress);

                TotalMessagesReceived += 4; //Total messages in a connection packet

                if (ValidateConnectionPacket(SenderAddress, ReceivedConnectionPacket)) {
                    PrintMessage("Connecting to server at", SenderAddress);
                    ConnectionAddress = SenderAddress;
                    CurrentConnectionStatus = ConnectionStatus::CONNECTING;
                    SessionID = uint16(ReceivedConnectionPacket.SessionID.Data);

                    //Payload response
                    PayloadPacket NewPayloadPacket = CreatePayloadPacket();
                    
                    Send(NewPayloadPacket);
                }
            }
            else //Just for catching such events
                PrintMessage("A connection packet was received while not searching for a server");
        }break;
        case 1: {
            DisconnectionPacket ReceivedDisconnectionPacket;
            ReceivedDisconnectionPacket.Read(NewBitStreamReader);

            if (ShowPacketsLog)
                PrintMessage("Disconnection packet received from server at", SenderAddress);

            TotalMessagesReceived += 3; //Total messages in a disconnection packet

            if (CurrentConnectionStatus == ConnectionStatus::DISCONNECTED)
                PrintMessage("Client is disconnected and a disconnection packet was received from", SenderAddress);

            DisconnectionReason Reason = (DisconnectionReason)ReceivedDisconnectionPacket.Reason.Data;
            CurrentConnectionStatus = ConnectionStatus::DISCONNECTED;
            PrintDisconnectionMessage(Reason);

            ResetConnectionData();

            ClearNetworkOverlayData();
        }break;
        case 2: {
            ConnectionStatus Status1 = ConnectionStatus::CONNECTING;
            ConnectionStatus Status2 = ConnectionStatus::CONNECTED;

            if (CurrentConnectionStatus == Status1 || CurrentConnectionStatus == Status2) {
                PayloadPacket ReceivedPayloadPacket;
                ReceivedPayloadPacket.Read(NewBitStreamReader);

                if (ShowPacketsLog)
                    PrintMessage("Payload packet received from server at", SenderAddress);

                TotalMessagesReceived += 71; //Total messages in a payload packet

                //RTT Calculation
                uint32 PacketTimestamp = 0;
                if (WasPayloadPacketSent((uint16)ReceivedPayloadPacket.AcknowledgeNumber.Data, PacketTimestamp)) {
                    uint32 Delta = RunningTime.asMilliseconds() - PacketTimestamp;
                    uint32 RTT = Delta - ReceivedPayloadPacket.ProcessingTime.Data;
                    AddRTTCalculation(RTT);
                }


                //EnemiesManager
                MainEnemiesManager.ApplyEnemiesAmountBits(ReceivedPayloadPacket.ActiveEnemiesBits.Data);
                uint32 XPositions[25]{ 0 };
                uint32 YPositions[25]{ 0 }; 
                for (uint32 i = 0; i < 25; i++) {
                    XPositions[i] = ReceivedPayloadPacket.EnemiesXPositions[i].Data;
                    YPositions[i] = ReceivedPayloadPacket.EnemiesYPositions[i].Data;
                }
                MainEnemiesManager.ApplyPositions(XPositions, YPositions);


                //IsPlayer2Connected
                IsPlayer2Connected = ReceivedPayloadPacket.IsPlayer2Connected.Data;
         

                //Player 1 Position
                float X = (float)ReceivedPayloadPacket.Player1PositionX.Data;
                float Y = (float)ReceivedPayloadPacket.Player1PositionY.Data;
                sf::Vector2f ServerPositionResults = sf::Vector2f(X, Y);
                sf::Vector2f FinalPosition
                    = MainInputinator.CompareResults(Player1.getPosition(), ServerPositionResults, ReceivedPayloadPacket.InputTick.Data);
                Player1.setPosition(FinalPosition);

                //Player2 Position
                Player2TargetPosition.x = (float)ReceivedPayloadPacket.Player2PositionX.Data;
                Player2TargetPosition.y = (float)ReceivedPayloadPacket.Player2PositionY.Data;

                //Avoid player 2 position snap
                if (!IsPlayer2Connected && (bool)ReceivedPayloadPacket.IsPlayer2Connected.Data)
                    Player2.setPosition(Player2TargetPosition);

                //Confirming Connection!
                if (CurrentConnectionStatus == ConnectionStatus::CONNECTING) {
                    CurrentConnectionStatus = ConnectionStatus::CONNECTED;
                    PrintMessage("Connection established with server at", SenderAddress);
                }


                //Shooting
                Player1ShootConfirm = ReceivedPayloadPacket.Player1ActionShoot.Data;
                Player2ShootConfirm = ReceivedPayloadPacket.Player2ActionShoot.Data;

                //Score
                MainGameMode.SetPlayer1Score(ReceivedPayloadPacket.Player1Score.Data);
                MainGameMode.SetPlayer2Score(ReceivedPayloadPacket.Player2Score.Data);

                //Ready check
                MainGameMode.SetPlayer1ReadyCheck(ReceivedPayloadPacket.Player1ReadyCheck.Data);
                MainGameMode.SetPlayer2ReadyCheck(ReceivedPayloadPacket.Player2ReadyCheck.Data);


                //Game Management
                if ((bool)ReceivedPayloadPacket.StartGame.Data && !MainGameMode.IsGameRunning()) {
                    MainGameMode.StartGame();
                    ReadyCheckRequest = false;
                }
                else if (!(bool)ReceivedPayloadPacket.StartGame.Data && MainGameMode.IsGameRunning()) {
                    if (IsPlayer2Connected) {
                        MainEnemiesManager.Reset();
                        MainGameMode.FinishGame((GameMode::GameResults)ReceivedPayloadPacket.GameResults.Data);
                    }
                    else if (!IsPlayer2Connected) {//Meh
                        MainEnemiesManager.Reset();
                        MainGameMode.StopGame();
                    }
                }


                //DownloadRate
                DownloadRate = NewBitStream.Size;

                //For assigning it as Acknowledge number
                SetLastReceivedPayloadSequence((uint16)ReceivedPayloadPacket.SequenceNumber.Data);

                //For updating the connection status - Timeout check
                ConfirmConnection();

                //For calculating processing time
                LastReceivedPayloadTime = RunningTime;

                //Calculating interpolation delay
                MainInterpolator.CalculateInterpolationDelay(RunningTime.asSeconds());
            }
            else
                PrintMessage("Payload packet was received while not connecting or being connected");
        }break;
        default:
            PrintMessage("Unknown packet type received from", SenderAddress);
        }

        return true;
    }
    return false;
}
bool Application::Send(ConnectionPacket packet) {
    BitStream NewBitStream{};
    BitStreamWriter NewBitStreamWriter{ NewBitStream };

    if (!packet.Write(NewBitStreamWriter)) {
        PrintMessage("Failed to write connection packet data");
        return false;
    }
    if (!Socket.Send(ConnectionAddress, NewBitStream)) {
        PrintLastCriticalError();
        return false;
    }

    if (ShowPacketsLog)
        PrintMessage("Connection packet sent", ConnectionAddress);


    UploadRate = NewBitStream.Size;

    SendingPPSAccumlator++;
 
    TotalBytesSent += NewBitStream.Size;

    TotalPacketsSent++;

    TotalMessagesSent += 4; 


    return true;
}
bool Application::Send(DisconnectionPacket packet) {
    BitStream NewBitStream{};
    BitStreamWriter NewBitStreamWriter{ NewBitStream };

    if (!packet.Write(NewBitStreamWriter)) {
        PrintMessage("Failed to write disconnection packet data");
        return false;
    }
    if (!Socket.Send(ConnectionAddress, NewBitStream)) {
        PrintLastCriticalError();
        return false;
    }


    if (ShowPacketsLog)
        PrintMessage("Disconnection packet sent", ConnectionAddress);


    UploadRate = NewBitStream.Size;

    SendingPPSAccumlator++;

    TotalBytesSent += NewBitStream.Size;

    TotalPacketsSent++;

    TotalMessagesSent += 3;


    return true;
}
bool Application::Send(PayloadPacket packet) {
    BitStream NewBitStream{};
    BitStreamWriter NewBitStreamWriter{ NewBitStream };

    if (!packet.Write(NewBitStreamWriter)) {
        PrintMessage("Failed to write payload packet data");
        return false;
    }
    if (!Socket.Send(ConnectionAddress, NewBitStream)) {
        PrintLastCriticalError();
        return false;
    }

    if (ShowPacketsLog)
        PrintMessage("Payload packet sent", ConnectionAddress);


    UploadRate = NewBitStream.Size;
    SendingPPSAccumlator++;
    TotalBytesSent += NewBitStream.Size;
    TotalPacketsSent++;
    TotalMessagesSent += 71; 


    RegisterSentPayloadPacket(CurrentPayloadSequenceNumber);
    IncrementPayloadSequenceNumber();


    return true;
}


ConnectionPacket Application::CreateConnectionPacket() {
    ConnectionPacket Packet;
    Packet.ProtocolID.Data = ProtocolID;
    Packet.ProtocolVersion.Data = ProtocolVersion;
    Packet.SessionID.Data = SessionID;
    return Packet;
}
DisconnectionPacket Application::CreateDisconnectionPacket(DisconnectionReason reason) {
    DisconnectionPacket Packet;
    Packet.SessionID.Data = (uint32)SessionID;
    Packet.Reason.Data = (uint32)reason;
    return Packet;
}
PayloadPacket Application::CreatePayloadPacket() {
    PayloadPacket Packet;

    //Essentials
    Packet.SessionID.Data = SessionID;
    Packet.SequenceNumber.Data = CurrentPayloadSequenceNumber;
    Packet.AcknowledgeNumber.Data = LastReceivedPayloadSequence;

    //Movement input
    MovementInputRequestType InputType = MainInputinator.GetLastMovementInputRequest();
    Packet.InputRequest.Data = (uint32)InputType;
    Packet.InputTick.Data = MainInputinator.GetLastInputTick(); // Im always sending last tick i did input on

    //Shoot request
    Packet.Player1ActionShoot.Data = Player1ShootRequest;

    //Process time since last receive - not when connecting
    if (CurrentConnectionStatus != ConnectionStatus::CONNECTING) {
        sf::Time ProcessingTime = RunningTime - LastReceivedPayloadTime;
        Packet.ProcessingTime.Data = ProcessingTime.asMilliseconds();
    }

    //ReadyCheck Request
    Packet.Player1ReadyCheck.Data = ReadyCheckRequest;

    //Player 1 position
    Packet.Player1PositionX.Data = (uint32)Player1.getPosition().x;
    Packet.Player1PositionY.Data = (uint32)Player1.getPosition().y;


    return Packet;
}


IP_Address Application::FindOwnIPAddress() {
    std::vector<IP_Address> addresses;
    if (!IP_Address::GetLocalAddresses(addresses)) {
        return {};
    }

    for (auto& addr : addresses) {
        if (addr.a() == 192) { //First part of your id
            return IP_Address{ addr.m_host, DefaultServerPort };
            break;
        }
    }

    return {};
}
void Application::UpdateServerSearch() {
    ServerSearchPPSRateTimer += DeltaTime.asSeconds();
    if (ServerSearchPPSRateTimer >= ServerSearchPPSRate) {
        ServerSearchPPSRateTimer -= ServerSearchPPSRate; //So the connect attempts and search duration are in sync
        Connect();
    }
}
void Application::UpdateServerSearchDuration() {
    ServerSearchDurationTimer += DeltaTime.asSeconds();
    if (ServerSearchDurationTimer >= ServerSearchDuration) {
        ServerSearchDurationTimer = 0.0f;
        ServerSearchPPSRateTimer = 0.0f; //Responsibility over resetting the connect attempts timer.
        CurrentConnectionStatus = ConnectionStatus::DISCONNECTED;
        PrintMessage("No servers were found");
    }
}


void Application::UpdatePayloadTransmission() {
    PayloadTransmissionRateTimer += DeltaTime.asSeconds();
    if (PayloadTransmissionRateTimer >= PayloadTransmissionRate) {
        PayloadTransmissionRateTimer -= PayloadTransmissionRate;
        PayloadPacket NewPayloadPacket = CreatePayloadPacket();
        Send(NewPayloadPacket);
    }
}
void Application::UpdateTimeoutCheck() {
    TimeoutThresholdTimer += DeltaTime.asSeconds();
    if (TimeoutThresholdTimer >= TimeoutThreshold) {
        TimeoutThresholdTimer = 0.0f;
        Timeout();
    }
}
inline void Application::ConfirmConnection() {
    TimeoutThresholdTimer = 0.0f;
}


inline void Application::SetupPlayer1() {
    Player1.setSize(sf::Vector2f(20.0f, 20.0f));
    Player1.setFillColor(sf::Color::Blue);
    Player1.setPosition(sf::Vector2f(400.0f, 500.0f));
}
inline void Application::SetupPlayer2() {
    Player2.setSize(sf::Vector2f(20.0f, 20.0f));
    Player2.setFillColor(sf::Color::Red);
    Player2.setPosition(sf::Vector2f(400.0f, 500.0f));
    Player2TargetPosition = Player2.getPosition();
}


inline void Application::IncrementPayloadSequenceNumber() {
    CurrentPayloadSequenceNumber++;
}
inline void Application::SetLastReceivedPayloadSequence(uint16 num) {
    LastReceivedPayloadSequence = num;
}
inline void Application::RegisterSentPayloadPacket(uint16 sequenceNumber) {
    SentPayloadsHistory[SentPayloadsIndex] = sequenceNumber;
    SentPayloadsTimestampsHistory[SentPayloadsIndex] = RunningTime.asMilliseconds();
    SentPayloadsIndex = (SentPayloadsIndex + 1) % 100;
}
bool Application::WasPayloadPacketSent(uint16 sequenceNumber, uint32& timestamp) {
    for (uint32 i = 0; i < 100; i++) {
        if (SentPayloadsHistory[i] == sequenceNumber) {
            timestamp = SentPayloadsTimestampsHistory[i];
            return true;
        }
    }
    return false;
}


inline void Application::AddRTTCalculation(uint32 rtt) {
    CalculatedRTTs[CalculatedRTTsIndex] = rtt;
    CalculatedRTTsIndex = (CalculatedRTTsIndex + 1) % 10;
}
inline void Application::CalculateAverageRTT() {
    uint32 Amount = 0;
    uint32 Accumulation = 0;

    for (auto& x : CalculatedRTTs) {
        Accumulation += x;
        Amount++;
    }

    float Results = float(Accumulation / Amount);
    AverageRTT = (uint32)floor(Results);
}
void Application::ResetConnectionData() {
    ConnectionAddress = {};
    for (auto& x : CalculatedRTTs)
        x = 0;

    CalculatedRTTsIndex = 0;

    for (auto& x : SentPayloadsHistory)
        x = 0;
    for (auto& x : SentPayloadsTimestampsHistory)
        x = 0;

    SentPayloadsIndex = 0;
    CurrentPayloadSequenceNumber = 0; //Questionable
    LastReceivedPayloadSequence = 0;
    LastReceivedPayloadTime = {};

    IsPlayer2Connected = false;
    Player1ShootRequest = false;
    ReadyCheckRequest = false;
    Player1ShootConfirm = false;
    Player2ShootConfirm = false;

    PayloadTransmissionRateTimer = 0.0f;
    TimeoutThresholdTimer = 0.0f;
    PPSCalculationRateTimer = 0.0f;
    ServerSearchDurationTimer = 0.0f;
    ServerSearchPPSRateTimer = 0.0f;
    ShootingRateTimer = ShootingRate;

    if (MainGameMode.IsGameRunning()) {
        MainEnemiesManager.Reset();
        MainGameMode.StopGame();
    }


    ClearNetworkOverlayData();
}


void Application::SetupNetworkOverlayData() {
    //Font
    if (!MainFont.loadFromFile("Assets/8bit.ttf")) {
        PrintMessage("Failed to load font from file");
        return;
    }

    //Connection Status
    ConnectionStatusText.setFont(MainFont);
    ConnectionStatusText.setCharacterSize(25);
    ConnectionStatusText.setPosition(0.0f, 0.0f);
    ConnectionStatusText.setString(sf::String("Status: Disconnected"));

    //InputTick
    CurrentTickText.setFont(MainFont);
    CurrentTickText.setCharacterSize(25);
    CurrentTickText.setPosition(0.0f, 25.0f);
    CurrentTickText.setString(sf::String("Tick: 0"));

    //RTT
    RTTText.setFont(MainFont);
    RTTText.setCharacterSize(25);
    RTTText.setPosition(0.0f, 50.0f);
    RTTText.setString(sf::String("RTT: 0ms"));

    //UploadRate
    UploadRateText.setFont(MainFont);
    UploadRateText.setCharacterSize(25);
    UploadRateText.setPosition(0.0f, 75.0f);
    UploadRateText.setString(sf::String("Upload: 0Bytes"));

    //DownloadRate
    DownloadRateText.setFont(MainFont);
    DownloadRateText.setCharacterSize(25);
    DownloadRateText.setPosition(0.0f, 100.0f);
    DownloadRateText.setString(sf::String("Download: 0Bytes"));


    //SendingPPS
    SendingPPSText.setFont(MainFont);
    SendingPPSText.setCharacterSize(25);
    SendingPPSText.setPosition(0.0f, 125.0f);
    SendingPPSText.setString(sf::String("Sending: 0PPS"));

    //ReceivingPPS
    ReceivingPPSText.setFont(MainFont);
    ReceivingPPSText.setCharacterSize(25);
    ReceivingPPSText.setPosition(0.0f, 150.0f);
    ReceivingPPSText.setString(sf::String("Receiving: 0PPS"));


    //TotalBytesSent
    TotalBytesSentText.setFont(MainFont);
    TotalBytesSentText.setCharacterSize(25);
    TotalBytesSentText.setPosition(0.0f, 175.0f);
    TotalBytesSentText.setString(sf::String("Total bytes sent: 0Bytes"));

    //TotalBytesReceived
    TotalBytesReceivedText.setFont(MainFont);
    TotalBytesReceivedText.setCharacterSize(25);
    TotalBytesReceivedText.setPosition(0.0f, 200.0f);
    TotalBytesReceivedText.setString(sf::String("Total bytes received: 0Bytes"));


    //TotalPacketsSent
    TotalPacketsSentText.setFont(MainFont);
    TotalPacketsSentText.setCharacterSize(25);
    TotalPacketsSentText.setPosition(0.0f, 225.0f);
    TotalPacketsSentText.setString(sf::String("Total packets sent: 0Packets"));

    //TotalPacketsReceived
    TotalPacketsReceivedText.setFont(MainFont);
    TotalPacketsReceivedText.setCharacterSize(25);
    TotalPacketsReceivedText.setPosition(0.0f, 250.0f);
    TotalPacketsReceivedText.setString(sf::String("Total packets received: 0Packets"));


    //TotalMessagesSent
    TotalMessagesSentText.setFont(MainFont);
    TotalMessagesSentText.setCharacterSize(25);
    TotalMessagesSentText.setPosition(0.0f, 275.0f);
    TotalMessagesSentText.setString(sf::String("Total messages sent: 0Messages"));

    //TotalMessagesReceived
    TotalMessagesReceivedText.setFont(MainFont);
    TotalMessagesReceivedText.setCharacterSize(25);
    TotalMessagesReceivedText.setPosition(0.0f, 300.0f);
    TotalMessagesReceivedText.setString(sf::String("Total messages received: 0Messages"));

    //TotalInputMispredictions
    TotalInputMispredictionsText.setFont(MainFont);
    TotalInputMispredictionsText.setCharacterSize(25);
    TotalInputMispredictionsText.setPosition(0.0f, 325.0f);
    TotalInputMispredictionsText.setString(sf::String("Total input mispredictions: 0Mispredictions"));
}
void Application::UpdateNetworkOverlayData() {
    //RTT
    CalculateAverageRTT();
    std::string AverageRTTString = std::to_string(AverageRTT);
    RTTText.setString(sf::String("RTT: " + AverageRTTString + "ms"));


    //UploadRate
    std::string UploadRateString = std::to_string(UploadRate);
    UploadRateText.setString(sf::String("Upload: " + UploadRateString + "Bytes"));

    //DownloadRate
    std::string DownloadRateString = std::to_string(DownloadRate);
    DownloadRateText.setString(sf::String("Download: " + DownloadRateString + "Bytes"));


    //SendingPPS
    std::string SendingPPSString = std::to_string(SendingPPS);
    SendingPPSText.setString(sf::String("Sending: " + SendingPPSString + "PPS"));

    //ReceivingPPS
    std::string ReceivingPPSString = std::to_string(ReceivingPPS);
    ReceivingPPSText.setString(sf::String("Receiving: " + ReceivingPPSString + "PPS"));


    //Total bytes sent
    std::string TotalBytesSentString = std::to_string(TotalBytesSent);
    TotalBytesSentText.setString(sf::String("Total bytes sent: " + TotalBytesSentString + "Bytes"));

    //Total bytes received
    std::string TotalBytesReceivedString = std::to_string(TotalBytesReceived);
    TotalBytesReceivedText.setString(sf::String("Total bytes received: " + TotalBytesReceivedString + "Bytes"));


    //Total packets sent
    std::string TotalPacketsSentString = std::to_string(TotalPacketsSent);
    TotalPacketsSentText.setString(sf::String("Total packets sent: " + TotalPacketsSentString + "Packets"));

    //Total packets received
    std::string TotalPacketsReceivedString = std::to_string(TotalBytesReceived);
    TotalPacketsReceivedText.setString(sf::String("Total packets received:  " + TotalPacketsReceivedString + "Packets"));


    //Total messages sent
    std::string TotalMessagesSentString = std::to_string(TotalMessagesSent);
    TotalMessagesSentText.setString(sf::String("Total messages sent: " + TotalMessagesSentString + "Messages"));

    //Total messages received
    std::string TotalMessagesReceivedString = std::to_string(TotalMessagesReceived);
    TotalMessagesReceivedText.setString(sf::String("Total messages received:  " + TotalMessagesReceivedString + "Messages"));


    //Total input mispredictions
    std::string TotalinputMispredictionsString = std::to_string(MainInputinator.GetMispredictionsAmount());
    TotalInputMispredictionsText.setString(sf::String("Total input mispredictions:  " + TotalinputMispredictionsString + "Mispredictions"));
}
void Application::ClearNetworkOverlayData() {
    UploadRate = 0;
    DownloadRate = 0;
    PPSCalculationRateTimer = 0.0f;
    ReceivingPPSAccumlator = 0;
    ReceivingPPS = 0;
    SendingPPSAccumlator = 0;
    SendingPPS = 0;
    TotalBytesSent = 0;
    TotalBytesReceived = 0;
    TotalPacketsSent = 0;
    TotalPacketsReceived = 0;
    TotalMessagesSent = 0;
    TotalMessagesReceived = 0;
    TotalInputMispredictions = 0;
    MainInputinator.ResetMispredictionsAmount();

    for (uint32 i = 0; i < 10; i++)
        CalculatedRTTs[i] = 0;
}
void Application::UpdatePPSCalculations() {
    PPSCalculationRateTimer += DeltaTime.asSeconds();
    if (PPSCalculationRateTimer >= PPSAccumulationRate) {
        PPSCalculationRateTimer -= PPSAccumulationRate;

        SendingPPS = SendingPPSAccumlator;
        ReceivingPPS = ReceivingPPSAccumlator;
        SendingPPSAccumlator = 0;
        ReceivingPPSAccumlator = 0;
    }
}
void Application::UpdateOverlayData() {
    //Connection Status
    const char* Status = GetCurrentStatusAsText();
    sf::String StatusText = sf::String(Status);
    ConnectionStatusText.setString(sf::String("Status: " + StatusText));

    //InputTick
    std::string CurrentTickString = std::to_string(InputTick);
    CurrentTickText.setString(sf::String("Tick: " + CurrentTickString));
}


void Application::RegisterMovementInput() {
    bool MovedRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    bool MovedLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);

    MovementInputRequestType InputType = MovementInputRequestType::NONE;
    if (MovedRight)
        InputType = MovementInputRequestType::RIGHT;
    else if (MovedLeft)
        InputType = MovementInputRequestType::LEFT;

    MainInputinator.RegisterInputRequest(InputType);
}
void Application::RegisterActionsInput() {
    if (CurrentConnectionStatus != ConnectionStatus::CONNECTED)
        return;

    bool Shoot = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
    if (MainGameMode.IsGameRunning()) {
        if (Shoot) {
            if (!Player1ShootRequest) {
                if (ShootingRateTimer == ShootingRate) {
                    ShootingRateTimer = 0.0f;
                    Player1ShootRequest = true;
                }
            }
        }
    }
}
void Application::UpdateShootingRate() {
    if (ShootingRateTimer < ShootingRate) {
        ShootingRateTimer += DeltaTime.asSeconds();
        if (ShootingRateTimer >= ShootingRate)
            ShootingRateTimer = ShootingRate;
    }
}
void Application::InterpolatePlayer2Position() {
    sf::Vector2f InterpolatedPlayer2Position = MainInterpolator.Interpolate(Player2.getPosition(), Player2TargetPosition);
    Player2.setPosition(InterpolatedPlayer2Position);
}


void Application::ProcessPlayersOrders() {
    if (Player1ShootConfirm) {
        Player1ShootRequest = false;
        Player1ShootConfirm = false;
        MainProjectileManager.SpawnProjectile(Player1.getPosition(), Player1ProjectilesColor, Player1OwnerIndex);
    }
    if (Player2ShootConfirm) {
        Player2ShootConfirm = false;
        MainProjectileManager.SpawnProjectile(Player2.getPosition(), Player2ProjectilesColor, Player2OwnerIndex);
    }
}


inline bool Application::ValidateDefaultPort(uint16 port) const {
    if (port == DefaultServerPort)
        return true;
    else
        return false;
}
inline bool Application::ValidateProtocolVersion(uint32 version) const {
    if (version == ProtocolVersion)
        return true;
    else
        return false;
}
inline bool Application::ValidateProtocolID(uint32 ID) const {
    if (ID == ProtocolID)
        return true;
    else
        return false;
}
inline bool Application::ValidateConnectionPacket(IP_Address address, ConnectionPacket& packet) const {
    if (!ValidateDefaultPort(address.m_port)) {
        PrintMessage("Invalid port number in connection packet received from ", address);
        return false;
    }
    if (!ValidateProtocolVersion(packet.ProtocolVersion.Data)) {
        PrintMessage("Invalid protocol version in connection packet received from ", address);
        return false;
    }
    if (!ValidateProtocolID(packet.ProtocolID.Data)) {
        PrintMessage("Invalid protocol ID in connection packet received from ", address);
        return false;
    }

    return true;
}


inline void Application::PrintMessage(const char* message) const {
    printf("[ %dms ] %s\n", RunningTime.asMilliseconds(), message);
}
inline void Application::PrintMessage(const char* message, uint32 num) const {
    printf("[ %dms ] %s %d\n", RunningTime.asMilliseconds(), message, num);
}
inline void Application::PrintMessage(const char* message, IP_Address address) const {
    printf("[ %dms ] %s - %d.%d.%d.%d:%d\n", RunningTime.asMilliseconds(), message, address.a(), address.b(), address.c(), address.d(), address.m_port);
}
inline void Application::PrintLastCriticalError() const {
    auto Error = Network::get_last_error();
    if (Error.is_critical())
        PrintMessage(Error.as_string());
}
inline void Application::PrintDisconnectionMessage(DisconnectionReason reason) const {
    switch (reason) {
    case DisconnectionReason::UKNOWN: {
        PrintMessage("Connection failed - Reason: Unkown");
        return;
    }
    case DisconnectionReason::INVALID_PROTOCOL_ID: {
        PrintMessage("Connection failed - Reason: Invalid protocol ID");
        return;
    }
    case DisconnectionReason::INVALID_PROTOCOL_VERSION: {
        PrintMessage("Connection failed - Reason: Invalid protocol Version");
        return;
    }
    case DisconnectionReason::SERVER_FULL: {
        PrintMessage("Connection failed - Reason: Server is full");
        return;
    }
    case DisconnectionReason::TIMEOUT: {
        PrintMessage("Client has been disconnected - Reason: Timed out");
        return;
    }
    case DisconnectionReason::INVALID_SESSION_ID: {
        PrintMessage("Client has been disconnected - Reason: Invalid session ID");
        return;
    }
    case DisconnectionReason::USER_CHOICE: {
        PrintMessage("Client has been disconnected - Reason: User choice");
        return;
    }
    case DisconnectionReason::HIGH_LATENCY: {
        PrintMessage("Client has been disconnected - Reason: High latency");
        return;

    }
    }
}
inline const char* Application::GetCurrentStatusAsText() const {
    switch (CurrentConnectionStatus) {
    case ConnectionStatus::DISCONNECTED:
        return "Disconnected";
    case ConnectionStatus::DISCONNECTING:
        return "Disconnecting...";
    case ConnectionStatus::CONNECTED:
        return "Connected";
    case ConnectionStatus::CONNECTING:
        return "Connecting...";
    case ConnectionStatus::SEARCHING:
        return "Searching..";
    }
    return "Error: No matching state found";
}


void Application::OnMouseMove(int x, int y) {

}
void Application::OnKeyPressed(const sf::Keyboard::Key key) {
    if (key == sf::Keyboard::I) {
        if (ShowNetworkOverlay)
            ShowNetworkOverlay = false;
        else
            ShowNetworkOverlay = true;
    }

    if (key == sf::Keyboard::C && CurrentConnectionStatus == ConnectionStatus::DISCONNECTED)
        CurrentConnectionStatus = ConnectionStatus::SEARCHING;
    else if (key == sf::Keyboard::D && CurrentConnectionStatus == ConnectionStatus::CONNECTED)
        Disconnect();

    if (key == sf::Keyboard::R) {
        if (!MainGameMode.IsGameRunning() && CurrentConnectionStatus == ConnectionStatus::CONNECTED)
            ReadyCheckRequest = true;
    }
}
void Application::OnKeyReleased(const sf::Keyboard::Key key) {
}
void Application::OnButtonPressed(const  sf::Mouse::Button button) {
}
void Application::OnButtonReleased(const sf::Mouse::Button button) {
}


void Application::ProcessTick() {
    TickTimer += DeltaTime.asSeconds();
    if (TickTimer >= TimeBetweenEachTick) {
        TickTimer -= TimeBetweenEachTick;

        //Movement is recorded every tick
        RegisterMovementInput();
        MovementInputRequestType InputType = MainInputinator.GetLastMovementInputRequest();
        MainInputinator.PredictMovementInput(Player1.getPosition(), InputType);
        if (InputType != MovementInputRequestType::NONE)
            Player1.setPosition(MainInputinator.GetLastMovementInputPrediction());


        if (CurrentConnectionStatus == ConnectionStatus::CONNECTED && IsPlayer2Connected)
            InterpolatePlayer2Position();

        InputTick++;
    }
}
bool Application::Update() {
    DeltaTime = Clock.restart();
    RunningTime += DeltaTime;

    ProcessTick();

    Receive();

    MainCollisionManager.Update();
    MainProjectileManager.Update(DeltaTime);
    MainEnemiesManager.Update(DeltaTime);

    if (CurrentConnectionStatus == ConnectionStatus::CONNECTED) {
        RegisterActionsInput();
        ProcessPlayersOrders();
        UpdateShootingRate();

        UpdateTimeoutCheck();
        UpdatePayloadTransmission();
        UpdatePPSCalculations();
    }
    else if (CurrentConnectionStatus == ConnectionStatus::SEARCHING) {
        UpdateServerSearch();
        UpdateServerSearchDuration();
    }

    if (ShowNetworkOverlay)
        UpdateNetworkOverlayData();

    UpdateOverlayData();


    return Running;
}
void Application::Render() {
    Window.clear(sf::Color{ 0x44, 0x55, 0x66, 0xff });

    Window.draw(ConnectionStatusText);
    Window.draw(CurrentTickText);

    if (CurrentConnectionStatus == ConnectionStatus::CONNECTED) {
        MainProjectileManager.Render(Window);
        MainEnemiesManager.Render(Window);
        MainGameMode.Render(Window);
        Window.draw(Player1);
        if (IsPlayer2Connected)
            Window.draw(Player2);
    }

    if (ShowNetworkOverlay) {
        Window.draw(RTTText);
        Window.draw(UploadRateText);
        Window.draw(DownloadRateText);
        Window.draw(SendingPPSText);
        Window.draw(ReceivingPPSText);
        Window.draw(TotalBytesSentText);
        Window.draw(TotalBytesReceivedText);
        Window.draw(TotalPacketsSentText);
        Window.draw(TotalPacketsReceivedText);
        Window.draw(TotalMessagesSentText);
        Window.draw(TotalMessagesReceivedText);
        Window.draw(TotalInputMispredictionsText);
    }

    Window.display();
}


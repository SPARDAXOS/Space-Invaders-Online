#pragma once
#include "Batch.hpp"
#include "Network.hpp"
#include "Protocol.hpp"
#include "Utility.hpp"
#include "Interpolator.hpp"
#include "ProjectilesManager.hpp"
#include "EnemiesManager.hpp"
#include "GameMode.hpp"
#include "CollisionManager.hpp"
#include "Inputinator.hpp"
#include <math.h>


class Application final {
public:
    Application() = default;

    void Run();

private:
    bool Start();
    void End();

private:
    void ProcessTick();
    bool Update();
    void Render();

private:
    void Connect();
    void Disconnect();
    void Timeout();

private:
    bool Receive();
    bool Send(ConnectionPacket packet);
    bool Send(DisconnectionPacket packet);
    bool Send(PayloadPacket packet);

private:
    ConnectionPacket    CreateConnectionPacket();
    DisconnectionPacket CreateDisconnectionPacket(DisconnectionReason reason);
    PayloadPacket       CreatePayloadPacket();

private:
    IP_Address FindOwnIPAddress();
    void       UpdateServerSearch();
    void       UpdateServerSearchDuration();

private:
    void        UpdatePayloadTransmission();
    void        UpdateTimeoutCheck();
    inline void ConfirmConnection();

private:
    inline void SetupPlayer1();
    inline void SetupPlayer2();

private:
    inline void IncrementPayloadSequenceNumber();
    inline void SetLastReceivedPayloadSequence(uint16 num);
    inline void RegisterSentPayloadPacket(uint16 sequenceNumber);
    bool        WasPayloadPacketSent(uint16 sequenceNumber, uint32& timestamp);

private:
    inline void AddRTTCalculation(uint32 rtt);
    inline void CalculateAverageRTT();
    void ResetConnectionData();

private:
    void SetupNetworkOverlayData();
    void UpdateNetworkOverlayData();
    void ClearNetworkOverlayData();
    void UpdatePPSCalculations();
    void UpdateOverlayData();

private:
    void RegisterMovementInput();
    void RegisterActionsInput();
    void UpdateShootingRate();
    void InterpolatePlayer2Position();

private:
    void ProcessPlayersOrders();

private:
    inline bool ValidateDefaultPort(uint16 port) const;
    inline bool ValidateProtocolVersion(uint32 version) const;
    inline bool ValidateProtocolID(uint32 ID) const;
    inline bool ValidateConnectionPacket(IP_Address address, ConnectionPacket& packet) const;

private:
    inline void        PrintMessage(const char* message) const;
    inline void        PrintMessage(const char* message, uint32 num) const;
    inline void        PrintMessage(const char* message, IP_Address address) const;
    inline void        PrintLastCriticalError() const;
    inline void        PrintDisconnectionMessage(DisconnectionReason reason) const;
    inline const char* GetCurrentStatusAsText() const;

private:
    void OnMouseMove(int x, int y);
    void OnKeyPressed(const sf::Keyboard::Key key);
    void OnKeyReleased(const sf::Keyboard::Key key);
    void OnButtonPressed(const sf::Mouse::Button button);
    void OnButtonReleased(const sf::Mouse::Button button);


private:
    uint32 AverageRTT;
    uint32 UploadRate;
    uint32 DownloadRate;
    uint32 SendingPPS;
    uint32 ReceivingPPS;
    uint32 SendingPPSAccumlator;
    uint32 ReceivingPPSAccumlator;
    uint32 TotalBytesSent;
    uint32 TotalBytesReceived;
    uint32 TotalPacketsSent;
    uint32 TotalPacketsReceived;
    uint32 TotalMessagesSent;
    uint32 TotalMessagesReceived;
    uint32 TotalInputMispredictions;

private:
    float PayloadTransmissionRate{ 0.1f };
    float TimeoutThreshold       { 5.0f };
    float PPSAccumulationRate    { 1.0f };
    float ServerSearchDuration   { 4.0f };
    float ServerSearchPPSRate    { 0.5f };
    float ShootingRate           { 0.3f };

private:
    float PayloadTransmissionRateTimer;
    float TimeoutThresholdTimer;
    float PPSCalculationRateTimer;
    float ServerSearchDurationTimer;
    float ServerSearchPPSRateTimer;
    float ShootingRateTimer;

private:
    bool IsPlayer2Connected;
    bool Player1ShootRequest;
    bool ReadyCheckRequest;
    bool Player1ShootConfirm;
    bool Player2ShootConfirm;

private:
    uint16   SentPayloadsHistory[100];
    uint32   SentPayloadsTimestampsHistory[100];
    uint32   SentPayloadsIndex;
    uint16   CurrentPayloadSequenceNumber;
    uint16   LastReceivedPayloadSequence;
    sf::Time LastReceivedPayloadTime;

private:
    uint32 CalculatedRTTs[10];
    uint32 CalculatedRTTsIndex;

private:
    sf::Font MainFont;
    sf::Text ConnectionStatusText;
    sf::Text RTTText;
    sf::Text CurrentTickText;
    sf::Text UploadRateText;
    sf::Text DownloadRateText;
    sf::Text SendingPPSText;
    sf::Text ReceivingPPSText;
    sf::Text TotalBytesSentText;
    sf::Text TotalBytesReceivedText;
    sf::Text TotalPacketsSentText;
    sf::Text TotalPacketsReceivedText;
    sf::Text TotalMessagesSentText;
    sf::Text TotalMessagesReceivedText;
    sf::Text TotalInputMispredictionsText;

private:
    sf::RectangleShape Player1;
    sf::RectangleShape Player2;
    sf::Vector2f       Player2TargetPosition;
    const uint32       Player1OwnerIndex{ 0x21032012 };
    const uint32       Player2OwnerIndex{ 0x31232415 };
    const sf::Color    Player1ProjectilesColor{ sf::Color::Blue };
    const sf::Color    Player2ProjectilesColor{ sf::Color::Red };

private:
    uint32 InputTick;
    float  TimeBetweenEachTick;
    float  TickRate{ 60.0f };
    float  TickTimer;

private:
    sf::RenderWindow Window;
    sf::Clock        Clock;
    sf::Time         RunningTime;
    sf::Time         DeltaTime;

private:
    Network    Network;
    UDP_Socket Socket;

private:
    IP_Address OwnAddress;
    IP_Address BroadcastAddress;
    IP_Address ConnectionAddress;

private:
    GameMode          MainGameMode;
    Interpolator      MainInterpolator;
    Inputinator       MainInputinator;
    CollisionManager  MainCollisionManager;
    ProjectilesManager MainProjectileManager;
    EnemiesManager    MainEnemiesManager;

private:
    ConnectionStatus CurrentConnectionStatus;
    bool             Running;
    bool             ShowNetworkOverlay{ false };
    bool             ShowPacketsLog{ false };
    uint16           SessionID{ 0x00000000 }; //default invalid
};


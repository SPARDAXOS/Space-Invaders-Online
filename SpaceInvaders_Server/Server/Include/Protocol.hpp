#pragma once
#include "Network.hpp"


enum class PacketType {
    CONNECTION = 0,
    DISCONNECTION,
    PAYLOAD,
};

class Packet {
public:
    Packet() = default;
    Packet(uint8 type)
        : Type(type)
    {
    }
    Packet(uint8 type, uint16 session_ID)
        : Type(type),
          SessionID(session_ID)
    {
    }

    virtual inline bool Read(BitStreamReader& stream)
    {
        bool Success = true;
        Success &= stream.DecryptData();
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);
        return Success;
    }
    virtual inline bool Write(BitStreamWriter& stream)
    {
        bool Success = true;
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);
        Success &= stream.FlushRemainingBits();
        return Success;
    }

    B2 Type{ 0 };
    B16 SessionID{ 0 };
};

class ConnectionPacket final : public Packet {
public: 
    ConnectionPacket() 
        : Packet(uint8(::PacketType::CONNECTION))
    {
    }
    ConnectionPacket(uint32 protocolID, uint32 protocolVersion)
        : Packet(uint8(::PacketType::CONNECTION)),
        ProtocolID(protocolID),
        ProtocolVersion(protocolVersion)
    {
    }
    ConnectionPacket(uint32 protocolID, uint32 protocolVersion, uint16 sessionID)
        : Packet(uint8(::PacketType::CONNECTION), sessionID),
          ProtocolID(protocolID),
          ProtocolVersion(protocolVersion)
    {
    }

    bool Read(BitStreamReader& stream) {
        bool Success = true;
        Success &= stream.DecryptData();
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);
        Success &= stream.Serialize(ProtocolID);
        Success &= stream.Serialize(ProtocolVersion);
        return Success;
    }
    bool Write(BitStreamWriter& stream) {
        bool Success = true;
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);
        Success &= stream.Serialize(ProtocolID);
        Success &= stream.Serialize(ProtocolVersion);
        Success &= stream.FlushRemainingBits();
        return Success;
    }

    //Unique message data
    B32 ProtocolID{ 0 };
    B32 ProtocolVersion{ 0 };
};

struct DisconnectionPacket final : public Packet {
    DisconnectionPacket()
        : Packet(uint8(::PacketType::DISCONNECTION))
    {
    }
    DisconnectionPacket(uint16 sessionID)
        : Packet(uint8(::PacketType::DISCONNECTION), sessionID)
    {
    }
    DisconnectionPacket(uint16 sessionID, uint8 reason)
        : Packet(uint8(::PacketType::DISCONNECTION), sessionID),
          Reason(reason)
    {
    }

    bool Read(BitStreamReader& stream) {
        bool Success = true;
        Success &= stream.DecryptData();
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);
        Success &= stream.Serialize(Reason);

        return Success;
    }
    bool Write(BitStreamWriter& stream) {
        bool Success = true;
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);
        Success &= stream.Serialize(Reason);
        Success &= stream.FlushRemainingBits();

        return Success;
    }

    //Unique message data
    B3 Reason{ 0 };
};

struct PayloadPacket final : public Packet {
    PayloadPacket()
        : Packet(uint8(::PacketType::PAYLOAD))
    {
    }
    PayloadPacket(uint16 sessionID)
        : Packet(uint8(::PacketType::PAYLOAD), sessionID)
    {
    }
    PayloadPacket(uint16 sessionID, uint32 processingTime) 
        : Packet(uint8(::PacketType::PAYLOAD), sessionID),
          ProcessingTime(processingTime)
    {
    }

    bool Read(BitStreamReader& stream) {
        bool Success = true;
        Success &= stream.DecryptData();

        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);

        Success &= stream.Serialize(ProcessingTime);
        Success &= stream.Serialize(SequenceNumber);
        Success &= stream.Serialize(AcknowledgeNumber);

        Success &= stream.Serialize(InputTick);

        Success &= stream.Serialize(Player1PositionX);
        Success &= stream.Serialize(Player1PositionY);
        Success &= stream.Serialize(Player2PositionX);
        Success &= stream.Serialize(Player2PositionY);

        Success &= stream.Serialize(ActiveEnemiesBits);
        //Cheap ass solution for now
        for (uint32 i = 0; i < 25; i++)
            Success &= stream.Serialize(EnemiesXPositions[i]);
        for (uint32 i = 0; i < 25; i++)
            Success &= stream.Serialize(EnemiesYPositions[i]);

        Success &= stream.Serialize(Player1Score);
        Success &= stream.Serialize(Player2Score);

        Success &= stream.Serialize(InputRequest);
        Success &= stream.Serialize(Player1ActionShoot);
        Success &= stream.Serialize(Player2ActionShoot);

        Success &= stream.Serialize(IsPlayer2Connected);

        Success &= stream.Serialize(Player1ReadyCheck);
        Success &= stream.Serialize(Player2ReadyCheck);

        Success &= stream.Serialize(StartGame);
        Success &= stream.Serialize(GameResults);

        return Success;
    }
    bool Write(BitStreamWriter& stream) {
        bool Success = true;
        Success &= stream.Serialize(Type);
        Success &= stream.Serialize(SessionID);

        Success &= stream.Serialize(ProcessingTime);
        Success &= stream.Serialize(SequenceNumber);
        Success &= stream.Serialize(AcknowledgeNumber);

        Success &= stream.Serialize(InputTick);

        Success &= stream.Serialize(Player1PositionX);
        Success &= stream.Serialize(Player1PositionY);
        Success &= stream.Serialize(Player2PositionX);
        Success &= stream.Serialize(Player2PositionY);

        Success &= stream.Serialize(ActiveEnemiesBits);

        //Cheap ass solution for now
        for(uint32 i = 0; i < 25; i++)
            Success &= stream.Serialize(EnemiesXPositions[i]);
        for (uint32 i = 0; i < 25; i++)
            Success &= stream.Serialize(EnemiesYPositions[i]);

        Success &= stream.Serialize(Player1Score);
        Success &= stream.Serialize(Player2Score);

        Success &= stream.Serialize(InputRequest);
        Success &= stream.Serialize(Player1ActionShoot);
        Success &= stream.Serialize(Player2ActionShoot);

        Success &= stream.Serialize(IsPlayer2Connected);

        Success &= stream.Serialize(Player1ReadyCheck);
        Success &= stream.Serialize(Player2ReadyCheck);

        Success &= stream.Serialize(StartGame);
        Success &= stream.Serialize(GameResults);
   
        Success &= stream.FlushRemainingBits();

        return Success;
    }

    //Unique message data
    B32 ProcessingTime{ 0 };
    B16 SequenceNumber{ 0 }; 
    B16 AcknowledgeNumber{ 0 };

    B16 InputTick{ 0 };

    B10 Player1PositionX{ 0 };
    B10 Player1PositionY{ 0 };
    B10 Player2PositionX{ 0 };
    B10 Player2PositionY{ 0 };

    B25 ActiveEnemiesBits{ 0 };
    B10 EnemiesXPositions[25]{ 0 };
    B10 EnemiesYPositions[25]{ 0 };

    B8 Player1Score{ 0 };
    B8 Player2Score{ 0 };

    B2 InputRequest{ 0 };
    B1 Player1ActionShoot{ 0 };
    B1 Player2ActionShoot{ 0 };

    B1 IsPlayer2Connected{ 0 };

    B1 Player1ReadyCheck{ 0 };
    B1 Player2ReadyCheck{ 0 };

    B1 StartGame{ 0 };
    B2 GameResults{ 0 };
};

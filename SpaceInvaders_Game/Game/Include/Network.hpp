#pragma once
#include <type_traits> 
#include <cstring>     
#include <vector>
#include <cassert>

using uint64 = unsigned long long;
using  int64 = signed long long;
using uint32 = unsigned int;
using  int32 = signed int;
using uint16 = unsigned short;
using  int16 = signed short;
using  uint8 = unsigned char;
using   int8 = signed char;



static constexpr uint16 DefaultServerPort = 10101;
static constexpr uint64 DecryptionKey = 0x1234567891011121;
static constexpr uint32 ProtocolID = 0x63123906;
static constexpr uint32 ProtocolVersion = 0x00000001;


enum class ConnectionStatus {
    DISCONNECTED = 0,
    DISCONNECTING,
    CONNECTED,
    CONNECTING,
    SEARCHING
};
enum class DisconnectionReason {
    UKNOWN = 0,
    INVALID_PROTOCOL_ID,
    INVALID_PROTOCOL_VERSION,
    SERVER_FULL,
    TIMEOUT,
    INVALID_SESSION_ID,
    USER_CHOICE,
    HIGH_LATENCY
};
enum class MovementInputRequestType {
    NONE = 0,
    LEFT,
    RIGHT,
};


struct BitDataType {
    BitDataType() = default;
    BitDataType(uint8 bitsAmount)
        : BitsAmount(bitsAmount)
    {
    }
    BitDataType(uint8 bitsAmount, uint32 data)
        : BitsAmount(bitsAmount),
        Data(data)
    {
    }

    const uint8 BitsAmount{ 0 };
    uint32 Data{ 0 };
};

struct B1 : public BitDataType {
    B1()
        : BitDataType(1)
    {
    }
    B1(uint32 data)
        : BitDataType(1, data)
    {
    }
};
struct B2 : public BitDataType {
    B2()
        : BitDataType(2)
    {
    }
    B2(uint32 data)
        : BitDataType(2, data)
    {
    }
};
struct B3 : public BitDataType {
    B3()
        : BitDataType(3)
    {
    }
    B3(uint32 data)
        : BitDataType(3, data)
    {
    }
};
struct B5 : public BitDataType {
    B5()
        : BitDataType(5)
    {
    }
    B5(uint32 data)
        : BitDataType(5, data)
    {
    }
};
struct B8 : public BitDataType {
    B8()
        : BitDataType(8)
    {
    }
    B8(uint32 data)
        : BitDataType(8, data)
    {
    }
};
struct B10 : public BitDataType {
    B10()
        : BitDataType(10)
    {
    }
    B10(uint32 data)
        : BitDataType(10, data)
    {
    }
};
struct B16 : public BitDataType {
    B16()
        : BitDataType(16)
    {
    }
    B16(uint32 data)
        : BitDataType(16, data)
    {
    }
};
struct B25 : public BitDataType {
    B25()
        : BitDataType(25)
    {
    }
    B25(uint32 data)
        : BitDataType(25, data)
    {
    }
};
struct B32 : public BitDataType {
    B32()
        : BitDataType(32)
    {
    }
    B32(uint32 data)
        : BitDataType(32, data)
    {
    }
};


struct BitStream {
    BitStream() = default;
    bool ApplyDecryptionKey() {
        uint8 TargetDecryptionByte = 0;

        uint16 TargetDecryptionByteCount = 0;
        uint16 TargetDataByteCount = 0;

        for (unsigned int i = 0; i < unsigned int(Size); i++) {
            TargetDecryptionByte = uint8(DecryptionKey >> (8 * TargetDecryptionByteCount));

            Buffer[TargetDataByteCount] ^= TargetDecryptionByte;

            TargetDataByteCount++;
            TargetDecryptionByteCount = uint64(TargetDecryptionByteCount + 1) % sizeof(DecryptionKey);
        }

        return true;
    }

    int   Size{ 0 };
    uint8 Buffer[1024] = {}; // 1024 x 8 = 8192 bits - MUST be divisible by 4.
};
struct BitStreamWriter {
    BitStreamWriter(BitStream& stream)
        : Stream(stream)
        , StreamSizeLimit(sizeof(stream.Buffer))
    {
    }

    bool Serialize(BitDataType& value) {
        float StreamSizeDivided = (float)(StreamSizeLimit / 4.0f);
        float StreamSizeRounded = (float)(ceil(StreamSizeDivided));
        assert((StreamSizeDivided == StreamSizeRounded && "Bitstream buffer size must be divisible by 4"));

        //Check if Stream is full
        if (BytesCount >= StreamSizeLimit) {
            printf("Error: Unable to write data due to bitstream being full\n");
            return false;
        }

        //Writing
        Scratch |= (uint64)value.Data << ScratchBits;
        ScratchBits += value.BitsAmount;

        //Check if 32bits limit of Scratch is reached
        if (ScratchBits >= 32) {
            for (unsigned int i = 0; i < 4; i++) { // 4 bytes flush
                Stream.Buffer[BytesCount] = (char)ScratchPointer[0];
                Scratch >>= 8;
                ScratchBits -= 8;
                Stream.Size++;
                BytesCount++;
            }
        }

        return true;
    }
    bool FlushRemainingBits() { //Should be called at the end of writing
        if (ScratchBits > 0) {
            uint32 BytesToFit = (uint32)ceil(ScratchBits / 8.0f);
            if (BytesCount + BytesToFit >= StreamSizeLimit) {
                printf("Error: Unable to wrap up due to bitstream size limit being reached\n");
                return false;
            }
            for (unsigned int i = 0; i < BytesToFit; i++) {
                Stream.Buffer[BytesCount] = (char)ScratchPointer[0];
                Scratch >>= 8;
                Stream.Size++;
                BytesCount++;
            }
            ScratchBits = 0;
            if (!Stream.ApplyDecryptionKey()) {
                printf("Data decryption failed while flushing remaining bits");
                return false;
            }
        }

        return true;
    }

    BitStream& Stream;
    uint32     StreamSizeLimit{ 0 };

    uint64     Scratch{ 0 };
    uint32     ScratchBits{ 0 };
    uint32     BytesCount{ 0 };
    char*      ScratchPointer = (char*)&Scratch;
};
struct BitStreamReader {
    BitStreamReader(BitStream& stream)
        : Stream(stream)
        , StreamSizeLimit(sizeof(stream.Buffer))
    {
    }

    inline uint8 Peek() const {
        uint64 TypeBitMask = 0b00000011;  //Should be set to the BitDataType's BitAmount used for the PacketType.

        uint8 Data = uint8(Stream.Buffer[0] & TypeBitMask);
        uint64 DecryptionKeyMask = DecryptionKey & TypeBitMask;

        return uint8(Data ^ DecryptionKeyMask);
    }
    inline bool HasData() const {
        return BytesCount < StreamSizeLimit;
    }
    inline bool DecryptData() {
        return Stream.ApplyDecryptionKey();
    }

    bool Serialize(BitDataType& value) {
        //Check if all data has been read
        float StreamSizeDivided = (float)(StreamSizeLimit / 4.0f);
        float StreamSizeRounded = (float)(ceil(StreamSizeDivided));
        assert((StreamSizeDivided == StreamSizeRounded && "Bitstream buffer size must be divisible by 4"));

        //Load 4 bytes more into the scratch
        if (value.BitsAmount > ScratchBits) {
            uint64 Bytes = 0;
            uint64* BytesPointer = &Bytes;

            for (unsigned int i = 0; i < 4; i++) {
                if (BytesCount >= StreamSizeLimit) { //If this is bypassed then it means THERE IS 4 more bytes to read.
                    printf("Error: Unable to load next 32 bits due to bitstream being empty\n");
                    return false;
                }
                BytesPointer[0] = uint64(Stream.Buffer[BytesCount]);
                Bytes <<= ScratchBits;
                Scratch |= Bytes;
                Bytes = 0;
                BytesCount++;
                ScratchBits += 8;
            }
        }

        //Create bitmask
        uint64 Mask = 0;
        uint64 MaskedBit = 1;
        for (uint64 i = 0; i < value.BitsAmount; i++) {
            Mask |= MaskedBit;
            MaskedBit <<= 1;
        }

        //Read data
        value.Data = uint32(Scratch & Mask);
        Scratch >>= value.BitsAmount;
        ScratchBits -= value.BitsAmount;

        return true;
    }

    BitStream& Stream;
    uint32     StreamSizeLimit{ 0 };

    uint64     Scratch{ 0 };
    uint32     ScratchBits{ 0 };
    uint32     BytesCount{ 0 };
    char*      ScratchPointer = (char*)&Scratch;
};


struct ByteStream {
    ByteStream() = default;

    int  Size{ 0 };
    char Buffer[1024] = {};
};
struct ByteStreamWriter {
    ByteStreamWriter(ByteStream& stream)
        : Stream(stream)
        , Cursor(stream.Buffer + stream.Size)
        , End(stream.Buffer + sizeof(stream.Buffer))
    {
    }

    template <typename T>
    bool Serialize(T value) {
        static_assert(std::is_fundamental_v<T>, "T needs to be a fundamental datatype!");
        if ((Cursor + sizeof(T)) >= End) {
            return false;
        }

        std::memcpy(Cursor, &value, sizeof(T));
        Cursor += sizeof(T);
        Stream.Size = int32_t(Cursor - Stream.Buffer);
        return true;
    }

    ByteStream& Stream;
    char* Cursor{ nullptr };
    char* End{ nullptr };
};
struct ByteStreamReader {
    ByteStreamReader(ByteStream& stream)
        : Stream(stream)
        , Cursor(stream.Buffer)
        , End(stream.Buffer + stream.Size)
    {
    }

    inline uint8 Peek() const {
        return Cursor[0];
    }

    inline bool has_data() const {
        return Cursor < End;
    }

    template <typename T>
    bool Serialize(T& value) {
        static_assert(std::is_fundamental_v<T>, "T needs to be a fundamental datatype!");
        if ((Cursor + sizeof(T)) > End) {
            return false;
        }

        std::memcpy(&value, Cursor, sizeof(T));
        Cursor += sizeof(T);
        return true;
    }

    ByteStream& Stream;
    char* Cursor{ nullptr };
    char* End{ nullptr };
};


struct Network
{
    struct Error
    {
        Error() = default;
        Error(int code);
        Error(const Error& rhs);

        bool  is_critical() const;
        int32 as_code() const;
        const char* as_string() const;

        const int32 m_code{ 0 };
    };

    static Error get_last_error();

    Network();
    ~Network();
};
struct IP_Address
{
    static bool GetLocalAddresses(std::vector<IP_Address>& addresses);

    static constexpr uint32 k_any_host = 0;
    static constexpr uint16 k_any_port = 0;

    IP_Address() = default;
    IP_Address(const IP_Address& rhs);
    IP_Address(const char* name, uint16 port = k_any_port);
    IP_Address(const uint32 host, uint16 port = k_any_port);
    IP_Address(uint8 a, uint8 b, uint8 c, uint8 d, uint16 port = k_any_port);

    bool operator==(const IP_Address& rhs) const;
    bool operator!=(const IP_Address& rhs) const;

    uint8 a() const;
    uint8 b() const;
    uint8 c() const;
    uint8 d() const;

    uint32 m_host{ k_any_host };
    uint16 m_port{ k_any_port };
};
struct UDP_Socket
{
    UDP_Socket();

    bool Valid() const;
    bool Open(const IP_Address& address);
    void Close();

    bool Send(const IP_Address& address, const BitStream& stream);
    bool Receive(IP_Address& address, BitStream& stream);

    bool GetAddress(IP_Address& address);

    uint64 Handle;
};
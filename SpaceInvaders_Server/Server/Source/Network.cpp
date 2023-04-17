#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#include "Network.hpp"


Network::Error::Error(int code)
    : m_code(code)
{
}

Network::Error::Error(const Error& rhs)
    : m_code(rhs.m_code)
{
}

bool Network::Error::is_critical() const
{
    if (m_code == 0 || m_code == WSAEWOULDBLOCK || m_code == WSAECONNRESET) {
        return false;
    }

    return true;
}

int32 Network::Error::as_code() const
{
    return m_code;
}

const char* Network::Error::as_string() const
{
    switch (m_code) {
    case WSAEADDRINUSE:
        return "Address already in use.";
    case WSAECONNRESET:
        return "Connection reset by peer.";
    case WSAEWOULDBLOCK:
        return "Resource temporarily unavailable.";
    case WSANOTINITIALISED:
        return "Successful WSAStartup not yet performed.";
    }

    return "Unknown socket error!";
}

//static 
Network::Error Network::get_last_error()
{
    return Network::Error{ WSAGetLastError() };
}

Network::Network()
{
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return;
    }
}

Network::~Network()
{
    WSACleanup();
}

// static 
bool IP_Address::get_local_addresses(std::vector<IP_Address>& addresses)
{
    DWORD size = 0;
    GetAdaptersAddresses(AF_INET,
        GAA_FLAG_INCLUDE_PREFIX,
        NULL,
        NULL,
        &size);

    IP_ADAPTER_ADDRESSES* adapter_addresses = (IP_ADAPTER_ADDRESSES*)calloc(1, size);
    GetAdaptersAddresses(AF_INET,
        GAA_FLAG_INCLUDE_PREFIX,
        NULL,
        adapter_addresses,
        &size);

    for (IP_ADAPTER_ADDRESSES* iter = adapter_addresses; iter != NULL; iter = iter->Next) {
        if (iter->OperStatus == IfOperStatusUp && (iter->IfType == IF_TYPE_ETHERNET_CSMACD ||
            iter->IfType == IF_TYPE_IEEE80211))
        {
            for (IP_ADAPTER_UNICAST_ADDRESS* ua = iter->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                char addrstr[1024] = {};
                getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, addrstr, sizeof(addrstr), NULL, 0, NI_NUMERICHOST);

                if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in ai = *(sockaddr_in*)ua->Address.lpSockaddr;
                    IP_Address address;
                    address.m_host = ntohl(ai.sin_addr.s_addr);
                    address.m_port = ntohs(ai.sin_port);
                    addresses.push_back(address);
                }
            }
        }
    }

    free(adapter_addresses);

    return !addresses.empty();
}

static uint32 dns_query(const char* name)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result{ nullptr };
    int res = getaddrinfo(name, nullptr, &hints, &result);
    if (res == WSAHOST_NOT_FOUND) {
        return 0;
    }

    uint32 host = 0;
    for (addrinfo* it = result; it != nullptr; it = it->ai_next) {
        if (it->ai_family == AF_INET) {
            sockaddr_in& addr = *(sockaddr_in*)it->ai_addr;
            host = ntohl(addr.sin_addr.s_addr);
            break;
        }
    }

    freeaddrinfo(result);

    return host;
}

IP_Address::IP_Address(const IP_Address& rhs)
    : m_host(rhs.m_host)
    , m_port(rhs.m_port)
{
}

IP_Address::IP_Address(const char* name, uint16 port)
    : m_host(dns_query(name))
    , m_port(port)
{
}

IP_Address::IP_Address(const uint32 host, uint16 port)
    : m_host(host)
    , m_port(port)
{
}

IP_Address::IP_Address(uint8 a, uint8 b, uint8 c, uint8 d, uint16 port)
    : m_host(k_any_host)
    , m_port(port)
{
    m_host = uint32(a) << 24;
    m_host |= uint32(b) << 16;
    m_host |= uint32(c) << 8;
    m_host |= uint32(d);
}

bool IP_Address::operator==(const IP_Address& rhs) const
{
    return m_host == rhs.m_host && m_port == rhs.m_port;
}

bool IP_Address::operator!=(const IP_Address& rhs) const
{
    return m_host != rhs.m_host || m_port != rhs.m_port;
}

uint8 IP_Address::a() const
{
    return uint8(m_host >> 24);
}

uint8 IP_Address::b() const
{
    return uint8(m_host >> 16);
}

uint8 IP_Address::c() const
{
    return uint8(m_host >> 8);
}

uint8 IP_Address::d() const
{
    return uint8(m_host & 0xff);
}
UDP_Socket::UDP_Socket()
    : Handle(INVALID_SOCKET)
{
}

bool UDP_Socket::Valid() const
{
    return Handle != INVALID_SOCKET;
}

bool UDP_Socket::Open(const IP_Address& address)
{
    SOCKET fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.m_port);
    addr.sin_addr.s_addr = htonl(address.m_host);
    if (::bind(fd, (const sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(fd);
        return false;
    }

    u_long non_blocking = 1;
    if (ioctlsocket(fd, FIONBIO, &non_blocking) == SOCKET_ERROR) {
        closesocket(fd);
        return false;
    }

    Handle = fd;

    return Valid();
}

void UDP_Socket::Close()
{
    if (Valid()) {
        closesocket(Handle);
    }

    Handle = INVALID_SOCKET;
}

bool UDP_Socket::Send(const IP_Address& address, const BitStream& stream)
{
    if (!Valid()) {
        return false;
    }

    const int32 length = stream.Size;
    const char* data = (const char*)stream.Buffer;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.m_port);
    addr.sin_addr.s_addr = htonl(address.m_host);
    if (::sendto(Handle, data, length, 0, (const sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        return false;
    }

    return true;
}

bool UDP_Socket::Receive(IP_Address& address, BitStream& stream)
{
    if (!Valid()) {
        return false;
    }

    sockaddr_in addr{};
    int32 addrlen = sizeof(addr);
    int32 res = ::recvfrom(Handle, (char*)stream.Buffer, sizeof(stream.Buffer), 0, (sockaddr*)&addr, &addrlen);
    if (res == SOCKET_ERROR) {
        return false;
    }

    address.m_host = ntohl(addr.sin_addr.s_addr);
    address.m_port = ntohs(addr.sin_port);
    stream.Size = res;

    return true;
}

bool UDP_Socket::GetAddress(IP_Address& address)
{
    if (!Valid()) {
        return false;
    }

    sockaddr_in addr{};
    int addrlen = sizeof(addr);
    if (getsockname(Handle, (sockaddr*)&addr, &addrlen) == SOCKET_ERROR) {
        return false;
    }

    address.m_host = ntohl(addr.sin_addr.s_addr);
    address.m_port = ntohs(addr.sin_port);

    return true;
}
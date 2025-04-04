
#include "./socket.h"

namespace eokas {
    
    /*
    ===========================================================
    == Protocol, Address, ByteOrder etc.
    ===========================================================
    */
    SocketAddress::SocketAddress() {
        memset(&value, 0, sizeof(value));
    }
    
    SocketAddress::SocketAddress(const sockaddr& addr)
        : value(addr) {
    }
    
    SocketAddress& SocketAddress::operator=(const sockaddr& addr) {
        this->value = addr;
        return *this;
    }
    
    SocketAddress::operator sockaddr() const {
        return value;
    }
    
    bool SocketAddress::operator==(const SocketAddress& addr) {
        return memcmp(&this->value, &addr.value, sizeof(this->value)) == 0;
    }
    
    bool SocketAddress::operator!=(const SocketAddress& addr) {
        return memcmp(&this->value, &addr.value, sizeof(this->value)) != 0;
    }
    
    InetAddress::InetAddress()
        : SocketAddress() {
    }
    
    InetAddress::InetAddress(const sockaddr_in& addr)
        : SocketAddress((const sockaddr&) addr) {
    }
    
    InetAddress::InetAddress(const char* ip, u16_t port) {
        sockaddr_in* sin = (sockaddr_in*) (&value);
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = inet_addr(ip);
        sin->sin_port = htons(port);
    }
    
    InetAddress& InetAddress::operator=(const sockaddr_in& addr) {
        value = *(sockaddr*) &addr;
        return *this;
    }
    
    InetAddress::operator sockaddr_in() const {
        return *(sockaddr_in*) (&value);
    }
    
    const char* InetAddress::ip() const {
        sockaddr_in* sin = ((sockaddr_in*) (&value));
        return inet_ntoa(sin->sin_addr);
    }
    
    u16_t InetAddress::port() const {
        sockaddr_in* sin = ((sockaddr_in*) (&value));
        return ntohs(sin->sin_port);
    }
    
    u16_t ByteOrder::h2nI16(u16_t value) {
        return htons(value);
    }
    
    u32_t ByteOrder::h2nI32(u32_t value) {
        return htonl(value);
    }
    
    u32_t ByteOrder::h2nF32(f32_t value) {
        u32_t temp = *(u32_t*) &value;
        return htonl(temp);
    }
    
    u16_t ByteOrder::n2hI16(u16_t value) {
        return ntohs(value);
    }
    
    u32_t ByteOrder::n2hI32(u32_t value) {
        return ntohl(value);
    }
    
    f32_t ByteOrder::n2hF32(u32_t value) {
        u32_t temp = ntohl(value);
        return *(f32_t*) &temp;
    }
    
    /*
    ===========================================================
    == Socket
    ===========================================================
    */
    Socket::Socket()
        : mHandle(_null_socket) {
    }
    
    Socket::Socket(const socket_t& handle)
        : mHandle(handle) {
    }
    
    Socket::~Socket() {
    }
    
    Socket::operator socket_t() const {
        return mHandle;
    }
    
    bool Socket::operator==(const Socket& other) const {
        return this->mHandle == other.mHandle;
    }
    
    bool Socket::operator!=(const Socket& other) const {
        return this->mHandle != other.mHandle;
    }
    
    bool Socket::open() {
        return this->open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    
    bool Socket::open(AddressFamily family, SocketType socktype, ProtocolType protocol) {
        mHandle = ::socket(family, socktype, protocol);
        return mHandle != _null_socket;
    }
    
    void Socket::close() {
        if (mHandle == _null_socket)
            return;
        ::_closesocket(mHandle);
        mHandle = _null_socket;
    }
    
    void Socket::shutdown(ShutdownMethod how) {
        ::shutdown(mHandle, (int) how);
    }
    
    bool Socket::isOpen() const {
        return mHandle != _null_socket;
    }
    
    socket_t Socket::handle() const {
        return mHandle;
    }
    
    SocketAddress Socket::local() const {
        sockaddr addr;
        socklen_t len = sizeof(addr);
        ::memset(&addr, 0, len);
        ::getsockname(mHandle, &addr, &len);
        return addr;
    }
    
    SocketAddress Socket::remote() const {
        sockaddr addr;
        socklen_t len = sizeof(addr);
        ::memset(&addr, 0, len);
        ::getpeername(mHandle, &addr, &len);
        return addr;
    }
    
    int Socket::setOpt(int level, int name, void* value, socklen_t len) {
        return ::setsockopt(mHandle, level, name, (const char*) value, len);
    }
    
    int Socket::getOpt(int level, int name, void* value, socklen_t* len) {
        return ::getsockopt(mHandle, level, name, (char*) value, len);
    }
    
    bool Socket::alive() const {
        u32_t bytesSent = ::send(mHandle, "", 0, 0);
        return bytesSent != -1;
    }
    
    bool Socket::bind(const SocketAddress& addr) const {
        sockaddr sa = addr;
        return ::bind(mHandle, &sa, sizeof(sa)) == 0;
    }
    
    bool Socket::listen(int maxconn) const {
        maxconn = maxconn <= 0 ? SOMAXCONN : maxconn;
        maxconn = maxconn >= SOMAXCONN ? SOMAXCONN : maxconn;
        return ::listen(mHandle, maxconn) != -1;
    }
    
    bool Socket::connect(const SocketAddress& addr) const {
        sockaddr sa = addr;
        return ::connect(mHandle, &sa, sizeof(sa)) == 0;
    }
    
    Socket Socket::accept() const {
        socket_t s = ::accept(mHandle, nullptr, nullptr);
        return Socket(s);
    }
    
    u32_t Socket::recv(void* data, u32_t size) const {
        return ::recv(mHandle, (char*) data, size, 0);
    }
    
    u32_t Socket::send(void* data, u32_t size) const {
        return ::send(mHandle, (char*) data, size, 0);
    }
    
    u32_t Socket::recvFrom(void* data, u32_t size, const SocketAddress& addr) {
        sockaddr sa = addr;
        socklen_t len = sizeof(sa);
        return ::recvfrom(mHandle, (char*) data, size, 0, &sa, &len);
    }
    
    u32_t Socket::sendTo(void* data, u32_t size, const SocketAddress& addr) {
        sockaddr sa = addr;
        return ::sendto(mHandle, (char*) data, size, 0, &sa, sizeof(sa));
    }
    
    /*
    ===========================================================
    == SocketOptions
    ===========================================================
    */
    const SocketOptions::Option<SOL_SOCKET, SO_BROADCAST, bool> SocketOptions::broadcast;
    const SocketOptions::Option<SOL_SOCKET, SO_DONTROUTE, bool> SocketOptions::dontroute;
    const SocketOptions::Option<SOL_SOCKET, SO_LINGER, linger> SocketOptions::linger;
    const SocketOptions::Option<SOL_SOCKET, SO_KEEPALIVE, bool> SocketOptions::keepalive;
    const SocketOptions::Option<SOL_SOCKET, SO_OOBINLINE, bool> SocketOptions::oobinline;
    const SocketOptions::Option<SOL_SOCKET, SO_REUSEADDR, bool> SocketOptions::reuseaddr;
    const SocketOptions::Option<SOL_SOCKET, SO_RCVTIMEO, int> SocketOptions::recvtimeout;
    const SocketOptions::Option<SOL_SOCKET, SO_SNDTIMEO, int> SocketOptions::sendtimeout;
    const SocketOptions::Option<SOL_SOCKET, SO_RCVBUF, int> SocketOptions::recvbuffersize;
    const SocketOptions::Option<SOL_SOCKET, SO_SNDBUF, int> SocketOptions::sendbuffersize;
    const SocketOptions::Option<IPPROTO_TCP, TCP_NODELAY, int> SocketOptions::nodelay;
    
}

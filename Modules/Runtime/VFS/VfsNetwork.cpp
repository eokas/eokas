#include "VfsNetwork.h"

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32 || _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
#define EOKAS_VFS_SOCKET 1
#include "Base/Socket.h"
#include <cstring>
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#define EOKAS_VFS_SOCKET 0
#endif

namespace eokas
{
#if EOKAS_VFS_SOCKET

    static const u32_t kMagic = 0x53465645u;
    static const u16_t kVersion = 2;
    static const u32_t kMaxPayload = 1u << 20;
    static const u64_t kMaxMessage = 64ull << 20;
    static const u16_t kMore = 1;
    static const u16_t kGet = 1;
    static const u16_t kPut = 2;
    static const u16_t kRemove = 3;
    static const u16_t kList = 4;
    static const u16_t kExists = 5;

    static void startupSockets()
    {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        static bool ready = false;
        if (!ready)
        {
            WSADATA data;
            ::WSAStartup(MAKEWORD(2, 2), &data);
            ready = true;
        }
#endif
    }

    static bool sendAll(Socket& socket, const void* data, u32_t size)
    {
        const u8_t* cursor = static_cast<const u8_t*>(data);
        u32_t sent = 0;
        while (sent < size)
        {
            u32_t step = socket.send((void*) (cursor + sent), size - sent);
            if (step == 0 || step > size - sent)
                return false;
            sent += step;
        }
        return true;
    }

    static bool recvAll(Socket& socket, void* data, u32_t size)
    {
        u8_t* cursor = static_cast<u8_t*>(data);
        u32_t got = 0;
        while (got < size)
        {
            u32_t step = socket.recv(cursor + got, size - got);
            if (step == 0 || step > size - got)
                return false;
            got += step;
        }
        return true;
    }

    struct Frame
    {
        u16_t op;
        u16_t status;
        String key;
        std::vector<u8_t> payload;
    };

    static void appendU16(std::vector<u8_t>& out, u16_t value)
    {
        out.push_back((u8_t) (value & 0xff));
        out.push_back((u8_t) ((value >> 8) & 0xff));
    }

    static void appendU32(std::vector<u8_t>& out, u32_t value)
    {
        out.push_back((u8_t) (value & 0xff));
        out.push_back((u8_t) ((value >> 8) & 0xff));
        out.push_back((u8_t) ((value >> 16) & 0xff));
        out.push_back((u8_t) ((value >> 24) & 0xff));
    }

    static u16_t readU16(const u8_t* bytes)
    {
        return (u16_t) bytes[0] | ((u16_t) bytes[1] << 8);
    }

    static u32_t readU32(const u8_t* bytes)
    {
        return (u32_t) bytes[0] | ((u32_t) bytes[1] << 8) | ((u32_t) bytes[2] << 16) | ((u32_t) bytes[3] << 24);
    }

    static bool writeFrame(Socket& socket, const Frame& frame, u16_t flags)
    {
        if (frame.key.length() > kMaxPayload || frame.payload.size() > kMaxPayload)
            return false;
        std::vector<u8_t> header;
        appendU32(header, kMagic);
        appendU16(header, kVersion);
        appendU16(header, frame.op);
        appendU16(header, frame.status);
        appendU16(header, flags);
        appendU32(header, (u32_t) frame.key.length());
        appendU32(header, (u32_t) frame.payload.size());
        if (!sendAll(socket, header.data(), (u32_t) header.size()))
            return false;
        if (frame.key.length() > 0 && !sendAll(socket, frame.key.cstr(), (u32_t) frame.key.length()))
            return false;
        if (!frame.payload.empty() && !sendAll(socket, frame.payload.data(), (u32_t) frame.payload.size()))
            return false;
        return true;
    }

    static bool writeMessage(Socket& socket, const Frame& message)
    {
        if (message.key.length() > kMaxPayload || message.payload.size() > kMaxMessage)
            return false;
        u64_t offset = 0;
        for (;;)
        {
            u64_t remain = message.payload.size() - offset;
            bool more = remain > kMaxPayload;
            u32_t chunk = more ? kMaxPayload : (u32_t) remain;
            Frame part;
            part.op = message.op;
            part.status = message.status;
            if (offset == 0)
                part.key = message.key;
            if (chunk > 0)
            {
                const u8_t* bytes = message.payload.data() + offset;
                part.payload.assign(bytes, bytes + chunk);
            }
            if (!writeFrame(socket, part, more ? kMore : 0))
                return false;
            offset += chunk;
            if (!more)
                return true;
        }
    }

    enum class RecvFrame
    {
        Ok,
        Closed,
        Error
    };

    static RecvFrame readFrame(Socket& socket, Frame& frame, u16_t& flags)
    {
        frame.key = String();
        frame.payload.clear();
        flags = 0;
        u8_t header[20];
        u32_t got = 0;
        while (got < 20)
        {
            u32_t step = socket.recv(header + got, 20 - got);
            if (step == 0 || step > 20 - got)
                return got == 0 ? RecvFrame::Closed : RecvFrame::Error;
            got += step;
        }
        u32_t magic = readU32(header);
        u16_t version = readU16(header + 4);
        frame.op = readU16(header + 6);
        frame.status = readU16(header + 8);
        flags = readU16(header + 10);
        u32_t keyLen = readU32(header + 12);
        u32_t payloadLen = readU32(header + 16);
        if (magic != kMagic || keyLen > kMaxPayload || payloadLen > kMaxPayload)
            return RecvFrame::Error;
        bool badVersion = version != kVersion;
        if (keyLen > 0)
        {
            std::vector<char> key(keyLen + 1, '\0');
            if (!recvAll(socket, key.data(), keyLen))
                return RecvFrame::Error;
            frame.key = String(key.data());
        }
        frame.payload.assign(payloadLen, 0);
        if (payloadLen > 0 && !recvAll(socket, frame.payload.data(), payloadLen))
            return RecvFrame::Error;
        if (badVersion)
            frame.status = (u16_t) VfsResult::Unsupported;
        return RecvFrame::Ok;
    }

    static RecvFrame readMessage(Socket& socket, Frame& message)
    {
        message.key = String();
        message.payload.clear();
        bool first = true;
        for (;;)
        {
            Frame part;
            u16_t flags = 0;
            RecvFrame received = readFrame(socket, part, flags);
            if (received != RecvFrame::Ok)
                return received;
            if ((flags & kMore) != 0 && part.payload.empty())
                return RecvFrame::Error;
            if (part.payload.size() > kMaxMessage - message.payload.size())
                return RecvFrame::Error;
            if (first)
            {
                message.op = part.op;
                message.status = part.status;
                message.key = part.key;
                first = false;
            }
            message.payload.insert(message.payload.end(), part.payload.begin(), part.payload.end());
            if ((flags & kMore) == 0)
                return RecvFrame::Ok;
        }
    }

    static bool knownStatus(u16_t status)
    {
        switch (status)
        {
        case (u16_t) VfsResult::Ok:
        case (u16_t) VfsResult::InvalidPath:
        case (u16_t) VfsResult::NotFound:
        case (u16_t) VfsResult::AlreadyExists:
        case (u16_t) VfsResult::NotEmpty:
        case (u16_t) VfsResult::Access:
        case (u16_t) VfsResult::Io:
        case (u16_t) VfsResult::Corrupt:
        case (u16_t) VfsResult::Mismatch:
        case (u16_t) VfsResult::Unsupported:
        case (u16_t) VfsResult::Closed:
            return true;
        default:
            return false;
        }
    }

    static void dropSocket(Socket& socket, bool& open)
    {
        socket.close();
        open = false;
    }

    static VfsResult encodeKeys(const std::vector<String>& keys, std::vector<u8_t>& out)
    {
        out.clear();
        appendU32(out, (u32_t) keys.size());
        for (auto& key : keys)
        {
            if (key.length() > 0xFFFFu)
                return VfsResult::Io;
            appendU16(out, (u16_t) key.length());
            const char* text = key.cstr();
            out.insert(out.end(), text, text + key.length());
        }
        return VfsResult::Ok;
    }

    static VfsResult decodeKeys(const std::vector<u8_t>& bytes, std::vector<String>& out)
    {
        out.clear();
        if (bytes.size() < 4)
            return VfsResult::Corrupt;
        u32_t count = readU32(bytes.data());
        u64_t pos = 4;
        for (u32_t index = 0; index < count; ++index)
        {
            if (pos + 2 > bytes.size())
                return VfsResult::Corrupt;
            u16_t length = readU16(bytes.data() + pos);
            pos += 2;
            if (pos + length > bytes.size())
                return VfsResult::Corrupt;
            std::vector<char> text((size_t) length + 1, '\0');
            if (length > 0)
                std::memcpy(text.data(), bytes.data() + pos, length);
            pos += length;
            out.push_back(String(text.data()));
        }
        return VfsResult::Ok;
    }

    struct VfsNetworkVolume::Conn
    {
        Socket socket;
        bool open = false;
    };

    struct VfsNetworkServer::Conn
    {
        Socket listen;
        bool open = false;
    };

    static void armTimeout(Socket& socket)
    {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        DWORD timeout = 5000;
        socket.setOpt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#else
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        socket.setOpt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
    }

    static VfsResult call(Socket& socket, bool& open, u16_t op, const String& key, const void* data, u64_t size, Frame& response)
    {
        if (size > kMaxMessage)
            return VfsResult::Io;
        Frame request;
        request.op = op;
        request.status = 0;
        request.key = key;
        if (size > 0 && data != nullptr)
        {
            const u8_t* bytes = static_cast<const u8_t*>(data);
            request.payload.assign(bytes, bytes + (size_t) size);
        }
        if (!writeMessage(socket, request))
        {
            dropSocket(socket, open);
            return VfsResult::Io;
        }
        RecvFrame received = readMessage(socket, response);
        if (received != RecvFrame::Ok)
        {
            dropSocket(socket, open);
            return VfsResult::Io;
        }
        if (!knownStatus(response.status))
            return VfsResult::Corrupt;
        return (VfsResult) response.status;
    }

    static VfsResult connectSocket(Socket& socket, bool& open, const String& host, u16_t port);

    static VfsResult invoke(Socket& socket, bool& open, const String& host, u16_t port, u16_t op, const String& key, const void* data, u64_t size, Frame& response)
    {
        VfsResult connected = connectSocket(socket, open, host, port);
        if (connected != VfsResult::Ok)
            return connected;
        return call(socket, open, op, key, data, size, response);
    }

#else

    struct VfsNetworkVolume::Conn
    {
    };

    struct VfsNetworkServer::Conn
    {
    };

#endif

    VfsNetworkVolume::VfsNetworkVolume(const String& host, u16_t port)
        : mHost(host)
        , mPort(port)
        , mConn(new Conn())
    {
    }

    VfsNetworkVolume::~VfsNetworkVolume()
    {
#if EOKAS_VFS_SOCKET
        if (mConn != nullptr && mConn->open)
            mConn->socket.close();
#endif
        delete mConn;
        mConn = nullptr;
    }

#if EOKAS_VFS_SOCKET
    static VfsResult connectSocket(Socket& socket, bool& open, const String& host, u16_t port)
    {
        if (open)
            return VfsResult::Ok;
        startupSockets();
        if (!socket.open(AF_INET, SOCK_STREAM, IPPROTO_TCP))
            return VfsResult::Io;
        armTimeout(socket);
        InetAddress address(host.cstr(), port);
        if (!socket.connect(address))
        {
            socket.close();
            open = false;
            return VfsResult::Io;
        }
        open = true;
        return VfsResult::Ok;
    }
#endif

    VfsResult VfsNetworkVolume::exists(const String& key, bool& out)
    {
#if EOKAS_VFS_SOCKET
        Frame response;
        VfsResult result = invoke(mConn->socket, mConn->open, mHost, mPort, kExists, key, nullptr, 0, response);
        if (result != VfsResult::Ok)
            return result;
        if (response.payload.size() != 1)
            return VfsResult::Corrupt;
        out = response.payload[0] != 0;
        return VfsResult::Ok;
#else
        (void) key;
        (void) out;
        return VfsResult::Unsupported;
#endif
    }

    VfsResult VfsNetworkVolume::get(const String& key, std::vector<u8_t>& out)
    {
#if EOKAS_VFS_SOCKET
        Frame response;
        VfsResult result = invoke(mConn->socket, mConn->open, mHost, mPort, kGet, key, nullptr, 0, response);
        if (result != VfsResult::Ok)
            return result;
        out = response.payload;
        return VfsResult::Ok;
#else
        (void) key;
        (void) out;
        return VfsResult::Unsupported;
#endif
    }

    VfsResult VfsNetworkVolume::put(const String& key, const void* data, u64_t size)
    {
#if EOKAS_VFS_SOCKET
        Frame response;
        return invoke(mConn->socket, mConn->open, mHost, mPort, kPut, key, data, size, response);
#else
        (void) key;
        (void) data;
        (void) size;
        return VfsResult::Unsupported;
#endif
    }

    VfsResult VfsNetworkVolume::remove(const String& key)
    {
#if EOKAS_VFS_SOCKET
        Frame response;
        return invoke(mConn->socket, mConn->open, mHost, mPort, kRemove, key, nullptr, 0, response);
#else
        (void) key;
        return VfsResult::Unsupported;
#endif
    }

    VfsResult VfsNetworkVolume::list(const String& prefix, std::vector<String>& out)
    {
#if EOKAS_VFS_SOCKET
        Frame response;
        VfsResult result = invoke(mConn->socket, mConn->open, mHost, mPort, kList, prefix, nullptr, 0, response);
        if (result != VfsResult::Ok)
            return result;
        return decodeKeys(response.payload, out);
#else
        (void) prefix;
        (void) out;
        return VfsResult::Unsupported;
#endif
    }

    VfsNetworkServer::VfsNetworkServer(VfsVolume& backend)
        : mBackend(&backend)
        , mConn(new Conn())
        , mPort(0)
    {
    }

    VfsNetworkServer::~VfsNetworkServer()
    {
#if EOKAS_VFS_SOCKET
        if (mConn != nullptr && mConn->open)
            mConn->listen.close();
#endif
        delete mConn;
        mConn = nullptr;
    }

    VfsResult VfsNetworkServer::listen(u16_t port)
    {
#if EOKAS_VFS_SOCKET
        startupSockets();
        if (mConn->open)
            mConn->listen.close();
        if (!mConn->listen.open(AF_INET, SOCK_STREAM, IPPROTO_TCP))
            return VfsResult::Io;
        int reuse = 1;
        mConn->listen.setOpt(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        InetAddress address("127.0.0.1", port);
        if (!mConn->listen.bind(address) || !mConn->listen.listen(8))
        {
            mConn->listen.close();
            return VfsResult::Io;
        }
        armTimeout(mConn->listen);
        SocketAddress local = mConn->listen.local();
        InetAddress bound(*(sockaddr_in*) &local.value);
        mPort = bound.port();
        mConn->open = true;
        return VfsResult::Ok;
#else
        (void) port;
        return VfsResult::Unsupported;
#endif
    }

    u16_t VfsNetworkServer::port() const
    {
        return mPort;
    }

    VfsResult VfsNetworkServer::serveOne()
    {
#if EOKAS_VFS_SOCKET
        if (!mConn->open)
            return VfsResult::Io;
        Socket client = mConn->listen.accept();
        if (!client.isOpen())
            return VfsResult::Io;
        armTimeout(client);
        for (;;)
        {
            Frame request;
            RecvFrame received = readMessage(client, request);
            if (received == RecvFrame::Closed)
            {
                client.close();
                return VfsResult::Ok;
            }
            if (received != RecvFrame::Ok)
            {
                client.close();
                return VfsResult::Io;
            }
            Frame response;
            response.op = request.op;
            response.status = (u16_t) VfsResult::Ok;
            if (request.status == (u16_t) VfsResult::Unsupported)
            {
                response.status = (u16_t) VfsResult::Unsupported;
            }
            else if (request.op == kGet)
            {
                VfsResult result = mBackend->get(request.key, response.payload);
                response.status = (u16_t) result;
                if (result != VfsResult::Ok)
                    response.payload.clear();
            }
            else if (request.op == kPut)
            {
                VfsResult result = mBackend->put(request.key, request.payload.empty() ? nullptr : request.payload.data(), request.payload.size());
                response.status = (u16_t) result;
            }
            else if (request.op == kRemove)
            {
                response.status = (u16_t) mBackend->remove(request.key);
            }
            else if (request.op == kList)
            {
                std::vector<String> keys;
                VfsResult result = mBackend->list(request.key, keys);
                response.status = (u16_t) result;
                if (result == VfsResult::Ok)
                {
                    result = encodeKeys(keys, response.payload);
                    response.status = (u16_t) result;
                }
            }
            else if (request.op == kExists)
            {
                bool present = false;
                VfsResult result = mBackend->exists(request.key, present);
                response.status = (u16_t) result;
                if (result == VfsResult::Ok)
                    response.payload.push_back(present ? 1 : 0);
            }
            else
            {
                response.status = (u16_t) VfsResult::Unsupported;
            }
            if (!writeMessage(client, response))
            {
                client.close();
                return VfsResult::Io;
            }
        }
#else
        return VfsResult::Unsupported;
#endif
    }

    void vfsNetworkBreakSocket(VfsNetworkVolume& volume)
    {
#if EOKAS_VFS_SOCKET
        if (volume.mConn != nullptr && volume.mConn->open)
            volume.mConn->socket.close();
#else
        (void) volume;
#endif
    }
}

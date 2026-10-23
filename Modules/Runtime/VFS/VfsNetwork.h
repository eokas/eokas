#ifndef _EOKAS_VFS_NETWORK_H_
#define _EOKAS_VFS_NETWORK_H_

#include "VfsVolume.h"

namespace eokas
{
    class VfsNetworkVolume : public VfsVolume
    {
    public:
        VfsNetworkVolume(const String& host, u16_t port);
        ~VfsNetworkVolume() override;

        VfsResult exists(const String& key, bool& out) override;
        VfsResult get(const String& key, std::vector<u8_t>& out) override;
        VfsResult put(const String& key, const void* data, u64_t size) override;
        VfsResult remove(const String& key) override;
        VfsResult list(const String& prefix, std::vector<String>& out) override;

        friend void vfsNetworkBreakSocket(VfsNetworkVolume& volume);

    private:
        String mHost;
        u16_t mPort;
        struct Conn;
        Conn* mConn;
    };

    class VfsNetworkServer
    {
    public:
        explicit VfsNetworkServer(VfsVolume& backend);
        ~VfsNetworkServer();

        VfsResult listen(u16_t port);
        u16_t port() const;
        VfsResult serveOne();

    private:
        VfsVolume* mBackend;
        struct Conn;
        Conn* mConn;
        u16_t mPort;
    };

    void vfsNetworkBreakSocket(VfsNetworkVolume& volume);
}

#endif//_EOKAS_VFS_NETWORK_H_

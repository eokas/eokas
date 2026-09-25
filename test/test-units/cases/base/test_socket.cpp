#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(socket) {
    WSADATA wsaData;
    EOKAS_EXPECT(::WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);

    InetAddress address("127.0.0.1", 8080);
    EOKAS_EXPECT(String(address.ip()) == "127.0.0.1");
    EOKAS_EXPECT(address.port() == 8080);
    EOKAS_EXPECT(ByteOrder::n2hI16(ByteOrder::h2nI16(0x1234)) == 0x1234);
    EOKAS_EXPECT(ByteOrder::n2hI32(ByteOrder::h2nI32(0x12345678)) == 0x12345678);

    Socket socket;
    EOKAS_EXPECT(!socket.isOpen());
    EOKAS_EXPECT(socket.open(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    EOKAS_EXPECT(socket.isOpen());
    socket.close();
    EOKAS_EXPECT(!socket.isOpen());

    ::WSACleanup();
    return 0;
}

			  
#include "./network.h"

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32

#include <mutex>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment(lib, "ws2_32.lib")

namespace eokas {
    
    void ShowError(DWORD error) {
        // Retrieve the system error message for windows error-code
        DWORD flag = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        DWORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        LPSTR msg = NULL;
        FormatMessageA(flag, NULL, error, lang, (LPSTR) &msg, 0, NULL);
        
        // Display the error message and exit the process
        char txt[1024] = {0};
        sprintf(txt, "ERROR %u: %s", error, msg);
        MessageBoxA(NULL, txt, "Error", MB_OK);
        
        LocalFree(msg);
    }
    
    struct IOCP {
        static IOCP instance;
        
        static DWORD WINAPI thread_func(LPVOID args) {
            IOCP* iocp = (IOCP*) args;
            return iocp != nullptr ? iocp->exec_oper() : 0;
        }
        
        HANDLE handle;
        std::vector<HANDLE> threads;
        LPFN_ACCEPTEX acceptEx;
        LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockAddrs;
        std::mutex mutex;
        
        IOCP()
            : handle(INVALID_HANDLE_VALUE), threads(), acceptEx(nullptr), getAcceptExSockAddrs(nullptr), mutex() {
        }
        
        bool init() {
            this->handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
            if (this->handle == INVALID_HANDLE_VALUE)
                return false;
            
            SYSTEM_INFO systemInfo;
            ::GetSystemInfo(&systemInfo);
            int cpuCount = systemInfo.dwNumberOfProcessors;
            int threadCount = cpuCount * 2;
            for (int i = 0; i < threadCount; i++) {
                HANDLE thread = ::CreateThread(NULL, 0, thread_func, this, 0, NULL);
                this->threads.push_back(thread);
            }
            
            return true;
        }
        
        void quit() {
            if (!this->threads.empty()) {
                size_t count = this->threads.size();
                HANDLE* handles = this->threads.data();
                while (WaitForMultipleObjects((DWORD) count, handles, TRUE, 0) != WAIT_OBJECT_0) {
                    PostQueuedCompletionStatus(this->handle, 0, 0, nullptr);
                }
                
                for (auto iter = this->threads.begin(); iter != this->threads.end(); ++iter) {
                    HANDLE thread = *iter;
                    if (thread != INVALID_HANDLE_VALUE) {
                        ::CloseHandle(thread);
                    }
                }
                this->threads.clear();
            }
            
            if (this->handle != INVALID_HANDLE_VALUE) {
                CloseHandle(this->handle);
                this->handle = INVALID_HANDLE_VALUE;
            }
        }
        
        bool bind_session(NetworkSession* session) {
            HANDLE s = (HANDLE) session->socket.handle();
            HANDLE h = ::CreateIoCompletionPort(s, this->handle, (ULONG_PTR) session, 0);
            return h == this->handle;
        }
        
        void* load_extfunc(const Socket& socket, GUID& guid) {
            void* func = nullptr;
            DWORD bytes = 0;
            WSAIoctl(socket.handle(), SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &func, sizeof(func), &bytes, nullptr, nullptr);
            return func;
        }
        
        bool load_extfuncs(const Socket& socket) {
            if (this->acceptEx == nullptr) {
                GUID guid = WSAID_ACCEPTEX;
                this->acceptEx = (LPFN_ACCEPTEX) load_extfunc(socket, guid);
            }
            if (this->acceptEx == nullptr)
                return false;
            
            if (this->getAcceptExSockAddrs == nullptr) {
                GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
                this->getAcceptExSockAddrs = (LPFN_GETACCEPTEXSOCKADDRS) load_extfunc(socket, guid);
            }
            if (this->getAcceptExSockAddrs == nullptr)
                return false;
            
            return true;
        }
        
        bool post_oper(NetworkSession* session, NetworkOperation* oper) {
            if (session == nullptr || oper == nullptr)
                return false;
            if (oper->type <= OperationType::None || oper->type >= OperationType::Max)
                return false;
            
            memset(&oper->overlapped, 0, sizeof(OVERLAPPED));
            oper->wsabuf.buf = oper->data;
            oper->wsabuf.len = oper->size;
            
            if (oper->type == OperationType::Accept) {
                oper->acceptedSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
                if (!oper->acceptedSocket.isOpen())
                    return false;
                size_t addrLen = sizeof(sockaddr_in) + 16;
                if (oper->wsabuf.len < addrLen * 2)
                    return false;
                
                // make oper->wsabuf.len - addrLen * 2 == 0
                oper->wsabuf.len = (ULONG) (addrLen * 2);
                
                DWORD trans = 0;
                if (TRUE !=
                    this->acceptEx(session->socket, oper->acceptedSocket, oper->wsabuf.buf, (DWORD) (oper->wsabuf.len - addrLen * 2), (DWORD) addrLen, (DWORD) addrLen, &trans, &oper->overlapped)) {
                    int error = GetLastError();
                    if (error != ERROR_IO_PENDING) {
                        oper->acceptedSocket.close();
                        return false;
                    }
                }
            } else if (oper->type == OperationType::Recv) {
                DWORD trans = 0;
                DWORD flags = 0;
                int ret = WSARecv(session->socket, &oper->wsabuf, 1, &trans, &flags, &oper->overlapped, NULL);
                int err = ::WSAGetLastError();
                if (err == ERROR_NETNAME_DELETED)
                    err = WSAECONNRESET;
                else if (err == ERROR_PORT_UNREACHABLE)
                    err = WSAECONNREFUSED;
                
                if (ret != 0 && err != WSA_IO_PENDING)
                    return false;
            } else if (oper->type == OperationType::Send) {
                DWORD trans = 0;
                DWORD flags = 0;
                int ret = WSASend(session->socket, &oper->wsabuf, 1, &trans, flags, &oper->overlapped, NULL);
                int err = ::WSAGetLastError();
                if (err == ERROR_NETNAME_DELETED)
                    err = WSAECONNRESET;
                else if (err == ERROR_PORT_UNREACHABLE)
                    err = WSAECONNREFUSED;
                
                if (ret != 0 && err != WSA_IO_PENDING)
                    return false;
            } else {
                int trans = oper->size;
                BOOL result = PostQueuedCompletionStatus(this->handle, trans, (ULONG_PTR) session, &oper->overlapped);
                if (result == FALSE)
                    return false;
            }
            
            this->mutex.lock();
            session->operations.push_back(oper);
            this->mutex.unlock();
            
            return true;
        }
        
        DWORD exec_oper() {
            DWORD trans = 0;
            ULONG_PTR completkey;
            LPOVERLAPPED overlapped;
            for (;;) {
                BOOL result = GetQueuedCompletionStatus(this->handle, &trans, &completkey, &overlapped, WSA_INFINITE);
                NetworkSession* session = (NetworkSession*) completkey;
                NetworkOperation* oper = (NetworkOperation*) overlapped;
                if (result && session == nullptr && oper == nullptr)
                    break;
                if (session == nullptr || oper == nullptr)
                    continue;
                
                this->mutex.lock();
                session->operations.remove(oper);
                this->mutex.unlock();
                
                if (result != TRUE) {
                    DWORD error = GetLastError();
                    this->handle_error(session, oper, error);
                    continue;
                }
                
                if ((oper->type == OperationType::Recv || oper->type == OperationType::Send) && trans == 0) {
                    this->invoke_callback(session, oper, NetworkError::Broken);
                    session->socket.close();
                    return true;
                }
                
                if (oper->type == OperationType::Accept) {
                    int addrLen = sizeof(sockaddr_in) + 16;
                    int dataLen = oper->wsabuf.len - addrLen * 2;
                    
                    sockaddr_in* addrL = nullptr;
                    int lenL = 0;
                    sockaddr_in* addrR = nullptr;
                    int lenR = 0;
                    
                    this->getAcceptExSockAddrs(oper->wsabuf.buf, dataLen, addrLen, addrLen, (sockaddr**) &addrL, &lenL, (sockaddr**) &addrR, &lenR);
                    
                    oper->size = trans;
                    this->invoke_callback(session, oper);
                } else {
                    oper->size = trans;
                    this->invoke_callback(session, oper);
                }
            }
            
            return 0;
        }
        
        void handle_error(NetworkSession* session, NetworkOperation* oper, int error) {
            if (error == ERROR_OPERATION_ABORTED) {
                if (oper->type == OperationType::Accept) {
                    oper->acceptedSocket.close();
                }
                this->invoke_callback(session, oper, NetworkError::Aborted);
            } else if (error == ERROR_NETNAME_DELETED) {
                if (oper->type == OperationType::Accept) {
                    oper->acceptedSocket.close();
                    this->invoke_callback(session, oper, NetworkError::Expired);
                } else {
                    this->invoke_callback(session, oper, NetworkError::Broken);
                }
            } else if (error == WAIT_TIMEOUT) {
                if (!session->socket.alive()) {
                    this->invoke_callback(session, oper, NetworkError::Broken);
                }
            }
        }
        
        void invoke_callback(NetworkSession* session, NetworkOperation* oper, NetworkError error = NetworkError::None) {
            if (session == nullptr || session->callback == nullptr)
                return;
            session->callback(session, oper, error);
        }
    };
    
    IOCP IOCP::instance;
    
    /*
    ============================================================
    ==== NetworkSession
    ============================================================
    */
    bool NetworkSession::open() {
        return this->open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    
    bool NetworkSession::open(AddressFamily family, SocketType socktype, ProtocolType protocol) {
        Socket s = ::WSASocket(family, socktype, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
        return this->open(s);
    }
    
    bool NetworkSession::open(const Socket& s) {
        if (!s.isOpen())
            return false;
        this->socket = s;
        if (!IOCP::instance.bind_session(this))
            return false;
        return true;
    }
    
    void NetworkSession::close() {
        this->socket.shutdown(Socket::ShutdownMethod::Send);
        this->socket.close();
    }
    
    bool NetworkSession::listen(const SocketAddress& addr, int maxconn) {
        if (!IOCP::instance.load_extfuncs(this->socket))
            return false;
        return this->socket.bind(addr) && this->socket.listen(maxconn);
    }
    
    bool NetworkSession::connect(const SocketAddress& addr) {
        return this->socket.connect(addr);
    }
    
    bool NetworkSession::post(NetworkOperation* oper) {
        return IOCP::instance.post_oper(this, oper);
    }
    
    /*
    ============================================================
    ==== NetworkService
    ============================================================
    */
    NetworkService::NetworkService() {
    }
    
    NetworkService::~NetworkService() {
    }
    
    bool NetworkService::init() {
        WSADATA wsaData;
        ZeroMemory(&wsaData, sizeof(wsaData));
        if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return false;
        if (!IOCP::instance.init()) {
            ::WSACleanup();
            return false;
        }
        return true;
    }
    
    void NetworkService::quit() {
        IOCP::instance.quit();
        ::WSACleanup();
    }
    
}

#elif _EOKAS_OS == _EOKAS_OS_MACOS

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <thread>

namespace eokas {

#define _EVENT_COUNT 128

struct Kqueue
{
	static Kqueue instance;

	int kq;
	bool runing;
	std::thread thread;
	std::mutex mutex;


	bool init()
	{
		this->kq = kqueue();
		this->runing = true;
		this->thread = std::thread(&Kqueue::exec_oper, this);
		return true;
	}

	void quit()
	{
		this->runing = false;
		this->thread.join();
		kq = 0;
	}

	bool post_oper(NetworkSession* session, NetworkOperation* oper)
	{
		if (session == nullptr || oper == nullptr)
			return false;
		if (oper->type < OperationType::Accept || oper->type > OperationType::Send)
			return false;

		oper->session = session;

		struct kevent changes[1];
		if (oper->type == OperationType::Accept)
		{
			EV_SET(&changes[0], session->socket, EVFILT_READ, EV_ADD, 0, 0, oper);
		}
		else if (oper->type == OperationType::Recv)
		{
			EV_SET(&changes[0], session->socket, EVFILT_READ, EV_ADD, 0, 0, oper);
		}
		else if (oper->type == OperationType::Send)
		{
			EV_SET(&changes[0], session->socket, EVFILT_WRITE, EV_ADD, 0, 0, oper);
		}

		int ret = kevent(this->kq, changes, 1, NULL, 0, NULL);
		if (ret == -1)
			return false;

		this->mutex.lock();
		session->operations.push_back(oper);
		this->mutex.unlock();

		return true;
	}

	void exec_oper()
	{
		struct kevent events[_EVENT_COUNT];
		while (this->runing)
		{
			int ret = kevent(this->kq, NULL, 0, events, _EVENT_COUNT, NULL);
			if (ret == -1)
			{
				printf("kevent failed!\n");
				continue;
			}

			this->handle_event(events, ret);
		}
	}

	void handle_event(struct kevent* events, int nevents)
	{
		for (int i = 0; i < nevents; i++)
		{
			long sock = events[i].ident;
			long data = events[i].data;

			NetworkOperation* oper = (NetworkOperation*)events[i].udata;
			if (oper == nullptr)
				continue;
			NetworkSession* session = oper->session;
			if (session == nullptr)
				continue;

			this->mutex.lock();
			session->operations.remove(oper);
			this->mutex.unlock();

			if (oper->type == OperationType::Accept)
			{
				this->launch_accept(session, oper, data);
			}
			else if (oper->type == OperationType::Recv)
			{
				this->launch_recv(session, oper, data);
			}
			else if (oper->type == OperationType::Send)
			{

			}
		}
	}

	void launch_accept(NetworkSession* session, NetworkOperation* oper, long nconnections)
	{
		for (int i = 0; i < nconnections; i++)
		{
			int client = session->socket.accept();
			if (client == -1)
			{
				printf("Accept failed.\n");
				continue;
			}

			oper->acceptedSocket = client;
			this->invoke_callback(session, oper);
		}
	}

	void launch_recv(NetworkSession* session, NetworkOperation* oper, long size)
	{
		oper->size = oper->size < size ? oper->size : size;
		u32_t len = session->socket.recv(oper->data, oper->size);
		if (len == 0 || len == -1)
		{
			this->invoke_callback(session, oper, NetworkError::Broken);
			session->socket.close();
			return;
		}

		this->invoke_callback(session, oper);
	}

	void invoke_callback(NetworkSession* session, NetworkOperation* oper, NetworkError error = NetworkError::None)
	{
		if(session == nullptr || session->callback == nullptr)
			return;
		session->callback(session, oper, error);		
	}
};

Kqueue Kqueue::instance;

/*
============================================================
==== NetworkSession
============================================================
*/
bool NetworkSession::open()
{
	return this->open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

bool NetworkSession::open(AddressFamily family, SocketType socktype, ProtocolType protocol)
{
	Socket s;
	s.open(family, socktype, protocol);
	return this->open(s);
}

bool NetworkSession::open(const Socket& s)
{
	if (!s.isOpen())
		return false;
	this->socket = s;
	return true;
}

void NetworkSession::close()
{
	this->socket.shutdown(Socket::ShutdownMethod::Send);
	this->socket.close();
}

bool NetworkSession::listen(const SocketAddress& addr, int maxconn)
{
	return this->socket.bind(addr)
		&& this->socket.listen(maxconn);
}

bool NetworkSession::connect(const SocketAddress& addr)
{
	return this->socket.connect(addr);
}

bool NetworkSession::post(NetworkOperation* oper)
{
	return Kqueue::instance.post_oper(this, oper);
}

/*
============================================================
==== NetworkService
============================================================
*/
NetworkService::NetworkService()
{}

NetworkService::~NetworkService()
{}

bool NetworkService::init()
{
	return Kqueue::instance.init();
}

void NetworkService::quit()
{
	Kqueue::instance.quit();
}

}

#endif

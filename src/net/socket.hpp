#include <iostream>
#include "../msutil.hpp"
#include "../silk.hpp"

#include <netinet/in.h>
#include <sys/socket.h>


namespace Sock {

class BasicSock {
public:
    enum class IpFormat {
        ipV4 = AF_INET,
        ipV6 = AF_INET6
    };
    enum class SocketFormat {
        TCP = SOCK_STREAM
    };
protected:
    i32 handle = 0;
    struct SocketInf {
        u32 port;
        enum IpFormat ip_fmt;
        enum SocketFormat s_fmt;
    } inf;

    union AddrInf {
        sockaddr_in v4;
        sockaddr_in6 v6;
    } addr_inf;
public:
    BasicSock(u32 port, SocketFormat s_fmt = SocketFormat::TCP, IpFormat ip_fmt = IpFormat::ipV4) {
        this->handle = socket((u32) ip_fmt, (u32) s_fmt, 0);

        this->inf.ip_fmt = ip_fmt;
        this->inf.s_fmt = s_fmt;
        this->inf.port = port;

        if (this->inf.ip_fmt == IpFormat::ipV4) {
            sockaddr_in target_addr;

            target_addr.sin_family = (sa_family_t) s_fmt;
            target_addr.sin_port = htons(this->inf.port);
            target_addr.sin_addr.s_addr = INADDR_ANY;

            this->addr_inf.v4 = target_addr;
        } else {
            sockaddr_in6 target_addr;

            target_addr.sin6_family = (sa_family_t) s_fmt;
            target_addr.sin6_port = htons(this->inf.port);
            target_addr.sin6_addr = IN6ADDR_ANY_INIT;

            this->addr_inf.v6 = target_addr;
        }
    }
    u32 getHandle() {
        return this->handle;
    }
    void close() {
        //close(this->handle);
        this->handle = 0;
    }
};



class Server : public BasicSock {
private:
    Silk::TPool *sessions;
    bool in_se = false;
public:
    Server(u32 port, SocketFormat s_fmt = SocketFormat::TCP, IpFormat ip_fmt = IpFormat::ipV4) : BasicSock(port, s_fmt, ip_fmt) {

    }
    u32 open(size_t nThreads) {
        if (this->in_se) {
            std::cout << "Cannot open already started server!" << std::endl;
            return 2;
        }

        const size_t sock_len = this->inf.ip_fmt == IpFormat::ipV4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

        //WARNING: union thing might fuck stuff up idk tho
        if (bind(this->handle, (const sockaddr*)&this->addr_inf, sock_len)) {
            std::cout << "Failed to bind to server socket!" << std::endl;

            return 1;
        }

        this->in_se = true;
        this->sessions = new Silk::TPool(nThreads);

        if (listen(this->handle, SOMAXCONN); //maybe change somaxconn idk) {
            std::cout << "Failed to listen for clients!" << std::endl;
            return 3;
        }

        

        return 0;
    }
    u32 c() {
        if (this->sessions) {
            _safe_free_a(this->sessions);
            this->sessions = nullptr;
        }
    }
};

class Client {

};

}
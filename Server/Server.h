#pragma once
#include <WinSock2.h>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdint>
#include "ClientSession.h"

#define SERVER_PORT      9000

class Server {
public:
    void        start(int port);
    void        acceptLoop();

    void        broadcast(const void* data, uint16_t size);
    void        removeSession(ClientSession* session);

private:
    friend class ServerPacketHandler;

    SOCKET                              listenSocket;
    std::vector<ClientSession*>         sessions;
    std::mutex                          sessionMutex;
    uint32_t                            nextPlayerID = 1;
};
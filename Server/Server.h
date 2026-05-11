#pragma once
#include <WinSock2.h>
#include <mutex>
#include <vector>
#include <cstdint>
#include "ClientSession.h"

class Server {
public:
    void        start(int port);   // 서버 시작
    void        acceptLoop();      // 클라이언트 접속 대기

private:
    SOCKET                              listenSocket;
    std::vector<ClientSession*>         sessions;
    std::mutex                          sessionMutex;  // 동기화
    uint32_t                            nextPlayerID = 1;

    void        broadcast(const void* data, uint16_t size);  // 전체 전송
    void        removeSession(ClientSession* session);  // 세션 제거
};
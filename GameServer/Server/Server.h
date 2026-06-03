#pragma once
#include <WinSock2.h>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdint>
#include <array>
#include "Room.h"
#include "ClientSession.h"

#define SERVER_PORT      9000

class Server {
public:
    void        start(int port);
    void        acceptLoop();
    void        initRooms();                           // 방 초기화
    Room*       getRoom(uint32_t roomID);              // roomID로 방 찾기
    void        removeSession(ClientSession* session); // 세션 제거 및 소켓 정리
    //void        broadcast(const void* data, uint16_t size); // 로그인한 모든 세션에 패킷 전송

private:
    friend class ServerPacketHandler;

    SOCKET                              listenSocket;
    std::vector<ClientSession*>         sessions;
    std::mutex                          sessionMutex;
    std::array<Room, ROOM_COUNT>        rooms;
    uint32_t                            nextPlayerID = 1;
};
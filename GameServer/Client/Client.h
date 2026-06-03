#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <cstdint>
#include "Packet.h"
#include "Player.h"

// 클라이언트 상태 종류
enum class ClientState {
    NOT_LOGGED_IN,
    WAITING,
    LOGGED_IN,
    SELECTING_ROOM,
    IN_ROOM,
};

class Client {
public:
    bool connectToServer(const std::string& ip, int port); // 서버 연결
    void run();                                            // 메인 루프 실행

private:
    friend class ClientPacketHandler;

    SOCKET      sock  = INVALID_SOCKET; 
    ClientState state = ClientState::NOT_LOGGED_IN;
    Player      player;
    uint32_t    roomID = 0; // 현재 들어가 있는 방

    void recvLoop();                                    // 수신 전용 루프
    void sendPacket(const void* data, uint16_t size);   // 패킷 송신

    // 각 요청 패킷 전송 함수
    void sendLogin(const std::string& nick);
    void sendRoomList();
    void sendRoomEnter(uint32_t roomID);   
    void sendRoomPlayersReq();
    void sendChat(const std::string& msg);
    void sendMove(int32_t x, int32_t y);
    void sendAttack(uint32_t targetID);
    void sendDisconnect();
};

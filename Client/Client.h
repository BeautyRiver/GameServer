#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <cstdint>
#include "Packet.h"
#include "Player.h"

enum class ClientState {
    NOT_LOGGED_IN,
    WAITING,
    LOGGED_IN,
    IN_ROOM,
    SELECTING_TARGET
};

class Client {
public:
    bool connectToServer(const std::string& ip, int port);
    void run();

private:
    friend class ClientPacketHandler;

    SOCKET      sock  = INVALID_SOCKET;
    ClientState state = ClientState::NOT_LOGGED_IN;
    Player      player;

    void recvLoop();
    void sendPacket(const void* data, uint16_t size);

    void sendLogin(const std::string& nick);
    void sendRoomEnter();
    void sendRoomPlayersReq();
    void sendChat(const std::string& msg);
    void sendMove(int32_t x, int32_t y);
    void sendAttack(uint32_t targetID);
    void sendDisconnect();
};

#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <thread>
#include <chrono>
#include "Client.h"
#include "ClientPacketHandler.h"

static bool recvAll(SOCKET sock, char* buf, int size) {
    int received = 0;
    while (received < size) {
        int ret = recv(sock, buf + received, size - received, 0);
        if (ret <= 0) return false;
        received += ret;
    }
    return true;
}

bool Client::connectToServer(const std::string& ip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cout << "[ERROR] socket() failed\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    std::cout << "[INFO] Connecting to server...\n";

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "[ERROR] connect() failed\n";
        return false;
    }
    system("cls");
    std::cout << "[INFO] Connected to server\n";
    return true;
}

void Client::recvLoop() {
    char buffer[1024];

    while (true) {
        if (!recvAll(sock, buffer, sizeof(PacketHeader))) break;

        PacketHeader* header   = reinterpret_cast<PacketHeader*>(buffer);
        uint16_t      bodySize = header->size - sizeof(PacketHeader);

        if (bodySize > 0) {
            if (!recvAll(sock, buffer + sizeof(PacketHeader), bodySize)) break;
        }

        ClientPacketHandler::handle(this, header);
    }

    std::cout << "[INFO] Server disconnected\n";
}

void Client::sendPacket(const void* data, uint16_t size) {
    const char* ptr  = reinterpret_cast<const char*>(data);
    int         sent = 0;
    while (sent < size) {
        int ret = send(sock, ptr + sent, size - sent, 0);
        if (ret == SOCKET_ERROR) return;
        sent += ret;
    }
}

void Client::run() {
    std::thread t(&Client::recvLoop, this);
    t.detach();

    while (true) {
        if (state == ClientState::NOT_LOGGED_IN) {
            std::cout << "\nEnter nickname: ";
            std::getline(std::cin, player.nickname);
            if (!player.nickname.empty()) {
                sendLogin(player.nickname);
                state = ClientState::WAITING;                
            }
        }
        else if (state == ClientState::WAITING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else if (state == ClientState::LOGGED_IN) {
            std::cout << "\n[MAIN MENU] 1.Enter Room  2.Quit\n> ";
            std::string input;
            std::getline(std::cin, input);
            if (input == "1") {
                sendRoomEnter();
                state = ClientState::WAITING;
            }
            else if (input == "2") {
                sendDisconnect();
                break;
            }

        }
        else if (state == ClientState::IN_ROOM) {
            std::cout << "\n[ROOM MENU] 1.Chat  2.Move  3.Attack  4.Quit\n> ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "1") {
                std::cout << "Message: ";
                std::string msg;
                std::getline(std::cin, msg);
                sendChat(msg);
            }
            else if (input == "2") {
                int32_t x, y;
                std::cout << "x: "; std::cin >> x;
                std::cout << "y: "; std::cin >> y;
                std::cin.ignore();
                sendMove(x, y);
            }
            else if (input == "3") {
                sendRoomPlayersReq();
                state = ClientState::WAITING;
            }
            else if (input == "4") {
                sendDisconnect();
                break;
            }
        }
        else if (state == ClientState::SELECTING_TARGET) {
            uint32_t targetID;
            std::cout << "Target playerID: "; std::cin >> targetID;
            std::cin.ignore();
            sendAttack(targetID);
            state = ClientState::IN_ROOM;
        }
    }

    closesocket(sock);
    WSACleanup();
}

void Client::sendLogin(const std::string& nick) {
    LoginReq pkt{};
    pkt.header.size = sizeof(LoginReq);
    pkt.header.type = PacketType::LOGIN_REQ;
    strncpy_s(pkt.nickname, sizeof(pkt.nickname), nick.c_str(), _TRUNCATE);
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendRoomPlayersReq() {
    RoomPlayersReq pkt{};
    pkt.header.size = sizeof(RoomPlayersReq);
    pkt.header.type = PacketType::ROOM_PLAYERS_REQ;
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendRoomEnter() {
    RoomEnterReq pkt{};
    pkt.header.size = sizeof(RoomEnterReq);
    pkt.header.type = PacketType::ROOM_ENTER_REQ;
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendChat(const std::string& msg) {
    ChatReq pkt{};
    pkt.header.size = sizeof(ChatReq);
    pkt.header.type = PacketType::CHAT_REQ;
    strncpy_s(pkt.message, sizeof(pkt.message), msg.c_str(), _TRUNCATE);
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendMove(int32_t x, int32_t y) {
    MoveReq pkt{};
    pkt.header.size = sizeof(MoveReq);
    pkt.header.type = PacketType::MOVE_REQ;
    pkt.x           = x;
    pkt.y           = y;
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendAttack(uint32_t targetID) {
    AttackReq pkt{};
    pkt.header.size = sizeof(AttackReq);
    pkt.header.type = PacketType::ATTACK_REQ;
    pkt.targetID    = targetID;
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendDisconnect() {
    Disconnect pkt{};
    pkt.header.size = sizeof(Disconnect);
    pkt.header.type = PacketType::DISCONNECT;
    sendPacket(&pkt, sizeof(pkt));
}

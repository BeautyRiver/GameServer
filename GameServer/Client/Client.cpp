#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <thread>
#include <chrono>
#include "Client.h"
#include "ClientPacketHandler.h"

// size 바이트가 모두 수신될 때까지 블로킹 
static bool recvAll(SOCKET sock, char* buf, int size) {
    int received = 0;
    while (received < size) {
        int ret = recv(sock, buf + received, size - received, 0);
        if (ret <= 0) return false;
        received += ret;
    }
    return true;
}

// WSAStartup → 소켓 생성 → connect
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

// 별도 쓰레드에서 실행 - 헤더 먼저 수신 후 바디 수신, 패킷 핸들러로 전달
void Client::recvLoop() {
    char buffer[1024];

    while (true) {
        // 헤더 수신
        if (!recvAll(sock, buffer, sizeof(PacketHeader))) break;

        PacketHeader* header   = reinterpret_cast<PacketHeader*>(buffer);
        uint16_t      bodySize = header->size - sizeof(PacketHeader);

        // 바디 수신 (바디가 있을 때만)
        if (bodySize > 0) {
            if (!recvAll(sock, buffer + sizeof(PacketHeader), bodySize)) break;
        }

        // 패킷 타입별 처리
        ClientPacketHandler::handle(this, header);
    }

    std::cout << "[INFO] Server disconnected\n";
}

// size 바이트가 모두 전송될 때까지 반복해서 전송
void Client::sendPacket(const void* data, uint16_t size) {
    const char* ptr  = reinterpret_cast<const char*>(data);
    int         sent = 0;
    while (sent < size) {
        int ret = send(sock, ptr + sent, size - sent, 0);
        if (ret == SOCKET_ERROR) return;
        sent += ret;
    }
}

// 메인루프
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
            // 메인 메뉴
            std::cout << "\n[MAIN MENU] 1.Room List  2.Quit\n> ";
            std::string input;
            std::getline(std::cin, input);
            if (input == "1") {
                sendRoomList();
                state = ClientState::WAITING;
            }
            else if (input == "2") {
                sendDisconnect();
                break;
            }
        }
        else if (state == ClientState::SELECTING_ROOM) {
            // 방 목록을 본 뒤 방 번호 선택
            std::cout << "\nEnter room number (0 = cancel): ";
            std::string input;
            std::getline(std::cin, input);

            uint32_t rid = (uint32_t)atoi(input.c_str());
            if (rid == 0) {
                state = ClientState::LOGGED_IN;   // 취소 → 메인 메뉴
            }
            else {
                sendRoomEnter(rid);
                state = ClientState::WAITING;
            }
        }
        else if (state == ClientState::IN_ROOM) {
            // 방 메뉴
            std::cout << "\n[ROOM MENU] 1.Chat  2.Move  3.Attack  4.Players  5.Quit\n> ";
            std::string input;
            std::getline(std::cin, input);

            // 1.채팅
            if (input == "1") {
                std::cout << "Message: ";
                std::string msg;
                std::getline(std::cin, msg);
                sendChat(msg);
            }
            // 2.움직임
            else if (input == "2") {
                int32_t x, y;
                std::cout << "x: "; std::cin >> x;
                std::cout << "y: "; std::cin >> y;
                std::cin.ignore();
                sendMove(x, y);
            }
            // 3.공격
            else if (input == "3") {
                uint32_t targetID;
                std::cout << "Target playerID: ";
                std::cin >> targetID;
                std::cin.ignore();
                sendAttack(targetID);
            }
            // 4.플레이어 목록 보기
            else if (input == "4") {
                sendRoomPlayersReq();         // 단순 목록 확인용
                state = ClientState::WAITING;
            }
            // 5.나가기
            else if (input == "5") {
                sendDisconnect();
                break;
            }
        }
    }

    closesocket(sock);
    WSACleanup();
}

// 패킷 전송 헬퍼들
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

void Client::sendRoomList() {
    RoomListReq pkt{};
    pkt.header.size = sizeof(RoomListReq);
    pkt.header.type = PacketType::ROOM_LIST_REQ;
    sendPacket(&pkt, sizeof(pkt));
}

void Client::sendRoomEnter(uint32_t roomID) {
    RoomEnterReq pkt{};
    pkt.header.size = sizeof(RoomEnterReq);
    pkt.header.type = PacketType::ROOM_ENTER_REQ;
    pkt.roomID = roomID;
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

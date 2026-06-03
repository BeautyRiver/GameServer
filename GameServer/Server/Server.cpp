#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <algorithm>
#include "Server.h"

void Server::start(int port) {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "[ERROR] WSAStartup failed\n";
        return;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "[ERROR] socket() failed\n";
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port        = htons(port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "[ERROR] bind() failed\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "[ERROR] listen() failed\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "[INFO] Server started on port " << port << "\n";

    initRooms(); // 방 초기화
    acceptLoop();
}


void Server::acceptLoop() {
    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "[ERROR] accept() failed\n";
            continue;
        }
        // 새 세션 동적 할당 및 초기화
        ClientSession* session  = new ClientSession();
        session->socket         = clientSocket;
        session->server         = this;
        session->isLoggedIn     = false;

        size_t count;
        {
            std::lock_guard<std::mutex> lock(sessionMutex);
            sessions.push_back(session);
            count = sessions.size();
        }

        std::cout << "[INFO] Client connected, total: " << count << "\n";

        // 클라이언트 수신 루프를 별도 스레드로 실행
        std::thread t(&ClientSession::recvLoop, session);
        t.detach();
    }
}

void Server::initRooms() {
    for (int i = 0; i < ROOM_COUNT; i++) {
        rooms[i].roomID = i + 1;
        rooms[i].capacity = ROOM_CAPACITY;
        sprintf_s(rooms[i].name, sizeof(rooms[i].name), "Room_%d", i + 1);
    }
}

// roomID로 방을 찾아 포인터 반환 (없으면 nullptr)
Room* Server::getRoom(uint32_t roomID) {
    for (Room& r : rooms) {
        if (r.roomID == roomID)
            return &r;
    }
    return nullptr;
}

// 세션 소켓 닫기
void Server::removeSession(ClientSession* session) {

    // 방에 들어가 있었다면 방에서도 제거
    if (session->roomID != 0) {
        Room* room = getRoom(session->roomID);
        if (room) room->leave(session);
    }

    if (session->socket != INVALID_SOCKET) {
        closesocket(session->socket);
        session->socket = INVALID_SOCKET;
    }

    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto it = std::find(sessions.begin(), sessions.end(), session);
        if (it != sessions.end())
            sessions.erase(it);
    }

    std::cout << "[INFO] Disconnected player_" << session->player.playerID
              << " (" << session->player.nickname << ")\n";

    delete session;
}

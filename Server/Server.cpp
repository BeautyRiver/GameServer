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

    acceptLoop();
}

void Server::acceptLoop() {
    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "[ERROR] accept() failed\n";
            continue;
        }

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

        std::thread t(&ClientSession::recvLoop, session);
        t.detach();
    }
}

void Server::broadcast(const void* data, uint16_t size) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    for (ClientSession* s : sessions) {
        if (s->isLoggedIn)
            s->sendPacket(data, size);
    }
}

void Server::removeSession(ClientSession* session) {
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

#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include "Server.h"

void Server::start(int port) {

    // WSAStartup
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup 실패" << std::endl;
        return;
    }

    // create socket
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "소켓 생성 실패" << std::endl;
        WSACleanup();
        return;
    }

    // bind
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "bind 실패" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    // listen
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "listen 실패" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "서버 시작 - 포트: " << port << std::endl;

    // accept loop
    acceptLoop();
}

void Server::acceptLoop() {
    while (true) {
        // 클라이언트 접속 대기
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "accept 실패" << std::endl;
            continue;
        }

        // 세션 ㅐㅅㅇ성
        ClientSession* session = new ClientSession();
        session->socket = clientSocket;
        session->isLoggedIn = false;

        // sessions에 추가 (mutex로 공유자원 관리)
        {
            std::lock_guard<std::mutex> lock(sessionMutex);
            sessions.push_back(session);
        }

        std::cout << "클라이언트 접속 - 현재 " << sessions.size() << "명" << std::endl;

        // 스레드 생성 → recvLoop 실행
        std::thread t(&ClientSession::recvLoop, session);
        t.detach();
    }
}

void Server::broadcast(const void* data, uint16_t size)
{
}

void Server::removeSession(ClientSession* session)
{
}

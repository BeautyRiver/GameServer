#include <iostream>
#include <cstring>
#include "ClientSession.h"
#include "Packet.h"
#include "ServerPacketHandler.h"
#include "Server.h"

static bool recvAll(SOCKET sock, char* buf, int size) {
    int received = 0;
    while (received < size) {
        int ret = recv(sock, buf + received, size - received, 0);
        if (ret <= 0) return false; 
        received += ret;
    }
    return true;
}

void ClientSession::recvLoop() {
    char buffer[1024];

    while (true) {
        if (!recvAll(socket, buffer, sizeof(PacketHeader)))
            break;

        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

        uint16_t bodySize = header->size - sizeof(PacketHeader);
        if (bodySize > 0) {
            if (!recvAll(socket, buffer + sizeof(PacketHeader), bodySize))
                break;
        }

        ServerPacketHandler::handle(this, header, server);
    }

    server->removeSession(this);
}

void ClientSession::sendPacket(const void* data, uint16_t size) {
    const char* ptr  = reinterpret_cast<const char*>(data);
    int         sent = 0;

    while (sent < size) {
        int ret = send(socket, ptr + sent, size - sent, 0);
        if (ret == SOCKET_ERROR) {
            std::cout << "[ERROR] send() failed, playerID: " << player.playerID << "\n";
            return;
        }
        sent += ret;
    }
}

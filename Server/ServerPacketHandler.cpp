#include <iostream>
#include <mutex>
#include "ServerPacketHandler.h"
#include "Server.h"

void ServerPacketHandler::handle(ClientSession* session, PacketHeader* header, Server* server) {
    switch (header->type) {
    case PacketType::LOGIN_REQ:
        onLogin(session, reinterpret_cast<LoginReq*>(header), server);
        break;
    case PacketType::CHAT_REQ:
        onChat(session, reinterpret_cast<ChatReq*>(header), server);
        break;
    case PacketType::MOVE_REQ:
        onMove(session, reinterpret_cast<MoveReq*>(header), server);
        break;
    case PacketType::ATTACK_REQ:
        onAttack(session, reinterpret_cast<AttackReq*>(header), server);
        break;
    case PacketType::ROOM_ENTER_REQ:
        onRoomEnter(session, reinterpret_cast<RoomEnterReq*>(header), server);
        break;
    case PacketType::ROOM_PLAYERS_REQ:
        onRoomPlayers(session, reinterpret_cast<RoomPlayersReq*>(header), server);
        break;
    case PacketType::DISCONNECT:
        onDisconnect(session);
        break;
    default:
        std::cout << "[WARN] Unknown packet type: " << static_cast<int>(header->type) << "\n";
        break;
    }
}

void ServerPacketHandler::onLogin(ClientSession* session, LoginReq* pkt, Server* server) {
    if (session->isLoggedIn) return;

    {
        std::lock_guard<std::mutex> lock(server->sessionMutex);
        session->player.playerID = server->nextPlayerID++;
    }

    session->player.nickname = pkt->nickname;
    session->isLoggedIn = true;

    std::cout << "[LOGIN] player_" << session->player.playerID
              << " (" << session->player.nickname << ")\n";

    LoginRes res{};
    res.header.size = sizeof(LoginRes);
    res.header.type = PacketType::LOGIN_RES;
    res.playerID    = session->player.playerID;
    res.success     = true;
    session->sendPacket(&res, sizeof(res));
}

void ServerPacketHandler::onChat(ClientSession* session, ChatReq* pkt, Server* server) {
    if (!session->isLoggedIn || !session->inRoom) return;

    ChatBroadcast bcast{};
    bcast.header.size = sizeof(ChatBroadcast);
    bcast.header.type = PacketType::CHAT_BROADCAST;
    bcast.playerID    = session->player.playerID;
    strncpy_s(bcast.nickname, sizeof(bcast.nickname), session->player.nickname.c_str(), _TRUNCATE);
    strncpy_s(bcast.message,  sizeof(bcast.message),  pkt->message,                    _TRUNCATE);

    std::cout << "[CHAT] player_" << session->player.playerID
              << ": " << pkt->message << "\n";

    server->broadcast(&bcast, sizeof(bcast));
}

void ServerPacketHandler::onMove(ClientSession* session, MoveReq* pkt, Server* server) {
    if (!session->isLoggedIn || !session->inRoom) return;

    session->player.pos.x = pkt->x;
    session->player.pos.y = pkt->y;

    MoveNotify notify{};
    notify.header.size = sizeof(MoveNotify);
    notify.header.type = PacketType::MOVE_NOTIFY;
    notify.playerID    = session->player.playerID;
    notify.x           = pkt->x;
    notify.y           = pkt->y;

    std::cout << "[MOVE] player_" << session->player.playerID
              << " -> (" << pkt->x << ", " << pkt->y << ")\n";

    server->broadcast(&notify, sizeof(notify));
}

void ServerPacketHandler::onAttack(ClientSession* session, AttackReq* pkt, Server* server) {
    if (!session->isLoggedIn || !session->inRoom) return;

    const int32_t  DAMAGE = 10;
    ClientSession* target = nullptr;
    AttackNotify   notify{};

    {
        std::lock_guard<std::mutex> lock(server->sessionMutex);

        for (ClientSession* s : server->sessions) {
            if (s->player.playerID == pkt->targetID) {
                target = s;
                break;
            }
        }

        if (!target || !target->player.isAlive) return;

        target->player.hp -= DAMAGE;
        if (target->player.hp <= 0) {
            target->player.hp      = 0;
            target->player.isAlive = false;
        }

        notify.targetHP = target->player.hp;
        notify.isDead   = !target->player.isAlive;
    }

    notify.header.size = sizeof(AttackNotify);
    notify.header.type = PacketType::ATTACK_NOTIFY;
    notify.attackerID  = session->player.playerID;
    notify.targetID    = pkt->targetID;
    notify.damage      = DAMAGE;

    std::cout << "[ATTACK] player_" << session->player.playerID
              << " -> player_" << pkt->targetID
              << "  | damage dealt: " << DAMAGE
              << "  | remaining hp: " << notify.targetHP;
    if (notify.isDead)
        std::cout << "  [DEAD]";
    std::cout << "\n";

    server->broadcast(&notify, sizeof(notify));
}

void ServerPacketHandler::onRoomEnter(ClientSession* session, RoomEnterReq* pkt, Server* server) {
    if (!session->isLoggedIn || session->inRoom) return;

    uint32_t count = 0;
    {
        std::lock_guard<std::mutex> lock(server->sessionMutex);
        session->inRoom = true;
        for (ClientSession* s : server->sessions) {
            if (s->inRoom) count++;
        }
    }

    std::cout << "[ROOM ENTER] player_" << session->player.playerID
              << " (" << session->player.nickname << ")\n";

    RoomEnterRes res{};
    res.header.size    = sizeof(RoomEnterRes);
    res.header.type    = PacketType::ROOM_ENTER_RES;
    res.success        = true;
    res.currentPlayers = count;
    session->sendPacket(&res, sizeof(res));
}

void ServerPacketHandler::onRoomPlayers(ClientSession* session, RoomPlayersReq* pkt, Server* server) {
    if (!session->isLoggedIn || !session->inRoom) return;

    RoomPlayersRes res{};
    res.header.size = sizeof(RoomPlayersRes);
    res.header.type = PacketType::ROOM_PLAYERS_RES;
    res.count       = 0;

    {
        std::lock_guard<std::mutex> lock(server->sessionMutex);
        for (ClientSession* s : server->sessions) {
            if (!s->inRoom || res.count >= 16) continue;

            PlayerInfo& info  = res.players[res.count++];
            info.playerID     = s->player.playerID;
            info.hp           = s->player.hp;
            info.x            = s->player.pos.x;
            info.y            = s->player.pos.y;
            info.isAlive      = s->player.isAlive;
            strncpy_s(info.nickname, sizeof(info.nickname),
                      s->player.nickname.c_str(), _TRUNCATE);
        }
    }

    session->sendPacket(&res, sizeof(res));
}

void ServerPacketHandler::onDisconnect(ClientSession* session) {
    if (session->socket != INVALID_SOCKET) {
        closesocket(session->socket);
        session->socket = INVALID_SOCKET;
    }
}

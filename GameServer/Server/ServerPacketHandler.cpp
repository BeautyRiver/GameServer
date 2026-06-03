#include <iostream>
#include <mutex>
#include "ServerPacketHandler.h"
#include "Server.h"

// 패킷 타입에 따라 알맞은 핸들러 호출
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
    case PacketType::ROOM_LIST_REQ:
        onRoomList(session, reinterpret_cast<RoomListReq*>(header), server);
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

// 클라이언트에게 에러 응답 패킷 전송 + 서버 로그
void ServerPacketHandler::sendError(ClientSession* session, ErrorCode code, const char* detail) {
    ErrorRes res{};
    res.header.size = sizeof(ErrorRes);
    res.header.type = PacketType::ERROR_RES;
    res.errorCode = code;
    strncpy_s(res.detail, sizeof(res.detail), detail, _TRUNCATE);
    session->sendPacket(&res, sizeof(res));

    std::cout << "[ERROR -> player_" << session->player.playerID
        << "] code=" << (int)code << " (" << detail << ")\n";
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
    if (!session->isLoggedIn) {
        sendError(session, ErrorCode::NOT_LOGGED_IN, "login required");
        return;
    }
    Room* room = server->getRoom(session->roomID);
    if (!room) {                                   // 방에 없으면 채팅 불가
        sendError(session, ErrorCode::ROOM_NOT_FOUND, "not in a room");
        return;
    }

    ChatBroadcast bcast{};
    bcast.header.size = sizeof(ChatBroadcast);
    bcast.header.type = PacketType::CHAT_BROADCAST;
    bcast.playerID    = session->player.playerID;
    strncpy_s(bcast.nickname, sizeof(bcast.nickname), session->player.nickname.c_str(), _TRUNCATE);
    strncpy_s(bcast.message,  sizeof(bcast.message),  pkt->message,                    _TRUNCATE);

    std::cout << "[CHAT][room_" << room->roomID << "] player_"
              << session->player.playerID << ": " << pkt->message << "\n";

    room->broadcast(&bcast, sizeof(bcast));        // 방 멤버에게만
}

void ServerPacketHandler::onMove(ClientSession* session, MoveReq* pkt, Server* server) {
    if (!session->isLoggedIn) {
        sendError(session, ErrorCode::NOT_LOGGED_IN, "login required");
        return;
    }
    Room* room = server->getRoom(session->roomID);
    if (!room) {
        sendError(session, ErrorCode::ROOM_NOT_FOUND, "not in a room");
        return;
    }
    // 맵 밖으로 이동 요청 검사
    if (pkt->x < MAP_MIN || pkt->x > MAP_MAX ||
        pkt->y < MAP_MIN || pkt->y > MAP_MAX) {
        sendError(session, ErrorCode::OUT_OF_BOUNDS, "out of map bounds");
        return;
    }

    session->player.pos.x = pkt->x;
    session->player.pos.y = pkt->y;

    MoveNotify notify{};
    notify.header.size = sizeof(MoveNotify);
    notify.header.type = PacketType::MOVE_NOTIFY;
    notify.playerID    = session->player.playerID;
    notify.x           = pkt->x;
    notify.y           = pkt->y;

    std::cout << "[MOVE][room_" << room->roomID << "] player_"
              << session->player.playerID << " -> (" << pkt->x << ", " << pkt->y << ")\n";

    room->broadcast(&notify, sizeof(notify));
}

void ServerPacketHandler::onAttack(ClientSession* session, AttackReq* pkt, Server* server) {
    if (!session->isLoggedIn) {
        sendError(session, ErrorCode::NOT_LOGGED_IN, "login required");
        return;
    }
    Room* room = server->getRoom(session->roomID);
    if (!room) {
        sendError(session, ErrorCode::ROOM_NOT_FOUND, "not in a room");
        return;
    }
    if (!session->player.isAlive) {                          // 사망한 플레이어의 공격
        sendError(session, ErrorCode::ATTACKER_DEAD, "you are dead");
        return;
    }

    const int32_t  DAMAGE = 10;
    ClientSession* target = nullptr;
    ErrorCode      err    = ErrorCode::NONE;
    AttackNotify   notify{};

    {
        std::lock_guard<std::mutex> lock(server->sessionMutex);

        for (ClientSession* s : server->sessions) {          // 전체에서 대상 검색
            if (s->isLoggedIn && s->player.playerID == pkt->targetID) {
                target = s;
                break;
            }
        }

        if (!target) {
            err = ErrorCode::TARGET_NOT_FOUND;               // 어디에도 없음
        }
        else if (target->roomID != session->roomID) {
            err    = ErrorCode::TARGET_NOT_IN_ROOM;          // 다른 방
            target = nullptr;
        }
        else if (!target->player.isAlive) {
            target = nullptr;                                // 이미 죽은 대상 -> 조용히 무시
        }
        else {
            target->player.hp -= DAMAGE;
            if (target->player.hp <= 0) {
                target->player.hp      = 0;
                target->player.isAlive = false;
            }
            notify.targetHP = target->player.hp;
            notify.isDead   = !target->player.isAlive;
        }
    }

    if (err != ErrorCode::NONE) {
        sendError(session, err, "attack failed");
        return;
    }
    if (!target) return;                                     // 무시 케이스

    notify.header.size = sizeof(AttackNotify);
    notify.header.type = PacketType::ATTACK_NOTIFY;
    notify.attackerID  = session->player.playerID;
    notify.targetID    = pkt->targetID;
    notify.damage      = DAMAGE;

    std::cout << "[ATTACK][room_" << room->roomID << "] player_"
              << session->player.playerID << " -> player_" << pkt->targetID
              << "  | dmg: " << DAMAGE << "  | hp: " << notify.targetHP;
    if (notify.isDead) std::cout << "  [DEAD]";
    std::cout << "\n";

    room->broadcast(&notify, sizeof(notify));
}

void ServerPacketHandler::onRoomEnter(ClientSession* session, RoomEnterReq* pkt, Server* server) {
    if (!session->isLoggedIn) {
        sendError(session, ErrorCode::NOT_LOGGED_IN, "login required");
        return;
    }
    if (session->roomID != 0) {        // 이미 어떤 방에 있으면 무시
        return;
    }

    Room* room = server->getRoom(pkt->roomID);
    if (!room) {                                          // 존재하지 않는 방
        sendError(session, ErrorCode::ROOM_NOT_FOUND, "no such room");
        return;
    }

    if (!room->tryEnter(session)) {                       // 방 정원 초과
        sendError(session, ErrorCode::ROOM_FULL, "room is full");
        return;
    }

    std::cout << "[ROOM ENTER] player_" << session->player.playerID
        << " -> room_" << room->roomID << "\n";

    RoomEnterRes res{};
    res.header.size = sizeof(RoomEnterRes);
    res.header.type = PacketType::ROOM_ENTER_RES;
    res.success = true;
    res.roomID = room->roomID;
    res.currentPlayers = room->playerCount();
    session->sendPacket(&res, sizeof(res));
}

void ServerPacketHandler::onRoomList(ClientSession* session, RoomListReq* pkt, Server* server) {
    if (!session->isLoggedIn) {
        sendError(session, ErrorCode::NOT_LOGGED_IN, "login required");
        return;
    }

    RoomListRes res{};
    res.header.size = sizeof(RoomListRes);
    res.header.type = PacketType::ROOM_LIST_RES;
    res.count = 0;

    for (Room& r : server->rooms) {              // 서버의 방 배열 순회
        if (res.count >= 8) break;                // 패킷 한계
        RoomInfo& info = res.rooms[res.count++];
        info.roomID = r.roomID;
        info.currentPlayers = r.playerCount();
        info.capacity = r.capacity;
        strncpy_s(info.name, sizeof(info.name), r.name, _TRUNCATE);   // Room.name → RoomInfo.name 복사
    }

    session->sendPacket(&res, sizeof(res));
}

void ServerPacketHandler::onRoomPlayers(ClientSession* session, RoomPlayersReq* pkt, Server* server) {
    if (!session->isLoggedIn) {
        sendError(session, ErrorCode::NOT_LOGGED_IN, "login required");
        return;
    }
    Room* room = server->getRoom(session->roomID);
    if (!room) {
        sendError(session, ErrorCode::ROOM_NOT_FOUND, "not in a room");
        return;
    }

    RoomPlayersRes res{};
    res.header.size = sizeof(RoomPlayersRes);
    res.header.type = PacketType::ROOM_PLAYERS_RES;
    res.count       = 0;

    {
        std::lock_guard<std::mutex> lock(room->roomMutex);   // 이 방 멤버만 순회
        for (ClientSession* s : room->members) {
            if (res.count >= 16) break;

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

#include <iostream>
#include "ClientPacketHandler.h"
#include "Client.h"

// 패킷 타입에 따라 알맞은 핸들러 호출
void ClientPacketHandler::handle(Client* client, PacketHeader* header) {
    switch (header->type) {
    case PacketType::LOGIN_RES:
        onLoginRes(client, reinterpret_cast<LoginRes*>(header));
        break;
    case PacketType::ROOM_LIST_RES:
        onRoomListRes(client, reinterpret_cast<RoomListRes*>(header));
        break;
    case PacketType::ROOM_ENTER_RES:
        onRoomEnterRes(client, reinterpret_cast<RoomEnterRes*>(header));
        break;
    case PacketType::ROOM_PLAYERS_RES:
        onRoomPlayersRes(client, reinterpret_cast<RoomPlayersRes*>(header));
        break;
    case PacketType::CHAT_BROADCAST:
        onChatBroadcast(client, reinterpret_cast<ChatBroadcast*>(header));
        break;
    case PacketType::MOVE_NOTIFY:
        onMoveNotify(client, reinterpret_cast<MoveNotify*>(header));
        break;
    case PacketType::ATTACK_NOTIFY:
        onAttackNotify(client, reinterpret_cast<AttackNotify*>(header));
        break;
    case PacketType::ERROR_RES:
        onError(client, reinterpret_cast<ErrorRes*>(header));
        break;
    default:
        std::cout << "[WARN] Unknown packet type: " << static_cast<int>(header->type) << "\n";
        break;
    }
}

void ClientPacketHandler::onLoginRes(Client* client, LoginRes* pkt) {
    if (pkt->success) {
        client->player.playerID = pkt->playerID;
        client->state           = ClientState::LOGGED_IN;
        system("cls");
        std::cout << "[CLIENT INFO] ID: player_" << client->player.playerID
                  << " / NAME: " << client->player.nickname << "\n";
    }
    else {
        std::cout << "\n[LOGIN FAIL]\n";
    }
}

void ClientPacketHandler::onRoomListRes(Client* client, RoomListRes* pkt) {
    std::cout << "\n[ROOM LIST]\n";
    std::cout << "------------------------------------\n";
    for (uint32_t i = 0; i < pkt->count; i++) {
        RoomInfo& r = pkt->rooms[i];
        std::cout << "  [" << r.roomID << "] " << r.name
            << "   (" << r.currentPlayers << "/" << r.capacity << ")\n";
    }
    std::cout << "------------------------------------\n";
    client->state = ClientState::SELECTING_ROOM;   // 방 번호 입력 단계로
}

void ClientPacketHandler::onRoomEnterRes(Client* client, RoomEnterRes* pkt) {
    if (pkt->success) {
        client->roomID = pkt->roomID;            // 입장한 방 기억
        client->state = ClientState::IN_ROOM;
        std::cout << "\n[ROOM ENTER OK] room_" << pkt->roomID
            << "   | players: " << pkt->currentPlayers << "\n";
    }
    else {
        std::cout << "\n[ROOM ENTER FAIL]\n";
    }
}

void ClientPacketHandler::onRoomPlayersRes(Client* client, RoomPlayersRes* pkt) {
    std::cout << "\n[PLAYERS IN ROOM]\n";
    std::cout << "------------------------------------\n";
    for (uint32_t i = 0; i < pkt->count; i++) {
        PlayerInfo& p = pkt->players[i];

        std::cout << "  player_" << p.playerID;
        if (p.playerID == client->player.playerID)
            std::cout << "(Me)";
        else
            std::cout << "    ";
        std::cout << " | " << p.nickname
            << " | HP: " << p.hp
            << " | pos: (" << p.x << ", " << p.y << ")";
        if (!p.isAlive) std::cout << " [DEAD]";
        std::cout << "\n";
    }
    std::cout << "------------------------------------\n";
    client->state = ClientState::IN_ROOM;
}

void ClientPacketHandler::onChatBroadcast(Client* client, ChatBroadcast* pkt) {
    std::cout << "\n[CHAT] player_" << pkt->playerID
              << " (" << pkt->nickname << "): " << pkt->message << "\n";
}

void ClientPacketHandler::onMoveNotify(Client* client, MoveNotify* pkt) {
    std::cout << "\n[MOVE] player_" << pkt->playerID
              << " -> (" << pkt->x << ", " << pkt->y << ")\n";
}

void ClientPacketHandler::onAttackNotify(Client* client, AttackNotify* pkt) {
    std::cout << "\n[ATTACK] player_" << pkt->attackerID
              << " -> player_" << pkt->targetID
              << "  | damage dealt: " << pkt->damage
              << "  | remaining hp: " << pkt->targetHP;
    if (pkt->isDead)
        std::cout << "  [DEAD]";
    std::cout << "\n";
}

void ClientPacketHandler::onError(Client* client, ErrorRes* pkt) {
    std::cout << "\n[ERROR] code=" << (int)pkt->errorCode
        << " : " << pkt->detail << "\n";

    // 응답을 기다리던 중(WAITING)이었다면 알맞은 메뉴로 되돌림
    if (client->state == ClientState::WAITING) {
        client->state = (client->roomID != 0)
            ? ClientState::IN_ROOM       // 방 안이었으면 방 메뉴로
            : ClientState::LOGGED_IN;    // 밖이었으면 메인 메뉴로
    }
}

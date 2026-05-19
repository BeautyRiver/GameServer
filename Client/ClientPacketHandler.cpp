#include <iostream>
#include "ClientPacketHandler.h"
#include "Client.h"

void ClientPacketHandler::handle(Client* client, PacketHeader* header) {
    switch (header->type) {
    case PacketType::LOGIN_RES:
        onLoginRes(client, reinterpret_cast<LoginRes*>(header));
        break;
    case PacketType::ROOM_ENTER_RES:
        onRoomEnterRes(client, reinterpret_cast<RoomEnterRes*>(header));
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
    case PacketType::ROOM_PLAYERS_RES:
        onRoomPlayersRes(client, reinterpret_cast<RoomPlayersRes*>(header));
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

void ClientPacketHandler::onRoomEnterRes(Client* client, RoomEnterRes* pkt) {
    if (pkt->success) {
        client->state = ClientState::IN_ROOM;
        std::cout << "\n[ROOM ENTER OK] Current Players: " << pkt->currentPlayers << "\n";
    }
    else {
        std::cout << "\n[ROOM ENTER FAIL]\n";
    }
}

void ClientPacketHandler::onChatBroadcast(Client* client, ChatBroadcast* pkt) {
    std::cout << "\n[CHAT] player_" << pkt->playerID
              << " (" << pkt->nickname << "): " << pkt->message << "\n";
}

void ClientPacketHandler::onMoveNotify(Client* client, MoveNotify* pkt) {
    std::cout << "\n[MOVE] player_" << pkt->playerID
              << " -> (" << pkt->x << ", " << pkt->y << ")\n";
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
    client->state = ClientState::SELECTING_TARGET;
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

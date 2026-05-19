#pragma once
#include "Packet.h"

class Client;

class ClientPacketHandler {
public:
    static void handle(Client* client, PacketHeader* header);

private:
    static void onLoginRes(Client* client, LoginRes* pkt);
    static void onRoomEnterRes(Client* client, RoomEnterRes* pkt);
    static void onChatBroadcast(Client* client, ChatBroadcast* pkt);
    static void onMoveNotify(Client* client, MoveNotify* pkt);
    static void onAttackNotify(Client* client, AttackNotify* pkt);
    static void onRoomPlayersRes(Client* client, RoomPlayersRes* pkt);
};

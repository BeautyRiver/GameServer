#pragma once
#include "ClientSession.h"
#include "Packet.h"
#include "Server.h"

class ServerPacketHandler {
public:
    static void handle(ClientSession* session, PacketHeader* header, Server* server);

private:
    static void onLogin(ClientSession* session, LoginReq* pkt, Server* server);
    static void onChat(ClientSession* session, ChatReq* pkt, Server* server);
    static void onMove(ClientSession* session, MoveReq* pkt, Server* server);
    static void onAttack(ClientSession* session, AttackReq* pkt, Server* server);
    static void onRoomEnter(ClientSession* session, RoomEnterReq* pkt, Server* server);
    static void onRoomPlayers(ClientSession* session, RoomPlayersReq* pkt, Server* server);
    static void onDisconnect(ClientSession* session);
};
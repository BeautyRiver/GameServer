#pragma once
#pragma pack(push, 1)

#include <stdint.h>

enum class PacketType : uint16_t {
    LOGIN_REQ       = 1,
    LOGIN_RES       = 2,
    CHAT_REQ        = 3,
    CHAT_BROADCAST  = 4,
    MOVE_REQ        = 5,
    MOVE_NOTIFY     = 6,
    ATTACK_REQ      = 7,
    ATTACK_NOTIFY   = 8,
    DISCONNECT      = 9,
    ROOM_ENTER_REQ      = 10,
    ROOM_ENTER_RES      = 11,
    ROOM_PLAYERS_REQ    = 12,
    ROOM_PLAYERS_RES    = 13
};

struct PacketHeader {
    uint16_t    size;
    PacketType  type;
};

// Client -> Server
struct LoginReq {
    PacketHeader header;
    char         nickname[32];
};

// Server -> Client
struct LoginRes {
    PacketHeader header;
    uint32_t     playerID;
    bool         success;
};

// Client -> Server
struct ChatReq {
    PacketHeader header;
    char         message[128];
};

// Server -> All
struct ChatBroadcast {
    PacketHeader header;
    uint32_t     playerID;
    char         nickname[32];
    char         message[128];
};

// Client -> Server
struct MoveReq {
    PacketHeader header;
    int32_t      x;
    int32_t      y;
};

// Server -> All
struct MoveNotify {
    PacketHeader header;
    uint32_t     playerID;
    int32_t      x;
    int32_t      y;
};

// Client -> Server
struct AttackReq {
    PacketHeader header;
    uint32_t     targetID;
};

// Server -> All
struct AttackNotify {
    PacketHeader header;
    uint32_t     attackerID;
    uint32_t     targetID;
    int32_t      damage;
    int32_t      targetHP;
    bool         isDead;
};

// Client -> Server
struct Disconnect {
    PacketHeader header;
};

// Client -> Server
struct RoomEnterReq {
    PacketHeader header;
};

// Server -> Client
struct RoomEnterRes {
    PacketHeader header;
    bool         success;
    uint32_t     currentPlayers;
};

// Client -> Server
struct RoomPlayersReq {
    PacketHeader header;
};

// per-player info
struct PlayerInfo {
    uint32_t playerID;
    char     nickname[32];
    int32_t  hp;
    int32_t  x;
    int32_t  y;
    bool     isAlive;
};

// Server -> Client
struct RoomPlayersRes {
    PacketHeader header;
    uint32_t     count;
    PlayerInfo   players[16];
};

#pragma pack(pop)

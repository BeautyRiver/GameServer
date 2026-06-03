#pragma once
#pragma pack(push, 1)

#include <stdint.h>

// 패킷 종류
enum class PacketType : uint16_t {
    LOGIN_REQ = 0,
    LOGIN_RES,
    CHAT_REQ,
    CHAT_BROADCAST,
    MOVE_REQ,
    MOVE_NOTIFY,
    ATTACK_REQ,
    ATTACK_NOTIFY,    
    ROOM_ENTER_REQ,
    ROOM_ENTER_RES,
    ROOM_LIST_REQ,
    ROOM_LIST_RES,
    ROOM_PLAYERS_REQ,
    ROOM_PLAYERS_RES,     
    DISCONNECT,
    ERROR_RES    
};

enum class ErrorCode : uint16_t {
    NONE = 0,
    NOT_LOGGED_IN,
    ROOM_NOT_FOUND,
    ROOM_FULL,
    OUT_OF_BOUNDS,
    TARGET_NOT_FOUND,
    TARGET_NOT_IN_ROOM,
    ATTACKER_DEAD
};

// 패킷 헤더
struct PacketHeader {
    uint16_t    size;
    PacketType  type;
};

// -----------------------------Info 구조체 관련-----------------------------
// 방 1개 정보
struct RoomInfo {
    uint32_t roomID;
    char     name[32];
    uint32_t currentPlayers;
    uint32_t capacity;
};

// 플레이어 1명 정보
struct PlayerInfo {
    uint32_t playerID;
    char     nickname[32];
    int32_t  hp;
    int32_t  x;
    int32_t  y;
    bool     isAlive;
};

// -----------------------------로그인 관련-----------------------------
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

// -----------------------------채팅 관련-----------------------------
// Client -> Server
struct ChatReq {
    PacketHeader header;
    char         message[128];
};

// -----------------------------Server -> All
struct ChatBroadcast {
    PacketHeader header;
    uint32_t     playerID;
    char         nickname[32];
    char         message[128];
};

// -----------------------------움직임 관련-----------------------------
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

// -----------------------------공격 관련-----------------------------
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

// -----------------------------방 관련-----------------------------
// Client -> Server
struct RoomEnterReq {
    PacketHeader header;
    uint32_t     roomID; // 입장할 방 번호
};

// Server -> Client
struct RoomEnterRes {
    PacketHeader header;
    bool         success;
    uint32_t     roomID; // 입장한 방 번호
    uint32_t     currentPlayers;
};

// Client -> Server
struct RoomListReq {
    PacketHeader header;
};

// Server -> Client 전체 방 목록 응답
struct RoomListRes {
    PacketHeader header;
    uint32_t     count;
    RoomInfo     rooms[8];  
};

// Client -> Server
struct RoomPlayersReq {
    PacketHeader header;
};

// Server -> Client
struct RoomPlayersRes {
    PacketHeader header;
    uint32_t     count;
    PlayerInfo   players[16];
};


// -----------------------------연결종료 & 에러 관련-----------------------------
// Client -> Server
struct Disconnect {
    PacketHeader header;
};


// 에러 응답
struct ErrorRes {
    PacketHeader header;
    ErrorCode    errorCode;
    char         detail[64];
};

#pragma pack(pop)

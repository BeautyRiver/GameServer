#pragma once
#pragma pack(push, 1)

#include <stdint.h>

// 패킷 타입 정의
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
};

// 공통 헤더
struct PacketHeader {
    uint16_t size;      // 전체 패킷 크기
    PacketType type;    // 패킷 타입
};

// 1. 로그인 요청 (Client → Server)
struct LoginReq {
    PacketHeader header;
    char nickname[32];
};

// 2. 로그인 응답 (Server → Client)
struct LoginRes {
    PacketHeader header;
    uint32_t playerID;  // 서버가 부여한 고유 ID
    bool     success;
};

// 3. 채팅 요청 (Client → Server)
struct ChatReq {
    PacketHeader header;
    char message[128];
};

// 4. 채팅 브로드캐스트 (Server → 전체)
struct ChatBroadcast {
    PacketHeader header;
    uint32_t playerID;
    char     nickname[32];
    char     message[128];
};

// 5. 이동 요청 (Client → Server)
struct MoveReq {
    PacketHeader header;
    int32_t x;
    int32_t y;
};

// 6. 이동 알림 (Server → 전체)
struct MoveNotify {
    PacketHeader header;
    uint32_t playerID;
    int32_t  x;
    int32_t  y;
};

// 7. 공격 요청 (Client → Server)
struct AttackReq {
    PacketHeader header;
    uint32_t targetID;
};

// 8. 공격 알림 (Server → 전체)
struct AttackNotify {
    PacketHeader header;
    uint32_t attackerID;
    uint32_t targetID;
    int32_t  damage;
    int32_t  targetHP;  // 공격 후 남은 HP
    bool     isDead;
};

// 9. 접속 종료 (Client → Server)
struct Disconnect {
    PacketHeader header;
    // 바디 없음
};


#pragma pack(pop)

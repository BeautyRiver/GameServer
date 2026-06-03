#pragma once
#include <vector>
#include <mutex>
#include <cstdint>

class ClientSession;

// 게임 설정 상수
constexpr int      ROOM_COUNT = 3;		 // 방 개수 고정
constexpr int      ROOM_CAPACITY = 4;    // 방별 정원
constexpr int32_t  MAP_MIN = 0;          // 맵 최소 좌표
constexpr int32_t  MAP_MAX = 100;        // 맵 최대 좌표

class Room {
public:
	uint32_t roomID = 0;
	char name[32] = {};
	int capacity = ROOM_CAPACITY;
	std::vector<ClientSession*> members; // 방 안의 플레이어들
	std::mutex roomMutex;				 // 방 전용 동기화

	bool tryEnter(ClientSession* session);				// 정원 확인 후 입장
	void leave(ClientSession* session);					// 방에서 제거
	void broadcast(const void* data, uint16_t size); 
	uint32_t playerCount();								// 현재 인원 수
};


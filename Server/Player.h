#pragma once
#include <cstdint>
#include <string>

struct Position {
	int32_t x;
	int32_t y;
};

class Player {
public:
	uint32_t playerId;
	std::string nickname;
	Position pos;
	int32_t hp;
	bool isAlive;
};


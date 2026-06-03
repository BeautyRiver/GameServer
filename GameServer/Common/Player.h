#pragma once
#include <cstdint>
#include <string>

struct Position {
    int32_t x = 0;
    int32_t y = 0;
};

class Player {
public:
    uint32_t    playerID = 0;
    std::string nickname;
    Position    pos;
    int32_t     hp      = 100;
    bool        isAlive = true;
};

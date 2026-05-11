#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "Player.h"
class ClientSession {
public:
	SOCKET socket;
	Player player;
	bool isLoggedIn;

	void recvLoop();
	void send(const void* data, uint16_t size);
};
#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "Player.h"

class Server; 

class ClientSession {
public:
	SOCKET  socket = INVALID_SOCKET;
	Server* server = nullptr;
	Player  player;
	bool    isLoggedIn = false;
	bool    inRoom     = false;

	void recvLoop();
	void sendPacket(const void* data, uint16_t size);
};
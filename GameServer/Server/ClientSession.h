#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "Player.h"

class Server; 


// 서버에 접속한 클라이언트 1명을 나타내는 세션
class ClientSession {
public:
	SOCKET  socket = INVALID_SOCKET; // 클라이언트 소켓
	Server* server = nullptr;		 // 소속된 서버	
	Player  player;					 // 세션 플레이어 정보  
	bool    isLoggedIn = false;		 // 로그인 완료 여부
	int32_t roomID = 0;

	void recvLoop();								  // 수신 전용 루프 (별도 스레드 실행) 
	void sendPacket(const void* data, uint16_t size); // 패킷 송신
};
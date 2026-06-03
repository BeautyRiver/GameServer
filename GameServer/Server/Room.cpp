#include "Room.h"
#include "ClientSession.h"

// 정원이 남아 있으면 입장시키고 true, 가득 찼으면 false
bool Room::tryEnter(ClientSession* session) {
    std::lock_guard<std::mutex> lock(roomMutex);

    if ((int)members.size() >= capacity)
        return false;                 // 정원 초과

    members.push_back(session);
    session->roomID = roomID;          // 세션에 현재 방 번호 기록
    return true;
}

// 방에서 해당 세션 제거
void Room::leave(ClientSession* session) {
    std::lock_guard<std::mutex> lock(roomMutex);

    for (auto it = members.begin(); it != members.end(); ++it) {
        if (*it == session) {
            members.erase(it);
            break;
        }
    }
    session->roomID = 0;               // 어느 방에도 없음
}

// 이 방 멤버 전원에게 패킷 전송
void Room::broadcast(const void* data, uint16_t size) {
    std::lock_guard<std::mutex> lock(roomMutex);
    for (ClientSession* s : members)
        s->sendPacket(data, size);
}

// 현재 인원 수 반환
uint32_t Room::playerCount() {
    std::lock_guard<std::mutex> lock(roomMutex);
    return (uint32_t)members.size();
}
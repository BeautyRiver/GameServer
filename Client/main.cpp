#include <iostream>
#include "Client.h"

int main() {
    Client client;
    if (!client.connectToServer("127.0.0.1", 9000))
        return 1;
    client.run();
    return 0;
}

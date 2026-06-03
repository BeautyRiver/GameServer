#include "Server.h"
#include <iostream>

int main()
{
    std::cout << R"(
 ____  _____ ______     _______ ____
/ ___||  ___||  _ \ \   / / ____|  _ \
\___ \| |_  | |_) \ \ / /|  _| | |_) |
 ___) | |___|  _ < \ V / | |___|  _ <
|____/|_____|_| \_\ \_/  |_____|_| \_\
)" << "\n\n";

    Server server;
    server.start(SERVER_PORT);
    return 0;
}

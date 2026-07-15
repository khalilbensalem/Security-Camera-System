/**
 * @file main.cpp
 * @brief Point d'entree du programme client.
 */
#include "TcpClient.h"

#include <iostream>

namespace {
constexpr const char* SERVER_IP = "192.168.7.2";
}  // namespace

int main() {
    Client::TcpClient client;

    if (!client.connectToServer(SERVER_IP)) {
        return 1;
    }

    client.close();

    return 0;
}
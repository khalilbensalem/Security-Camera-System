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

    // Envoi de GET_FRAME et reception de la reponse du serveur
    const auto response = client.sendAndReceive(Protocol::MessageCode::GetFrame);
    if (response && response.value() == Protocol::MessageCode::FrameHdr) {
        std::cout << "FRAME_HDR recu" << std::endl;
    }

    client.close();

    return 0;
}
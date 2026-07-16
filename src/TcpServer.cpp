/**
 * @file TcpServer.cpp
 * @brief Implementation de la classe TcpServer.
 */
#include "TcpServer.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace Server {

TcpServer::~TcpServer() {
    if (_listenFd >= 0) {
        close(_listenFd);
    }
}

bool TcpServer::init() {
    // Creation du socket TCP/IP
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0) {
        std::cerr << "Erreur socket() : " << std::strerror(errno) << std::endl;
        return false;
    }
    // Permet de relancer le serveur sans attendre la liberation du port
    const int opt = 1;
    setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(Protocol::PORT);

    if (bind(_listenFd, reinterpret_cast<sockaddr*>(&_serverAddr), sizeof(_serverAddr)) < 0) {
        std::cerr << "Erreur bind() : " << std::strerror(errno) << std::endl;
        return false;
    }

    if (listen(_listenFd, 1) < 0) {
        std::cerr << "Erreur listen() : " << std::strerror(errno) << std::endl;
        return false;
    }

    std::cout << "Serveur en attente de connexion sur le port " << Protocol::PORT << std::endl;
    return true;
}

std::optional<int> TcpServer::acceptClient() const {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    const auto clientFd = accept(_listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (clientFd < 0) {
        std::cerr << "Erreur accept() : " << std::strerror(errno) << std::endl;
        return std::nullopt;
    }

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
    std::cout << "Client connecte depuis " << ipStr << std::endl;

    return clientFd;
}

void TcpServer::handleClient(int clientFd) const {
    // Boucle tant que le client n'a pas demande l'arret (STOP)
    while (true) {
        uint8_t message = 0;
        const auto received = recv(clientFd, &message, sizeof(message), 0);
        if (received <= 0) {
            std::cerr << "Client deconnecte ou erreur recv()" << std::endl;
            return;
        }

        const auto code = static_cast<Protocol::MessageCode>(message);

        if (code == Protocol::MessageCode::GetFrame) {
            const auto response = static_cast<uint8_t>(Protocol::MessageCode::FrameHdr);
            send(clientFd, &response, sizeof(response), 0);
        } else if (code == Protocol::MessageCode::Stop) {
            const auto response = static_cast<uint8_t>(Protocol::MessageCode::StopAck);
            send(clientFd, &response, sizeof(response), 0);
            std::cout << "STOP recu, arret du serveur" << std::endl;
            return;
        }
    }
}

void TcpServer::run() {
    const auto clientFd = acceptClient();
    if (!clientFd) {
        return;
    }

    handleClient(clientFd.value());
    close(clientFd.value());
}

}  // namespace Server
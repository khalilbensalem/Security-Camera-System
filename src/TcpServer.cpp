/**
 * @file TcpServer.cpp
 * @brief Implementation de la connexion TCP/IP cote serveur.
 */
#include "TcpServer.h"
#include "Protocol.h"

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

    // Permet de relancer le serveur sans attendre la liberation du port pour eviter l'erreur "Address already in use"
    const int opt = 1;
    setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Association du socket a l'adresse locale et au port du protocole
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(Protocol::PORT);

    if (bind(_listenFd, reinterpret_cast<sockaddr*>(&_serverAddr), sizeof(_serverAddr)) < 0) {
        std::cerr << "Erreur bind() : " << std::strerror(errno) << std::endl;
        return false;
    }

    // Mise en ecoute des connexions entrantes
    if (listen(_listenFd, 1) < 0) {
        std::cerr << "Erreur listen() : " << std::strerror(errno) << std::endl;
        return false;
    }

    std::cout << "Serveur en attente de connexion sur le port " << Protocol::PORT << std::endl;
    return true;
}

std::optional<int> TcpServer::acceptClient() const {
    // Blocage jusqu'a ce qu'un client se connecte
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

void TcpServer::run() {
    const auto clientFd = acceptClient();
    if (!clientFd) {
        return;
    }

    // Connexion TCP/IP validee, on ferme
    close(clientFd.value());
}

}  // namespace Server
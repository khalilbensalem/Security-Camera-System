/**
 * @file TcpClient.cpp
 * @brief Implementation de la connexion TCP/IP cote client.
 */
#include "TcpClient.h"
#include "Protocol.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace Client {

TcpClient::~TcpClient() {
    close();
}

bool TcpClient::connectToServer(const std::string& serverIp) {
    // Creation du socket TCP/IP
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd < 0) {
        std::cerr << "Erreur socket() : " << std::strerror(errno) << std::endl;
        return false;
    }

    // Preparation de l'adresse du serveur (IP + port du protocole)
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Protocol::PORT);

    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Adresse IP invalide : " << serverIp << std::endl;
        return false;
    }

    // Etablissement de la connexion TCP/IP avec le serveur
    if (connect(_socketFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Erreur connect() : " << std::strerror(errno) << std::endl;
        return false;
    }

    std::cout << "Connecte au serveur " << serverIp << ":" << Protocol::PORT << std::endl;
    return true;
}

void TcpClient::close() {
    if (_socketFd >= 0) {
        ::close(_socketFd);
        _socketFd = -1;
    }
}

}  // namespace Client
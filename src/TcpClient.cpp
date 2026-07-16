/**
 * @file TcpClient.cpp
 * @brief Implementation de la classe TcpClient.
 */
#include "TcpClient.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace Client {

TcpClient::~TcpClient() { close(); }

bool TcpClient::connectToServer(const std::string &serverIp) {
  _socketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_socketFd < 0) {
    std::cerr << "Erreur socket() : " << std::strerror(errno) << std::endl;
    return false;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(Protocol::PORT);

  if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
    std::cerr << "Adresse IP invalide : " << serverIp << std::endl;
    return false;
  }

  if (connect(_socketFd, reinterpret_cast<sockaddr *>(&serverAddr),
              sizeof(serverAddr)) < 0) {
    std::cerr << "Erreur connect() : " << std::strerror(errno) << std::endl;
    return false;
  }

  // Timeout de reception, pour respecter la contrainte des 60 ms
  timeval timeout{};
  timeout.tv_sec = 0;
  timeout.tv_usec = Protocol::RECV_TIMEOUT_MS * 1000;
  setsockopt(_socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  std::cout << "Connecte au serveur " << serverIp << ":" << Protocol::PORT
            << std::endl;
  return true;
}

std::optional<Protocol::MessageCode>
TcpClient::sendAndReceive(Protocol::MessageCode message) {
  // Envoi du message au serveur
  const auto request = static_cast<uint8_t>(message);
  if (send(_socketFd, &request, sizeof(request), 0) <= 0) {
    std::cerr << "Erreur send() : " << std::strerror(errno) << std::endl;
    return std::nullopt;
  }

  // Attente de la reponse du serveur
  uint8_t response = 0;
  if (recv(_socketFd, &response, sizeof(response), 0) <= 0) {
    std::cerr << "Erreur recv() : " << std::strerror(errno) << std::endl;
    return std::nullopt;
  }

  return static_cast<Protocol::MessageCode>(response);
}

void TcpClient::close() {
  if (_socketFd >= 0) {
    ::close(_socketFd);
    _socketFd = -1;
  }
}

} // namespace Client
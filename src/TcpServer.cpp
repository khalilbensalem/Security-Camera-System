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
#include <vector>

namespace {

// Envoie exactement 'size' octets, meme si send() les livre en plusieurs
// morceaux (comportement normal des sockets TCP)
bool sendAll(int fd, const void *data, size_t size) {
  const auto *bytes = static_cast<const uint8_t *>(data);
  size_t sent = 0;
  while (sent < size) {
    const auto result = send(fd, bytes + sent, size - sent, 0);
    if (result <= 0) {
      return false;
    }
    sent += static_cast<size_t>(result);
  }
  return true;
}

} // namespace

namespace Server {

TcpServer::~TcpServer() {
  if (_listenFd >= 0) {
    close(_listenFd);
  }
}

bool TcpServer::init() {
  if (!_camera.init()) {
    return false;
  }

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

  if (bind(_listenFd, reinterpret_cast<sockaddr *>(&_serverAddr),
           sizeof(_serverAddr)) < 0) {
    std::cerr << "Erreur bind() : " << std::strerror(errno) << std::endl;
    return false;
  }

  if (listen(_listenFd, 1) < 0) {
    std::cerr << "Erreur listen() : " << std::strerror(errno) << std::endl;
    return false;
  }

  std::cout << "Serveur en attente de connexion sur le port " << Protocol::PORT
            << std::endl;
  return true;
}

std::optional<int> TcpServer::acceptClient() const {
  sockaddr_in clientAddr{};
  socklen_t clientLen = sizeof(clientAddr);

  const auto clientFd =
      accept(_listenFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
  if (clientFd < 0) {
    std::cerr << "Erreur accept() : " << std::strerror(errno) << std::endl;
    return std::nullopt;
  }

  // Timeout de reception, pour respecter la contrainte des 60 ms
  timeval timeout{};
  timeout.tv_sec = 0;
  timeout.tv_usec = Protocol::RECV_TIMEOUT_MS * 1000;
  setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  char ipStr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
  std::cout << "Client connecte depuis " << ipStr << std::endl;

  return clientFd;
}

void TcpServer::sendFrame(int clientFd) {
  // Capture d'une image depuis la camera
  const auto frame = _camera.captureFrame();
  if (!frame) {
    return;
  }

  // Compression JPEG avant transmission (contrainte de l'enonce)
  std::vector<uchar> jpegBuffer;
  cv::imencode(".jpg", frame.value(), jpegBuffer);

  ++_frameId;

  // En-tete : FRAME_HDR (1 octet), puis frame_id et jpeg_size en ordre
  // reseau (32 bits chacun), suivis des donnees JPEG elles-memes
  const auto header = static_cast<uint8_t>(Protocol::MessageCode::FrameHdr);
  const uint32_t frameIdNet = htonl(_frameId);
  const uint32_t jpegSizeNet = htonl(static_cast<uint32_t>(jpegBuffer.size()));

  sendAll(clientFd, &header, sizeof(header));
  sendAll(clientFd, &frameIdNet, sizeof(frameIdNet));
  sendAll(clientFd, &jpegSizeNet, sizeof(jpegSizeNet));
  sendAll(clientFd, jpegBuffer.data(), jpegBuffer.size());
}

void TcpServer::handleClient(int clientFd) {
  while (true) {
    uint8_t message = 0;
    const auto received = recv(clientFd, &message, sizeof(message), 0);

    if (received == 0) {
      // Le client a ferme la connexion (deconnexion normale ou tue)
      std::cout << "Client deconnecte" << std::endl;
      return;
    }

    if (received < 0) {
      // Timeout : aucun message recu durant ce cycle, on continue simplement
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        continue;
      }
      std::cerr << "Erreur recv() : " << std::strerror(errno) << std::endl;
      return;
    }

    const auto code = static_cast<Protocol::MessageCode>(message);

    if (code == Protocol::MessageCode::GetFrame) {
      sendFrame(clientFd);
    } else if (code == Protocol::MessageCode::Stop) {
      const auto response =
          static_cast<uint8_t>(Protocol::MessageCode::StopAck);
      send(clientFd, &response, sizeof(response), 0);
      std::cout << "Arret demande" << std::endl;
      return;
    }
  }
}

void TcpServer::run() {
  // Le serveur retourne toujours attendre un nouveau client, sauf apres un
  // STOP explicite
  while (true) {
    const auto clientFd = acceptClient();
    if (!clientFd) {
      continue;
    }

    handleClient(clientFd.value());
    close(clientFd.value());
  }
}

} // namespace Server
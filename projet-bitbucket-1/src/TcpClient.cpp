/**
 * @file TcpClient.cpp
 * @brief Implementation de la classe TcpClient.
 */
#include "TcpClient.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/select.h>
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

  configureRecvTimeout();

  std::cout << "Connecte au serveur " << serverIp << ":" << Protocol::PORT
            << std::endl;
  return true;
}

void TcpClient::configureRecvTimeout() {
  timeval recvTimeout{};
  recvTimeout.tv_sec = 0;
  recvTimeout.tv_usec = Protocol::RECV_TIMEOUT_MS * 1000;
  setsockopt(_socketFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout,
             sizeof(recvTimeout));
}

bool TcpClient::tryReconnect(const std::string &serverIp) {
  close(); // s'assure qu'aucun ancien descripteur ne traine

  _socketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_socketFd < 0) {
    return false;
  }

  // Socket non bloquant le temps du connect() : sur un lien mort (cable
  // debranche), un connect() bloquant peut prendre plusieurs secondes avant
  // d'echouer, ce qui empecherait de respecter la cadence de 500 ms entre
  // deux tentatives.
  const int flags = fcntl(_socketFd, F_GETFL, 0);
  fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK);

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(Protocol::PORT);
  if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
    close();
    return false;
  }

  const auto connectResult = connect(
      _socketFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));
  if (connectResult < 0 && errno != EINPROGRESS) {
    close();
    return false;
  }

  if (connectResult < 0) {
    // Connexion en cours (EINPROGRESS) : on attend au maximum
    // RECONNECT_ATTEMPT_TIMEOUT_MS pour savoir si elle aboutit.
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(_socketFd, &writeSet);
    timeval selectTimeout{};
    selectTimeout.tv_sec = 0;
    selectTimeout.tv_usec = Protocol::RECONNECT_ATTEMPT_TIMEOUT_MS * 1000;

    const auto selectResult =
        select(_socketFd + 1, nullptr, &writeSet, nullptr, &selectTimeout);
    if (selectResult <= 0) {
      close();
      return false;
    }

    int socketError = 0;
    socklen_t errorLen = sizeof(socketError);
    getsockopt(_socketFd, SOL_SOCKET, SO_ERROR, &socketError, &errorLen);
    if (socketError != 0) {
      close();
      return false;
    }
  }

  // Connexion etablie : restaure le mode bloquant et le timeout habituel.
  fcntl(_socketFd, F_SETFL, flags);
  configureRecvTimeout();

  std::cout << "Reconnecte au serveur " << serverIp << ":" << Protocol::PORT
            << std::endl;
  return true;
}

bool TcpClient::receiveAll(void *buffer, size_t size) {
  auto *ptr = static_cast<uint8_t *>(buffer);
  size_t received = 0;
  while (received < size) {
    const auto n = recv(_socketFd, ptr + received, size - received, 0);
    if (n <= 0) {
      return false;
    }
    received += static_cast<size_t>(n);
  }
  return true;
}

std::optional<Protocol::MessageCode>
TcpClient::sendAndReceive(Protocol::MessageCode message) {
  const auto code = static_cast<uint8_t>(message);
  if (send(_socketFd, &code, sizeof(code), 0) <= 0) {
    return std::nullopt;
  }

  uint8_t response = 0;
  if (recv(_socketFd, &response, sizeof(response), 0) <= 0) {
    return std::nullopt;
  }

  return static_cast<Protocol::MessageCode>(response);
}

bool TcpClient::requestStop() {
  // Le serveur (mono-thread, synchrone) peut mettre jusqu'a ~500ms a finir
  // le sendAll() d'une frame en cours, potentiellement suivi d'un second
  // sendAll() dans le meme sendFrame() (jusqu'a ~1000ms) avant de relire le
  // STOP, puis jusqu'a ~500ms de plus pour que le STOP_ACK reparte si le
  // reseau est encore degrade : jusqu'a ~1500ms au pire cas. On retente donc
  // plusieurs fois avec le timeout standard de 60ms par tentative, plutot
  // que d'allonger ce timeout.
  constexpr int STOP_MAX_ATTEMPTS = 30; // ~1800ms de budget total au pire cas
  for (int attempt = 1; attempt <= STOP_MAX_ATTEMPTS; ++attempt) {
    const auto response = sendAndReceive(Protocol::MessageCode::Stop);
    if (response && *response == Protocol::MessageCode::StopAck) {
      return true;
    }
  }
  return false;
}

std::optional<Frame> TcpClient::requestFrame() {
  const auto code = static_cast<uint8_t>(Protocol::MessageCode::GetFrame);
  if (send(_socketFd, &code, sizeof(code), 0) <= 0) {
    return std::nullopt;
  }

  uint8_t header = 0;
  if (!receiveAll(&header, sizeof(header)))
    return std::nullopt;

  const auto messageCode = static_cast<Protocol::MessageCode>(header);

  // Le frame_id est toujours transmis, peu importe l'etat rapporte par le
  // serveur (Normal, BUTTON_PRESS, NO_LIGHT ou SENSOR_ERROR).
  uint32_t frameIdNet = 0;
  if (!receiveAll(&frameIdNet, sizeof(frameIdNet)))
    return std::nullopt;
  const uint32_t frameId = ntohl(frameIdNet);

  if (messageCode == Protocol::MessageCode::NoLight) {
    return Frame{frameId, cv::Mat(), FrameStatus::NoLight};
  }
  if (messageCode == Protocol::MessageCode::SensorError) {
    return Frame{frameId, cv::Mat(), FrameStatus::SensorError};
  }
  if (messageCode != Protocol::MessageCode::FrameHdr &&
      messageCode != Protocol::MessageCode::ButtonPress) {
    return std::nullopt;
  }

  // FRAME_HDR et BUTTON_PRESS transmettent ensuite la taille puis les
  // donnees de l'image JPEG.
  uint32_t jpegSizeNet = 0;
  if (!receiveAll(&jpegSizeNet, sizeof(jpegSizeNet)))
    return std::nullopt;
  const uint32_t jpegSize = ntohl(jpegSizeNet);

  std::vector<uchar> jpegBuffer(jpegSize);
  if (!receiveAll(jpegBuffer.data(), jpegSize))
    return std::nullopt;

  const cv::Mat image = cv::imdecode(jpegBuffer, cv::IMREAD_COLOR);
  if (image.empty()) {
    std::cerr << "Erreur : decodage JPEG echoue" << std::endl;
    return std::nullopt;
  }

  const auto status = messageCode == Protocol::MessageCode::ButtonPress
                          ? FrameStatus::ButtonPress
                          : FrameStatus::Normal;
  return Frame{frameId, image, status};
}

void TcpClient::close() {
  if (_socketFd >= 0) {
    ::close(_socketFd);
    _socketFd = -1;
  }
}

} // namespace Client
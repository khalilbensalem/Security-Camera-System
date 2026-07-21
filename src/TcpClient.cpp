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

  timeval timeout{};
  timeout.tv_sec = 0;
  timeout.tv_usec = Protocol::RECV_TIMEOUT_MS * 1000;
  setsockopt(_socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  std::cout << "Connecte au serveur " << serverIp << ":" << Protocol::PORT
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

std::optional<Frame> TcpClient::requestFrame() {
  const auto code = static_cast<uint8_t>(Protocol::MessageCode::GetFrame);
  if (send(_socketFd, &code, sizeof(code), 0) <= 0) {
    return std::nullopt;
  }

  uint8_t header = 0;
  if (!receiveAll(&header, sizeof(header)))
    return std::nullopt;
  if (static_cast<Protocol::MessageCode>(header) !=
      Protocol::MessageCode::FrameHdr) {
    return std::nullopt;
  }

  uint32_t frameIdNet = 0;
  uint32_t jpegSizeNet = 0;
  if (!receiveAll(&frameIdNet, sizeof(frameIdNet)))
    return std::nullopt;
  if (!receiveAll(&jpegSizeNet, sizeof(jpegSizeNet)))
    return std::nullopt;

  const uint32_t frameId = ntohl(frameIdNet);
  const uint32_t jpegSize = ntohl(jpegSizeNet);

  std::vector<uchar> jpegBuffer(jpegSize);
  if (!receiveAll(jpegBuffer.data(), jpegSize))
    return std::nullopt;

  const cv::Mat image = cv::imdecode(jpegBuffer, cv::IMREAD_COLOR);
  if (image.empty()) {
    std::cerr << "Erreur : decodage JPEG echoue" << std::endl;
    return std::nullopt;
  }

  return Frame{frameId, image};
}

void TcpClient::close() {
  if (_socketFd >= 0) {
    ::close(_socketFd);
    _socketFd = -1;
  }
}

} // namespace Client
/**
 * @file TcpServer.cpp
 * @brief Implementation de la classe TcpServer.
 */
#include "TcpServer.h"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

/// Qualite JPEG (0-100). Reduite depuis la valeur par defaut d'OpenCV (95)
/// pour diminuer le temps d'encodage et la taille transmise sur le reseau,
/// afin de respecter le cycle de 30 ms impose par le protocole.
constexpr int JPEG_QUALITY = 85;

/// Sous ce seuil (intensite moyenne en niveaux de gris, 0-255), l'image
/// capturee est consideree comme sombre. Metrique simple (luminosite
/// moyenne de la scene) suffisante pour detecter une absence de lumiere ou
/// un cache devant l'objectif, sans le cout d'une analyse d'histogramme.
constexpr double IMAGE_DARK_THRESHOLD = 40.0;

/**
 * @brief Envoie exactement @p size octets sur le socket @p fd.
 *
 * Boucle sur send() tant que necessaire, meme si les donnees sont livrees
 * en plusieurs morceaux (comportement normal des sockets TCP).
 * @param fd Descripteur du socket cible.
 * @param data Pointeur vers les donnees a envoyer.
 * @param size Nombre d'octets a envoyer.
 * @return true si tous les octets ont ete envoyes, false en cas d'erreur.
 */
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
  // Ignore SIGPIPE : sinon, un send() vers un client deconnecte (CTRL+C)
  // tue le processus au lieu d'echouer simplement (EPIPE), deja gere par
  // sendAll().
  std::signal(SIGPIPE, SIG_IGN);

  if (!_camera.init()) {
    return false;
  }

  if (!_button.init("gpiochip1", 92)) {
    return false;
  }

  if (!_lightSensor.init()) {
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
  timeval recvTimeout{};
  recvTimeout.tv_sec = 0;
  recvTimeout.tv_usec = Protocol::RECV_TIMEOUT_MS * 1000;
  setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout,
             sizeof(recvTimeout));

  // Timeout d'envoi : sans cela, un send() vers un lien reseau mort (cable
  // debranche, sans FIN ni RST) peut bloquer indefiniment si le tampon noyau
  // se remplit, empechant le serveur de jamais revenir en attente d'un
  // nouveau client.
  timeval sendTimeout{};
  sendTimeout.tv_sec = Protocol::SEND_TIMEOUT_MS / 1000;
  sendTimeout.tv_usec = (Protocol::SEND_TIMEOUT_MS % 1000) * 1000;
  setsockopt(clientFd, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout,
             sizeof(sendTimeout));

  // Desactive l'algorithme de Nagle, qui ajouterait des dizaines de ms au
  // cycle de 30 ms en attendant de regrouper les petits messages
  const int noDelay = 1;
  setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));

  char ipStr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
  std::cout << "Client connecte depuis " << ipStr << std::endl;

  return clientFd;
}

TcpServer::SystemState
TcpServer::computeInstantState(const cv::Mat &frame) const {
  cv::Mat gray;
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
  const double meanBrightness = cv::mean(gray)[0];

  const bool imageIsDark = meanBrightness < IMAGE_DARK_THRESHOLD;
  const bool sensorIsDark = _lightSensor.isDark();

  if (sensorIsDark && imageIsDark) {
    return SystemState::NoLight;
  }
  if (sensorIsDark != imageIsDark) {
    return SystemState::SensorError;
  }
  return SystemState::Normal;
}

const char *TcpServer::stateName(SystemState state) {
  switch (state) {
  case SystemState::Normal:
    return "NORMAL";
  case SystemState::NoLight:
    return "NO_LIGHT";
  case SystemState::SensorError:
    return "SENSOR_ERROR";
  }
  return "?";
}

TcpServer::SystemState TcpServer::updateConfirmedState(SystemState instant) {
  if (instant == _pendingState) {
    const auto elapsed = std::chrono::steady_clock::now() - _pendingSince;
    if (elapsed >= Protocol::STATE_DEBOUNCE &&
        _pendingState != _confirmedState) {
      _confirmedState = _pendingState;
    }
  } else {
    _pendingState = instant;
    _pendingSince = std::chrono::steady_clock::now();
  }
  return _confirmedState;
}

bool TcpServer::sendFrame(int clientFd) {
  // Capture d'une image depuis la camera
  const auto frame = _camera.captureFrame();
  if (!frame) {
    // Echec de capture (camera), pas un probleme de connexion reseau.
    return true;
  }

  const SystemState state =
      updateConfirmedState(computeInstantState(frame.value()));

  ++_frameId;
  const uint32_t frameIdNet = htonl(_frameId);

  const bool buttonPressed = _button.consumePressEvent();

  if (state != SystemState::Normal) {
    const auto messageCode = state == SystemState::NoLight
                                 ? Protocol::MessageCode::NoLight
                                 : Protocol::MessageCode::SensorError;
    const auto header = static_cast<uint8_t>(messageCode);

    std::array<uint8_t, sizeof(header) + sizeof(frameIdNet)> headerBuffer{};
    size_t offset = 0;
    std::memcpy(headerBuffer.data() + offset, &header, sizeof(header));
    offset += sizeof(header);
    std::memcpy(headerBuffer.data() + offset, &frameIdNet, sizeof(frameIdNet));

    return sendAll(clientFd, headerBuffer.data(), headerBuffer.size());
  }

  // Compression JPEG avant transmission
  std::vector<uchar> jpegBuffer;
  const std::vector<int> jpegParams{cv::IMWRITE_JPEG_QUALITY, JPEG_QUALITY};
  cv::imencode(".jpg", frame.value(), jpegBuffer, jpegParams);

  const auto messageCode = buttonPressed ? Protocol::MessageCode::ButtonPress
                                         : Protocol::MessageCode::FrameHdr;
  const auto header = static_cast<uint8_t>(messageCode);
  const uint32_t jpegSizeNet = htonl(static_cast<uint32_t>(jpegBuffer.size()));

  std::array<uint8_t, sizeof(header) + sizeof(frameIdNet) + sizeof(jpegSizeNet)>
      headerBuffer{};
  size_t offset = 0;
  std::memcpy(headerBuffer.data() + offset, &header, sizeof(header));
  offset += sizeof(header);
  std::memcpy(headerBuffer.data() + offset, &frameIdNet, sizeof(frameIdNet));
  offset += sizeof(frameIdNet);
  std::memcpy(headerBuffer.data() + offset, &jpegSizeNet, sizeof(jpegSizeNet));

  const bool headerOk =
      sendAll(clientFd, headerBuffer.data(), headerBuffer.size());
  const bool dataOk = sendAll(clientFd, jpegBuffer.data(), jpegBuffer.size());
  return headerOk && dataOk;
}

bool TcpServer::handleClient(int clientFd) {
  // Reference pour detecter une perte de lien silencieuse (cable debranche) :
  // aucun FIN/RST n'est genere dans ce cas, seule l'absence prolongee de
  // messages recus permet de la reconnaitre.
  auto lastActivity = std::chrono::steady_clock::now();

  while (true) {
    uint8_t message = 0;
    const auto received = recv(clientFd, &message, sizeof(message), 0);

    if (received == 0) {
      // Le client a ferme la connexion (deconnexion normale ou tue)
      std::cout << "Client deconnecte" << std::endl;
      return false;
    }

    if (received < 0) {
      // Timeout : aucun message recu durant ce cycle
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        const auto idleFor = std::chrono::steady_clock::now() - lastActivity;
        if (idleFor >= Protocol::CLIENT_IDLE_TIMEOUT) {
          std::cout << "Client deconnecte (inactivite, lien probablement "
                       "rompu)"
                    << std::endl;
          return false;
        }
        continue;
      }
      std::cerr << "Erreur recv() : " << std::strerror(errno) << std::endl;
      return false;
    }

    lastActivity = std::chrono::steady_clock::now();
    const auto code = static_cast<Protocol::MessageCode>(message);

    if (code == Protocol::MessageCode::GetFrame) {
      if (!sendFrame(clientFd)) {
        std::cout << "Client deconnecte (echec d'envoi)" << std::endl;
        return false;
      }
    } else if (code == Protocol::MessageCode::Stop) {
      const auto response =
          static_cast<uint8_t>(Protocol::MessageCode::StopAck);
      send(clientFd, &response, sizeof(response), 0);
      std::cout << "Arret demande" << std::endl;
      return true;
    }
  }
}

void TcpServer::run() {
  // Le serveur retourne toujours attendre un nouveau client, sauf apres un
  // STOP explicite (handleClient() retourne alors true)
  while (true) {
    const auto clientFd = acceptClient();
    if (!clientFd) {
      continue;
    }

    const bool stopRequested = handleClient(clientFd.value());
    close(clientFd.value());

    if (stopRequested) {
      return;
    }
  }
}

} // namespace Server
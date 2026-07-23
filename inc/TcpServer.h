/**
 * @file TcpServer.h
 * @brief Serveur TCP/IP transmettant des images capturees par la camera.
 */
#pragma once

#include "Camera.h"
#include "Protocol.h"
#include <netinet/in.h>
#include <optional>

namespace Server {

/**
 * @class TcpServer
 * @brief Encapsule un socket serveur TCP/IP : bind, ecoute, acceptation
 *        d'un client et transmission d'images selon Protocol::MessageCode.
 */
class TcpServer {
public:
  TcpServer() = default;
  ~TcpServer();

  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  /**
   * @brief Ignore SIGPIPE, cree le socket d'ecoute, applique SO_REUSEADDR,
   *        effectue le bind, demarre l'ecoute sur le port du protocole et
   *        initialise la camera.
   * @return true si l'initialisation a reussi, false sinon.
   */
  bool init();

  /// Boucle infinie : accepte un client, le traite, puis en attend un autre.
  void run();

private:
  /**
   * @brief Bloque jusqu'a l'acceptation d'un nouveau client.
   * @return Le descripteur du socket client, ou std::nullopt en cas
   *         d'erreur.
   */
  std::optional<int> acceptClient() const;

  /**
   * @brief Boucle de traitement des messages d'un client.
   * @param clientFd Descripteur du socket client.
   */
  void handleClient(int clientFd);

  /**
   * @brief Capture une image, l'encode en JPEG et transmet l'en-tete
   *        complet (FRAME_HDR, frame_id, jpeg_size) suivi des donnees.
   * @param clientFd Descripteur du socket client.
   */
  void sendFrame(int clientFd);

  int _listenFd = -1;
  sockaddr_in _serverAddr{};
  Camera _camera;
  uint32_t _frameId = 0;
};

} // namespace Server
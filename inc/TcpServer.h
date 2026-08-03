/**
 * @file TcpServer.h
 * @brief Serveur TCP/IP transmettant des images capturees par la camera.
 */
#pragma once

#include "Camera.h"
#include "GpioButton.h"
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

  /// Constructeur par copie desactive (non copiable).
  /// @param other Instance a copier (non utilise).
  TcpServer(const TcpServer &other) = delete;

  /// Affectation par copie desactivee (non copiable).
  /// @param other Instance a copier (non utilise).
  /// @return Reference vers l'instance courante (jamais atteint).
  TcpServer &operator=(const TcpServer &other) = delete;

  /**
   * @brief Ignore SIGPIPE, cree le socket d'ecoute, applique SO_REUSEADDR,
   *        effectue le bind, demarre l'ecoute sur le port du protocole et
   *        initialise la camera et le bouton GPIO.
   * @return true si l'initialisation a reussi, false sinon.
   */
  bool init();

  /**
   * @brief Accepte un client, le traite, puis en attend un autre, jusqu'a
   *        ce qu'un arret explicite (STOP) soit recu.
   */
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
   * @return true si un arret explicite (STOP) a ete recu, false si le
   *         client s'est deconnecte ou en cas d'erreur.
   */
  bool handleClient(int clientFd);

  /**
   * @brief Capture une image, l'encode en JPEG et transmet l'en-tete
   *        complet (FRAME_HDR ou BUTTON_PRESS, frame_id, jpeg_size) suivi
   *        des donnees.
   * @param clientFd Descripteur du socket client.
   */
  void sendFrame(int clientFd);

  int _listenFd = -1;           ///< Descripteur du socket d'ecoute TCP/IP.
  sockaddr_in _serverAddr{};    ///< Adresse et port sur lesquels le serveur ecoute.
  Camera _camera;                ///< Camera USB utilisee pour la capture d'images.
  GpioButton _button;             ///< Bouton-poussoir surveille via libgpiod.
  uint32_t _frameId = 0;          ///< Compteur incremente a chaque image transmise.
};

} // namespace Server
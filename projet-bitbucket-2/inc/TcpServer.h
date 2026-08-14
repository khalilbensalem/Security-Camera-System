/**
 * @file TcpServer.h
 * @brief Serveur TCP/IP transmettant des images capturees par la camera.
 */
#pragma once

#include "Camera.h"
#include "GpioButton.h"
#include "LightSensor.h"
#include "Protocol.h"
#include <chrono>
#include <netinet/in.h>
#include <opencv2/core.hpp>
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
   * @brief Etat de coherence entre la photoresistance et l'image capturee.
   */
  enum class SystemState {
    Normal,      ///< Capteur et image coherents ; l'image est transmise.
    NoLight,     ///< Capteur et image indiquent tous deux une scene sombre.
    SensorError, ///< Capteur et image sont incoherents entre eux.
  };

  /**
   * @brief Nom lisible d'un SystemState, pour les messages de journalisation.
   * @param state Etat a convertir.
   * @return Une chaine litterale statique (ex: "NO_LIGHT").
   */
  static const char *stateName(SystemState state);

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
   * @brief Capture une image, evalue l'etat de coherence luminosite/image
   *        et transmet la reponse appropriee au client.
   *
   * Si l'etat confirme (voir updateConfirmedState()) est NoLight ou
   * SensorError, seul un en-tete (code, frame_id) est transmis, sans
   * image. Sinon, l'image est encodee en JPEG et transmise avec l'en-tete
   * complet (FRAME_HDR ou BUTTON_PRESS, frame_id, jpeg_size) suivi des
   * donnees.
   * @param clientFd Descripteur du socket client.
   * @return false si une transmission reseau a echoue (connexion probablement
   *         perdue), true sinon (y compris si la capture d'image a echoue,
   *         ce qui n'indique pas un probleme de connexion).
   */
  bool sendFrame(int clientFd);

  /**
   * @brief Calcule l'etat instantane (non filtre) a partir de l'image
   *        capturee et de la lecture courante de la photoresistance.
   * @param frame Image capturee lors de ce cycle.
   * @return SystemState::NoLight si le capteur et l'image indiquent tous
   *         deux une scene sombre, SystemState::SensorError s'ils sont
   *         incoherents, SystemState::Normal sinon.
   */
  SystemState computeInstantState(const cv::Mat &frame) const;

  /**
   * @brief Applique un filtre temporel (200 ms) sur les transitions d'etat.
   *
   * Un nouvel etat instantane doit persister sans interruption pendant au
   * moins Protocol::STATE_DEBOUNCE avant d'etre confirme ; jusque-la, l'etat
   * confirme precedent continue d'etre retourne.
   * @param instant Etat instantane calcule par computeInstantState().
   * @return L'etat confirme a transmettre au client pour ce cycle.
   */
  SystemState updateConfirmedState(SystemState instant);

  int _listenFd = -1; ///< Descripteur du socket d'ecoute TCP/IP.
  sockaddr_in
      _serverAddr{};        ///< Adresse et port sur lesquels le serveur ecoute.
  Camera _camera;           ///< Camera USB utilisee pour la capture d'images.
  GpioButton _button;       ///< Bouton-poussoir surveille via libgpiod.
  LightSensor _lightSensor; ///< Photoresistance surveillee via iio.
  uint32_t _frameId = 0;    ///< Compteur incremente a chaque image transmise.

  SystemState _pendingState =
      SystemState::Normal; ///< Dernier etat instantane observe.
  SystemState _confirmedState =
      SystemState::Normal; ///< Etat confirme, transmis au client.
  std::chrono::steady_clock::time_point _pendingSince =
      std::chrono::steady_clock::now(); ///< Instant depuis lequel _pendingState
                                        ///< est stable.
};

} // namespace Server
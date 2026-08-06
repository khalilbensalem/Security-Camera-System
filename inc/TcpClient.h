/**
 * @file TcpClient.h
 * @brief Client TCP/IP qui dialogue avec le serveur Odroid-C2.
 */
#pragma once

#include "Protocol.h"

#include <opencv2/opencv.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Client {

/**
 * @brief Indique la nature de la reponse du serveur a une requete
 *        GET_FRAME.
 */
enum class FrameStatus {
  Normal,      ///< Reponse FRAME_HDR : image normale.
  ButtonPress, ///< Reponse BUTTON_PRESS : image liee a un appui bouton.
  NoLight,     ///< Reponse NO_LIGHT : luminosite insuffisante, pas d'image.
  SensorError, ///< Reponse SENSOR_ERROR : incoherence capteur/image, pas
               ///< d'image.
};

/**
 * @brief Represente la reponse du serveur a une requete GET_FRAME. L'image
 *        n'est presente que lorsque status vaut Normal ou ButtonPress.
 */
struct Frame {
  uint32_t frameId; ///< Numero de la frame, attribue par le serveur.
  cv::Mat image;    ///< Image decodee (BGR), vide si aucune image transmise.
  FrameStatus status = FrameStatus::Normal; ///< Nature de la reponse recue.
};

/**
 * @class TcpClient
 * @brief Encapsule la connexion TCP/IP et l'echange de messages avec le
 *        serveur, y compris la reception d'images.
 */
class TcpClient {
public:
  /// @brief Construit un client non connecte.
  TcpClient() = default;
  /// @brief Ferme la connexion si elle est encore active.
  ~TcpClient();

  /// @brief Non copiable : un TcpClient possede un descripteur de socket
  /// unique.
  TcpClient(const TcpClient &) = delete;
  /// @brief Non copiable : un TcpClient possede un descripteur de socket
  /// unique.
  TcpClient &operator=(const TcpClient &) = delete;

  /**
   * @brief Ouvre la connexion TCP/IP vers le serveur.
   * @param serverIp Adresse IP du serveur (Odroid-C2).
   * @return true si la connexion a reussi, false sinon.
   */
  bool connectToServer(const std::string &serverIp);

  /**
   * @brief Envoie GET_FRAME et recoit la reponse du serveur.
   *        La reponse peut etre FRAME_HDR ou BUTTON_PRESS (en-tete + JPEG),
   *        ou NO_LIGHT / SENSOR_ERROR (aucune image transmise), voir
   *        Frame::status.
   * @return La frame recue, ou std::nullopt en cas d'erreur.
   */
  std::optional<Frame> requestFrame();

  /**
   * @brief Envoie un message simple d'un octet et attend la reponse
   *        (utilise pour STOP).
   * @param message Code de message a envoyer.
   * @return Le code de reponse recu, ou std::nullopt en cas d'erreur.
   */
  std::optional<Protocol::MessageCode>
  sendAndReceive(Protocol::MessageCode message);

  /**
   * @brief Ferme la connexion si elle est active.
   */
  void close();

private:
  /**
   * @brief Recoit exactement 'size' octets, meme si recv() les livre en
   *        plusieurs morceaux.
   * @param buffer Destination des donnees recues.
   * @param size Nombre d'octets attendus.
   * @return true si tous les octets ont ete recus, false en cas d'erreur.
   */
  bool receiveAll(void *buffer, size_t size);

  int _socketFd = -1; ///< Descripteur du socket TCP, -1 si non connecte.
};

} // namespace Client
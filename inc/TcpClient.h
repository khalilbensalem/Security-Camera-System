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
 * @brief Represente une image recue du serveur, avec son numero.
 */
struct Frame {
  uint32_t frameId;
  cv::Mat image;
};

/**
 * @class TcpClient
 * @brief Encapsule la connexion TCP/IP et l'echange de messages avec le
 *        serveur, y compris la reception d'images.
 */
class TcpClient {
public:
  TcpClient() = default;
  ~TcpClient();

  TcpClient(const TcpClient &) = delete;
  TcpClient &operator=(const TcpClient &) = delete;

  /**
   * @brief Ouvre la connexion TCP/IP vers le serveur.
   * @param serverIp Adresse IP du serveur (Odroid-C2).
   * @return true si la connexion a reussi, false sinon.
   */
  bool connectToServer(const std::string &serverIp);

  /**
   * @brief Envoie GET_FRAME et recoit l'image complete (en-tete + JPEG).
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

  int _socketFd = -1;
};

} // namespace Client
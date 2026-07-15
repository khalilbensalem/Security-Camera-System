/**
 * @file TcpClient.h
 * @brief Client TCP/IP qui dialogue avec le serveur Odroid-C2.
 */
#pragma once

#include "Protocol.h"

#include <optional>
#include <string>

namespace Client {

/**
 * @class TcpClient
 * @brief Encapsule la connexion TCP/IP et l'echange de messages avec le
 *        serveur, selon Protocol::MessageCode.
 */
class TcpClient {
public:
    TcpClient() = default;
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    /**
     * @brief Ouvre la connexion TCP/IP vers le serveur.
     * @param serverIp Adresse IP du serveur (Odroid-C2).
     * @return true si la connexion a reussi, false sinon.
     */
    bool connectToServer(const std::string& serverIp);

    /**
     * @brief Envoie un message au serveur et attend sa reponse.
     * @param message Code de message a envoyer (voir Protocol::MessageCode).
     * @return Le code de reponse recu, ou std::nullopt en cas d'erreur.
     */
    std::optional<Protocol::MessageCode> sendAndReceive(Protocol::MessageCode message);

    /// Ferme la connexion si elle est active.
    void close();

private:
    int _socketFd = -1;
};

}  // namespace Client
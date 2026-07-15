/**
 * @file TcpServer.h
 * @brief Serveur TCP/IP simple gerant une connexion client.
 */
#pragma once

#include "Protocol.h"

#include <netinet/in.h>
#include <optional>

namespace Server {

/**
 * @class TcpServer
 * @brief Encapsule un socket serveur TCP/IP : bind, ecoute, acceptation
 *        d'un client et echange de messages selon Protocol::MessageCode.
 */
class TcpServer {
public:
    TcpServer() = default;
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    /**
     * @brief Cree le socket d'ecoute, applique SO_REUSEADDR, effectue le
     *        bind et demarre l'ecoute sur le port du protocole.
     * @return true si l'initialisation a reussi, false sinon.
     */
    bool init();

    /// Accepte un client et traite son message (GET_FRAME -> FRAME_HDR).
    void run();

private:
    /**
     * @brief Bloque jusqu'a l'acceptation d'un nouveau client.
     * @return Le descripteur du socket client, ou std::nullopt en cas
     *         d'erreur.
     */
    std::optional<int> acceptClient() const;

    /**
     * @brief Recoit un message du client et repond selon le protocole.
     * @param clientFd Descripteur du socket client.
     */
    void handleClient(int clientFd) const;

    int _listenFd = -1;
    sockaddr_in _serverAddr{};
};

}  // namespace Server
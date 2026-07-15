/**
 * @file TcpServer.h
 * @brief Serveur TCP/IP simple gerant une connexion client.
 */
#pragma once

#include <netinet/in.h>
#include <optional>

namespace Server {

/**
 * @class TcpServer
 * @brief Encapsule un socket serveur TCP/IP : bind, ecoute et acceptation
 *        d'un client.
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

    /**
     * @brief Attend et accepte une connexion client, confirme la connexion,
     *        puis ferme proprement (etape 2 : validation de la connexion
     *        seulement, sans echange de message).
     */
    void run();

private:
    /**
     * @brief Bloque jusqu'a l'acceptation d'un nouveau client.
     * @return Le descripteur du socket client, ou std::nullopt en cas
     *         d'erreur.
     */
    std::optional<int> acceptClient() const;

    int _listenFd = -1;
    sockaddr_in _serverAddr{};
};

}  // namespace Server

/**
 * @file TcpClient.h
 * @brief Client TCP/IP qui se connecte au serveur Odroid-C2.
 */
#pragma once

#include <string>

namespace Client {

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

    /// Ferme la connexion si elle est active.
    void close();

private:
    int _socketFd = -1;
};

}  // namespace Client
/**
 * @file main.cpp
 * @brief Point d'entree du programme serveur.
 */
#include "TcpServer.h"

/**
 * @brief Point d'entree du serveur : initialise puis lance la boucle TCP/IP.
 * @return 0 en cas d'arret normal, 1 si l'initialisation echoue.
 */
int main() {
  Server::TcpServer server;

  if (!server.init()) {
    return 1;
  }

  server.run();

  return 0;
}
/**
 * @file main.cpp
 * @brief Point d'entree du programme serveur.
 */
#include "TcpServer.h"

int main() {
  Server::TcpServer server;

  if (!server.init()) {
    return 1;
  }

  server.run();

  return 0;
}
/**
 * @file main.cpp
 * @brief Point d'entree du programme client.
 */
#include "TcpClient.h"

#include <fstream>
#include <iostream>

namespace {
constexpr const char *SERVER_IP = "192.168.7.2";
} // namespace

int main() {
  Client::TcpClient client;

  if (!client.connectToServer(SERVER_IP)) {
    return 1;
  }

  // Etape 2 du livrable 2 : validation d'une seule image recue, sauvegardee
  // sur disque pour inspection visuelle (l'affichage OpenCV vient a l'etape
  // suivante)
  const auto frame = client.requestFrame();
  if (frame) {
    std::ofstream file("test_received.jpg", std::ios::binary);
    file.write(reinterpret_cast<const char *>(frame->jpegData.data()),
               frame->jpegData.size());
    std::cout << "Image recue (frame " << frame->frameId
              << ") sauvegardee dans test_received.jpg" << std::endl;
  }

  client.close();

  return 0;
}
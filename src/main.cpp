/// @file main.cpp
/// @brief Point d'entree du programme client (poste de travail).

#include "TcpClient.h"

#include <opencv2/opencv.hpp>

#include <iostream>

namespace {
constexpr const char *SERVER_IP = "192.168.7.2";
constexpr const char *WINDOW_NAME = "Surveillance video";
} // namespace

int main() {
  Client::TcpClient client;

  if (!client.connectToServer(SERVER_IP)) {
    return 1;
  }

  cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE);

  // Etape 3 : une seule requete, affichee dans la fenetre OpenCV
  // (la boucle des 30 ms sera reintroduite a l'etape 4)
  const auto frame = client.requestFrame();
  if (frame) {
    std::cout << "Image recue (frame " << frame->frameId << ")" << std::endl;
    cv::imshow(WINDOW_NAME, frame->image);
  } else {
    std::cerr << "Erreur : aucune image recue" << std::endl;
  }

  // Attend un appui sur une touche pour fermer (validation manuelle)
  cv::waitKey(0);

  client.close();
  return 0;
}
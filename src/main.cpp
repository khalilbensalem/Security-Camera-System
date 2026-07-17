/**
 * @file main.cpp
 * @brief Point d'entree du programme serveur.
 */
#include "Camera.h"
#include "TcpServer.h"

#include <opencv2/opencv.hpp>

#include <iostream>

int main() {
  // Etape 1 du livrable 2 : validation locale de la camera
  Server::Camera camera;
  if (!camera.init()) {
    return 1;
  }

  const auto frame = camera.captureFrame();
  if (frame) {
    cv::imwrite("test_capture.jpg", frame.value());
    std::cout << "Image sauvegardee dans test_capture.jpg" << std::endl;
  }

  // livrable 1
  Server::TcpServer server;

  if (!server.init()) {
    return 1;
  }

  server.run();

  return 0;
}
/**
 * @file Camera.cpp
 * @brief Implementation de la classe Camera.
 */
#include "Camera.h"

#include <iostream>

namespace {
// Resolution imposee par l'enonce du livrable 2
constexpr int FRAME_WIDTH = 800;
constexpr int FRAME_HEIGHT = 600;
} // namespace

namespace Server {

bool Camera::init() {
  // Ouverture de la premiere camera USB detectee (index 0)
  _videoCapture.open(0);
  if (!_videoCapture.isOpened()) {
    std::cerr << "Erreur : impossible d'ouvrir la camera USB" << std::endl;
    return false;
  }

  // Configuration de la resolution demandee par l'enonce
  _videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
  _videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

  std::cout << "Camera ouverte avec succes" << std::endl;
  return true;
}

std::optional<cv::Mat> Camera::captureFrame() {
  cv::Mat frame;

  // Lecture bloquante d'une image depuis la camera
  if (!_videoCapture.read(frame) || frame.empty()) {
    std::cerr << "Erreur : capture d'image echouee" << std::endl;
    return std::nullopt;
  }

  return frame;
}

} // namespace Server
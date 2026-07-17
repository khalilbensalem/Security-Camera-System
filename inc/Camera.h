/**
 * @file Camera.h
 * @brief Capture d'images a partir de la camera USB.
 */
#pragma once

#include <opencv2/opencv.hpp>

#include <optional>

namespace Server {

/**
 * @class Camera
 * @brief Encapsule cv::VideoCapture pour la capture d'images depuis la
 *        camera USB connectee a l'Odroid-C2.
 */
class Camera {
public:
  Camera() = default;

  Camera(const Camera &) = delete;
  Camera &operator=(const Camera &) = delete;

  /**
   * @brief Ouvre la camera USB et configure la resolution attendue.
   * @return true si la camera a ete ouverte avec succes, false sinon.
   */
  bool init();

  /**
   * @brief Capture une image depuis la camera.
   * @return L'image capturee, ou std::nullopt en cas d'erreur.
   */
  std::optional<cv::Mat> captureFrame();

private:
  cv::VideoCapture _videoCapture;
};

} // namespace Server
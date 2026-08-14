/**
 * @file Camera.h
 * @brief Capture d'images a partir de la camera USB.
 */
#pragma once

#include <opencv2/opencv.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>

namespace Server {

/**
 * @class Camera
 * @brief Encapsule cv::VideoCapture pour la capture d'images depuis la
 *        camera USB connectee a l'Odroid-C2.
 *
 * La capture s'effectue en continu sur un thread dedie (voir captureLoop()),
 * plutot qu'au moment de la requete : cv::VideoCapture::read() peut bloquer
 * bien au-dela du cycle de 30 ms attendu par le protocole (l'auto-exposition
 * rallonge le temps d'obturation en tres faible luminosite, par exemple).
 * captureFrame() renvoie simplement la derniere image disponible, sans
 * jamais attendre la camera.
 */
class Camera {
public:
  Camera() = default;
  /// Arrete le thread de capture et attend sa terminaison.
  ~Camera();

  /// Constructeur par copie desactive (non copiable).
  /// @param other Instance a copier (non utilise).
  Camera(const Camera &other) = delete;

  /// Affectation par copie desactivee (non copiable).
  /// @param other Instance a copier (non utilise).
  /// @return Reference vers l'instance courante (jamais atteint).
  Camera &operator=(const Camera &other) = delete;

  /**
   * @brief Ouvre la camera USB, configure la resolution attendue et
   *        demarre le thread de capture continue.
   * @return true si la camera a ete ouverte avec succes, false sinon.
   */
  bool init();

  /**
   * @brief Renvoie la derniere image capturee, sans jamais bloquer sur la
   *        camera.
   * @return La derniere image disponible, ou std::nullopt si aucune image
   *         n'a encore ete capturee.
   */
  std::optional<cv::Mat> captureFrame();

private:
  /// Boucle executee sur le thread dedie : capture des images en continu et
  /// met _latestFrame a jour (protege par _frameMutex).
  void captureLoop();

  cv::VideoCapture
      _videoCapture; ///< Flux de capture OpenCV vers la camera USB.
  std::thread _captureThread; ///< Thread dedie a la capture continue.
  std::atomic<bool> _running{
      false};              ///< Controle l'execution de captureLoop().
  std::mutex _frameMutex;  ///< Protege l'acces a _latestFrame.
  cv::Mat _latestFrame;    ///< Derniere image capturee (vide si aucune encore).
};

} // namespace Server
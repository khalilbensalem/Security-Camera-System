/**
 * @file Camera.cpp
 * @brief Implementation de la classe Camera.
 */
#include "Camera.h"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace {
/// Largeur d'image imposee par l'enonce du livrable 2.
constexpr int FRAME_WIDTH = 800;
/// Hauteur d'image imposee par l'enonce du livrable 2.
constexpr int FRAME_HEIGHT = 600;
/// Taux de rafraichissement vise, en images par seconde.
constexpr int FRAME_FPS = 30;

/**
 * @brief Redirige stderr vers /dev/null le temps de sa duree de vie.
 *
 * Masque les avertissements libjpeg benins emis lors du decodage MJPG
 * materiel de la camera (voir Camera::init()), puis restaure stderr a la
 * destruction de l'objet.
 */
class ScopedStderrSuppressor {

public:
  ScopedStderrSuppressor() {
    _savedStderr = dup(STDERR_FILENO);
    const int devNull = open("/dev/null", O_WRONLY);
    if (devNull >= 0) {
      dup2(devNull, STDERR_FILENO);
      close(devNull);
    }
  }

  ~ScopedStderrSuppressor() {
    if (_savedStderr >= 0) {
      dup2(_savedStderr, STDERR_FILENO);
      close(_savedStderr);
    }
  }

  /// Constructeur par copie desactive (non copiable).
  /// @param other Instance a copier (non utilise).
  ScopedStderrSuppressor(const ScopedStderrSuppressor &other) = delete;

  /// Affectation par copie desactivee (non copiable).
  /// @param other Instance a copier (non utilise).
  /// @return Reference vers l'instance courante (jamais atteint).
  ScopedStderrSuppressor &operator=(const ScopedStderrSuppressor &other) = delete;

private:
  int _savedStderr = -1; ///< Copie du descripteur stderr original, pour le restaurer.
};

} // namespace

namespace Server {

bool Camera::init() {
  // Backend V4L2 explicite (le backend par defaut tentait GStreamer sans
  // succes)
  _videoCapture.open(0, cv::CAP_V4L2);
  if (!_videoCapture.isOpened()) {
    std::cerr << "Erreur : impossible d'ouvrir la camera USB" << std::endl;
    return false;
  }

  // Le format doit etre impose avant la resolution : le pilote V4L2 negocie
  // le fps selon le format de pixel courant. En YUYV brut, le bus USB
  // plafonne a 20 fps a cette resolution ; en MJPG (compresse par la
  // camera), le vrai 30 fps vise par le cycle est atteignable.
  _videoCapture.set(cv::CAP_PROP_FOURCC,
                    cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

  // Resolution et fps demandes par l'enonce, une fois le format MJPG etabli
  _videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
  _videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
  _videoCapture.set(cv::CAP_PROP_FPS, FRAME_FPS);

  std::cout << "Camera ouverte avec succes" << std::endl;
  return true;
}

std::optional<cv::Mat> Camera::captureFrame() {
  cv::Mat frame;
  bool readOk = false;

  {
    // Scope limite a read() : le message d'erreur ci-dessous reste visible
    ScopedStderrSuppressor suppressStderr;
    readOk = _videoCapture.read(frame);
  }

  if (!readOk || frame.empty()) {
    std::cerr << "Erreur : capture d'image echouee" << std::endl;
    return std::nullopt;
  }

  return frame;
}

} // namespace Server
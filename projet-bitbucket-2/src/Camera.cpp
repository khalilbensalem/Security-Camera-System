/**
 * @file Camera.cpp
 * @brief Implementation de la classe Camera.
 */
#include "Camera.h"
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

namespace {
/// Largeur d'image imposee par l'enonce du livrable 2.
constexpr int FRAME_WIDTH = 800;
/// Hauteur d'image imposee par l'enonce du livrable 2.
constexpr int FRAME_HEIGHT = 600;
/// Taux de rafraichissement vise, en images par seconde.
constexpr int FRAME_FPS = 30;
/// Plus grand index /dev/videoN teste lors de la recherche de la camera.
constexpr int MAX_VIDEO_NODE_INDEX = 9;
/// Nombre d'echecs de lecture consecutifs tolere avant de tenter de
/// rouvrir la camera (voir Camera::openCamera()).
constexpr int MAX_CONSECUTIVE_READ_FAILURES = 15;
/// Delai d'attente entre deux tentatives de reouverture infructueuses.
constexpr auto REOPEN_RETRY_DELAY = std::chrono::milliseconds(500);

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
  ScopedStderrSuppressor &
  operator=(const ScopedStderrSuppressor &other) = delete;

private:
  int _savedStderr =
      -1; ///< Copie du descripteur stderr original, pour le restaurer.
};

} // namespace

namespace Server {

Camera::~Camera() {
  _running = false;
  if (_captureThread.joinable()) {
    _captureThread.join();
  }
}

bool Camera::openCamera() {
  // Recherche par essai plutot qu'un index fixe (0) : sur l'Odroid-C2, la
  // camera USB se reinitialise au niveau materiel au demarrage (visible
  // dans dmesg : "USB disconnect" suivi d'une reenumeration), ce qui peut
  // faire disparaitre /dev/video0 au profit d'un autre index (ex:
  // /dev/video1). On teste chaque noeud avec une vraie lecture, pas
  // seulement l'ouverture, pour eviter de s'arreter sur un noeud de
  // metadonnees UVC qui s'ouvre mais ne produit jamais d'image.
  for (int index = 0; index <= MAX_VIDEO_NODE_INDEX; ++index) {
    const std::string path = "/dev/video" + std::to_string(index);

    // Backend V4L2 explicite (le backend par defaut tentait GStreamer sans
    // succes)
    {
      ScopedStderrSuppressor suppressStderr;
      _videoCapture.open(path, cv::CAP_V4L2);
    }
    if (!_videoCapture.isOpened()) {
      continue;
    }

    // Le format doit etre impose avant la resolution : le pilote V4L2
    // negocie le fps selon le format de pixel courant. En YUYV brut, le bus
    // USB plafonne a 20 fps a cette resolution ; en MJPG (compresse par la
    // camera), le vrai 30 fps vise par le cycle est atteignable.
    _videoCapture.set(cv::CAP_PROP_FOURCC,
                      cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    _videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    _videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    _videoCapture.set(cv::CAP_PROP_FPS, FRAME_FPS);

    cv::Mat testFrame;
    bool readOk = false;
    {
      ScopedStderrSuppressor suppressStderr;
      readOk = _videoCapture.read(testFrame);
    }
    if (readOk && !testFrame.empty()) {
      std::cout << "Camera ouverte avec succes (" << path << ")" << std::endl;
      return true;
    }

    _videoCapture.release();
  }

  return false;
}

bool Camera::init() {
  if (!openCamera()) {
    std::cerr << "Erreur : impossible d'ouvrir la camera USB" << std::endl;
    return false;
  }

  _running = true;
  _captureThread = std::thread(&Camera::captureLoop, this);
  return true;
}

void Camera::captureLoop() {
  int consecutiveFailures = 0;

  while (_running) {
    cv::Mat frame;
    bool readOk = false;

    {
      // Scope limite a read() : le message d'erreur ci-dessous reste visible
      ScopedStderrSuppressor suppressStderr;
      readOk = _videoCapture.read(frame);
    }

    if (readOk && !frame.empty()) {
      consecutiveFailures = 0;
      std::lock_guard<std::mutex> lock(_frameMutex);
      _latestFrame = frame;
      continue;
    }

    // Le noeud video ouvert peut avoir disparu (ex: reinitialisation
    // materielle de la camera USB au demarrage de l'Odroid-C2) : au-dela
    // d'un nombre d'echecs consecutifs, on tente de rouvrir la camera
    // plutot que de rester bloque indefiniment sur un descripteur mort.
    if (++consecutiveFailures >= MAX_CONSECUTIVE_READ_FAILURES) {
      std::cerr << "Camera : trop d'echecs de lecture consecutifs, "
                   "nouvelle tentative d'ouverture."
                << std::endl;
      _videoCapture.release();
      if (openCamera()) {
        consecutiveFailures = 0;
      } else {
        std::this_thread::sleep_for(REOPEN_RETRY_DELAY);
      }
    }
  }
}

std::optional<cv::Mat> Camera::captureFrame() {
  std::lock_guard<std::mutex> lock(_frameMutex);
  if (_latestFrame.empty()) {
    return std::nullopt;
  }
  return _latestFrame.clone();
}

} // namespace Server
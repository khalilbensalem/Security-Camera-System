/**
 * @file main.cpp
 * @brief Point d'entree du programme client (poste de travail).
 */
#include "CycleStats.h"
#include "TcpClient.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
/// Adresse IP du serveur (Odroid-C2) auquel le client se connecte.
constexpr const char *SERVER_IP = "192.168.7.2";
/// Titre de la fenetre OpenCV d'affichage video.
constexpr const char *WINDOW_NAME = "Surveillance video";
/// Repertoire ou sont sauvegardees les images capturees sur appui bouton.
constexpr const char *IMAGES_DIR = "images";
/// Largeur des images transmises par le serveur (Livrable 2), utilisee pour
/// construire les images d'avertissement NO_LIGHT / SENSOR_ERROR qui ne
/// contiennent pas d'image.
constexpr int FRAME_WIDTH = 800;
/// Hauteur des images transmises par le serveur (Livrable 2), utilisee pour
/// construire les images d'avertissement NO_LIGHT / SENSOR_ERROR qui ne
/// contiennent pas d'image.
constexpr int FRAME_HEIGHT = 600;

/// @brief Cree le repertoire "images" s'il n'existe pas, ou le vide s'il
/// existe deja (exigence du Livrable 3 : purge au demarrage du client).
void prepareImagesDirectory() {
  namespace fs = std::filesystem;
  const fs::path dir(IMAGES_DIR);
  if (fs::exists(dir)) {
    for (const auto &entry : fs::directory_iterator(dir)) {
      fs::remove_all(entry.path());
    }
  } else {
    fs::create_directories(dir);
  }
}

/// @brief Construit une image noire avec un message centre, utilisee pour
/// signaler les etats NO_LIGHT et SENSOR_ERROR (Livrable 4).
/// @param message Le texte d'avertissement a afficher, centre dans l'image.
/// @return L'image noire (FRAME_WIDTH x FRAME_HEIGHT) avec le message
/// centre en rouge.
cv::Mat makeWarningFrame(const std::string &message) {
  cv::Mat frame(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
  int baseline = 0;
  const auto textSize =
      cv::getTextSize(message, cv::FONT_HERSHEY_SIMPLEX, 1.0, 2, &baseline);
  const cv::Point origin((FRAME_WIDTH - textSize.width) / 2,
                         (FRAME_HEIGHT + textSize.height) / 2);
  cv::putText(frame, message, origin, cv::FONT_HERSHEY_SIMPLEX, 1.0,
              cv::Scalar(0, 0, 255), 2);
  return frame;
}
/// @brief Etat de connexion du client au serveur (Livrable 5).
enum class ConnectionState {
  Connected,    ///< Connexion active, les GET_FRAME sont envoyes normalement.
  Disconnected, ///< Connexion perdue, en attente de reconnexion.
};
} // namespace

/// @brief Point d'entree : connecte le client au serveur, puis boucle sur
/// la capture/l'affichage des images jusqu'a l'arret demande par l'usager.
/// @return 0 en cas d'arret normal, 1 si la connexion au serveur echoue.
int main() {
  Client::TcpClient client;
  Client::CycleStats stats;

  if (!client.connectToServer(SERVER_IP)) {
    return 1;
  }

  prepareImagesDirectory();

  cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE);

  ConnectionState connState = ConnectionState::Connected;
  auto lastFrameSuccessTime = std::chrono::steady_clock::now();
  auto lastReconnectAttempt = std::chrono::steady_clock::now();

  bool running = true;
  while (running) {
    const auto cycleStart = std::chrono::steady_clock::now();

    if (connState == ConnectionState::Connected) {
      const auto frame = client.requestFrame();
      if (frame) {
        lastFrameSuccessTime = cycleStart;
        switch (frame->status) {
        case Client::FrameStatus::Normal:
        case Client::FrameStatus::ButtonPress:
          cv::putText(frame->image, "Frame: " + std::to_string(frame->frameId),
                      cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                      cv::Scalar(0, 255, 0), 2);
          cv::imshow(WINDOW_NAME, frame->image);

          if (frame->status == Client::FrameStatus::ButtonPress) {
            const std::string filename =
                std::string(IMAGES_DIR) + "/capture_frame_" +
                std::to_string(frame->frameId) + ".jpg";
            cv::imwrite(filename, frame->image);
            std::cout << "Image sauvegardee : " << filename << std::endl;
          }
          break;

        case Client::FrameStatus::NoLight:
          cv::imshow(WINDOW_NAME, makeWarningFrame("Lumiere insuffisante"));
          break;

        case Client::FrameStatus::SensorError:
          cv::imshow(WINDOW_NAME, makeWarningFrame("Capteur errone"));
          break;
        }
      } else {
        // requestFrame() a echoue (timeout recv, send echoue, etc.) : on
        // tolere quelques echecs isoles (gigue reseau normale) avant de
        // declarer la connexion perdue.
        const auto idleFor = cycleStart - lastFrameSuccessTime;
        if (idleFor >=
            std::chrono::milliseconds(Protocol::CONNECTION_LOST_TIMEOUT_MS)) {
          std::cout << "Connexion perdue" << std::endl;
          client.close();
          connState = ConnectionState::Disconnected;
          lastReconnectAttempt = cycleStart;
        }
      }
    } else {
      if (cycleStart - lastReconnectAttempt >=
          std::chrono::milliseconds(Protocol::RECONNECT_INTERVAL_MS)) {
        lastReconnectAttempt = cycleStart;
        if (client.tryReconnect(SERVER_IP)) {
          connState = ConnectionState::Connected;
          lastFrameSuccessTime = cycleStart;
        }
      }
      cv::imshow(WINDOW_NAME, makeWarningFrame("CONNEXION PERDUE"));
    }

    // Soustrait le temps deja ecoule (requete + affichage) du delai
    // d'attente, pour que le cycle TOTAL dure ~30 ms plutot que d'ajouter
    // 30 ms de plus par-dessus le traitement.
    const auto elapsed = std::chrono::steady_clock::now() - cycleStart;
    const auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    const int remainingMs =
        std::max(1, static_cast<int>(Protocol::CYCLE_INTERVAL_MS - elapsedMs));

    const int key = cv::waitKey(remainingMs) & 0xFF;
    if (key == 'q') {
      std::cout << "Arret demande" << std::endl;
      client.sendAndReceive(Protocol::MessageCode::Stop);
      running = false;
    }

    const auto cycleEnd = std::chrono::steady_clock::now();
    stats.recordCycle(cycleEnd - cycleStart);
  }

  client.close();
  return 0;
}
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
constexpr const char *SERVER_IP = "192.168.7.2";
constexpr const char *WINDOW_NAME = "Surveillance video";
constexpr const char *IMAGES_DIR = "images";

// Cree le repertoire "images" s'il n'existe pas, ou le vide s'il existe
// deja (exigence du Livrable 3 : purge au demarrage du client).
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
} // namespace

int main() {
  Client::TcpClient client;
  Client::CycleStats stats;

  if (!client.connectToServer(SERVER_IP)) {
    return 1;
  }

  prepareImagesDirectory();

  cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE);

  bool running = true;
  while (running) {
    const auto cycleStart = std::chrono::steady_clock::now();

    const auto frame = client.requestFrame();
    if (frame) {
      // Affiche le numero de la frame dans le coin superieur gauche
      cv::putText(frame->image, "Frame: " + std::to_string(frame->frameId),
                  cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                  cv::Scalar(0, 255, 0), 2);

      cv::imshow(WINDOW_NAME, frame->image);

      // Un appui sur le bouton a ete signale par le serveur : on
      // sauvegarde l'image en plus de l'afficher normalement.
      if (frame->isButtonPress) {
        const std::string filename = std::string(IMAGES_DIR) +
                                     "/capture_frame_" +
                                     std::to_string(frame->frameId) + ".jpg";
        cv::imwrite(filename, frame->image);
        std::cout << "Image sauvegardee : " << filename << std::endl;
      }
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
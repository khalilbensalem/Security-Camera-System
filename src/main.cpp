/**
 * @file main.cpp
 * @brief Point d'entree du programme client (poste de travail).
 */
#include "CycleStats.h"
#include "TcpClient.h"

#include <opencv2/opencv.hpp>

#include <chrono>
#include <iostream>
#include <string>

namespace {
constexpr const char *SERVER_IP = "192.168.7.2";
constexpr const char *WINDOW_NAME = "Surveillance video";
} // namespace

int main() {
  Client::TcpClient client;
  Client::CycleStats stats;

  if (!client.connectToServer(SERVER_IP)) {
    return 1;
  }

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
    }

    const int key = cv::waitKey(Protocol::CYCLE_INTERVAL_MS) & 0xFF;
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
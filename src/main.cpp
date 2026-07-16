/**
 * @file main.cpp
 * @brief Point d'entree du programme client.
 */
#include "CycleStats.h"
#include "TcpClient.h"

#include <opencv2/opencv.hpp>

#include <chrono>
#include <iostream>

namespace {
constexpr const char* SERVER_IP = "192.168.7.2";
constexpr const char* WINDOW_NAME = "Surveillance video";
}  // namespace

int main() {
    Client::TcpClient client;

    if (!client.connectToServer(SERVER_IP)) {
        return 1;
    }

    cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE);
    const cv::Mat blankFrame = cv::Mat::zeros(600, 800, CV_8UC3);
    cv::imshow(WINDOW_NAME, blankFrame);

    Client::CycleStats stats;
    bool running = true;

    while (running) {
        const auto cycleStart = std::chrono::steady_clock::now();

        client.sendAndReceive(Protocol::MessageCode::GetFrame);

        const int key = cv::waitKey(Protocol::CYCLE_INTERVAL_MS) & 0xFF;
        if (key == 'q') {
            client.sendAndReceive(Protocol::MessageCode::Stop);
            running = false;
        }

        const auto cycleEnd = std::chrono::steady_clock::now();
        stats.recordCycle(cycleEnd - cycleStart);
    }

    client.close();

    return 0;
}
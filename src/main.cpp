/**
 * @file main.cpp
 * @brief Point d'entree du programme client.
 */
#include "TcpClient.h"

#include <opencv2/opencv.hpp>

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

    // Boucle principale : un cycle GET_FRAME/FRAME_HDR toutes les 30 ms
    bool running = true;
    while (running) {
        client.sendAndReceive(Protocol::MessageCode::GetFrame);

        // waitKey sert de minuterie (30 ms) ET de lecture clavier
        const int key = cv::waitKey(Protocol::CYCLE_INTERVAL_MS) & 0xFF;

        if (key == 'q') {
            client.sendAndReceive(Protocol::MessageCode::Stop);
            running = false;
        }
    }

    client.close();

    return 0;
}
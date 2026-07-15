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

    // Validation de l'echange GET_FRAME / FRAME_HDR (etape 3)
    const auto response = client.sendAndReceive(Protocol::MessageCode::GetFrame);
    if (response && response.value() == Protocol::MessageCode::FrameHdr) {
        std::cout << "FRAME_HDR recu" << std::endl;
    }

    // Fenetre OpenCV, vide pour l'instant (aucune image n'est encore transmise)
    cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE);
    cv::Mat blankFrame = cv::Mat::zeros(600, 800, CV_8UC3);
    cv::imshow(WINDOW_NAME, blankFrame);

    // Attend un appui sur une touche pour fermer (validation manuelle de la fenetre)
    cv::waitKey(0);

    client.close();

    return 0;
}
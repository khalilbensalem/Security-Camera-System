#include "TcpClient.h"

#include <iostream>

namespace {
constexpr const char *SERVER_IP = "192.168.7.2";
}

int main() {
  Client::TcpClient client;

  if (!client.connectToServer(SERVER_IP)) {
    return 1;
  }

  const auto frame = client.requestFrame();
  if (frame) {
    cv::imwrite("test_received.jpg", frame->image);
    std::cout << "Image recue (frame " << frame->frameId
              << ") sauvegardee dans test_received.jpg" << std::endl;
  }

  client.close();
  return 0;
}
/**
 * @file GpioButton.cpp
 * @brief Implementation de GpioButton.
 */
#include "GpioButton.h"

#include <chrono>
#include <gpiod.hpp>
#include <iostream>

namespace Server {

namespace {
/// Periode d'echantillonnage de la ligne GPIO dans monitorLoop().
constexpr auto POLL_INTERVAL = std::chrono::milliseconds(5);
/// Delai minimal entre deux changements d'etat acceptes (anti-rebond).
constexpr auto DEBOUNCE_DELAY = std::chrono::milliseconds(50);
} // namespace

GpioButton::~GpioButton() {
  _running = false;
  if (_thread.joinable()) {
    _thread.join();
  }
}

bool GpioButton::init(const std::string &chipName, unsigned int lineOffset) {
  _chipName = chipName;
  _lineOffset = lineOffset;
  _running = true;
  _thread = std::thread(&GpioButton::monitorLoop, this);
  return true;
}

bool GpioButton::consumePressEvent() {
  // exchange(false) : lit ET reinitialise en une seule operation atomique
  // -> garantit qu'un appui ne peut declencher qu'une seule sauvegarde,
  // meme si sendFrame() est appele par un autre thread pendant ce temps.
  return _pressed.exchange(false);
}

void GpioButton::monitorLoop() {
  try {
    gpiod::chip chip(_chipName);
    gpiod::line line = chip.get_line(_lineOffset);
    line.request({"button_monitor", gpiod::line_request::DIRECTION_INPUT, 0});

    int lastValue = line.get_value();
    auto lastChangeTime = std::chrono::steady_clock::now();

    while (_running) {
      const int value = line.get_value();
      const auto now = std::chrono::steady_clock::now();

      // Anti-rebond : on n'accepte un changement que 50 ms apres le dernier
      if (value != lastValue && (now - lastChangeTime) > DEBOUNCE_DELAY) {
        lastChangeTime = now;
        lastValue = value;

        if (value == 1) { // a ajuster selon actif haut/bas de ton cablage
          _pressed = true;
        }
      }

      std::this_thread::sleep_for(POLL_INTERVAL);
    }
  } catch (const std::exception &e) {
    std::cerr << "GpioButton: erreur sur " << _chipName << " ligne "
              << _lineOffset << " : " << e.what() << std::endl;
    _running = false;
  }
}

} // namespace Server
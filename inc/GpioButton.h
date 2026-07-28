/**
 * @file GpioButton.h
 * @brief Surveillance non bloquante d'un bouton-poussoir via libgpiod.
 */
#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace Server {

class GpioButton {
public:
  GpioButton() = default;
  ~GpioButton();

  GpioButton(const GpioButton &) = delete;
  GpioButton &operator=(const GpioButton &) = delete;

  /**
   * @brief Demarre le thread de surveillance de la ligne GPIO donnee.
   * @param chipName Nom du controleur (ex: "gpiochip1").
   * @param lineOffset Numero de la ligne correspondant au bouton.
   * @return true si le thread a demarre correctement.
   */
  bool init(const std::string &chipName, unsigned int lineOffset);

  /**
   * @brief Consomme l'evenement d'appui en attente, s'il y en a un.
   * @return true si un appui etait en attente (et le reinitialise).
   */
  bool consumePressEvent();

private:
  void monitorLoop();

  std::string _chipName;
  unsigned int _lineOffset = 0;
  std::atomic<bool> _pressed{false};
  std::atomic<bool> _running{false};
  std::thread _thread;
};

} // namespace Server
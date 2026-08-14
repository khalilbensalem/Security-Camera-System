/**
 * @file GpioButton.h
 * @brief Surveillance non bloquante d'un bouton-poussoir via libgpiod.
 */
#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace Server {

/**
 * @brief Surveille de facon non bloquante l'etat d'un bouton-poussoir GPIO.
 *
 * Le suivi s'effectue sur un thread dedie (voir monitorLoop()) afin de ne
 * jamais bloquer le cycle principal du serveur. consumePressEvent() permet
 * de recuperer et reinitialiser atomiquement un appui detecte.
 */
class GpioButton {
public:
  GpioButton() = default;
  ~GpioButton();

  /// Constructeur par copie desactive (non copiable).
  /// @param other Instance a copier (non utilise).
  GpioButton(const GpioButton &other) = delete;

  /// Affectation par copie desactivee (non copiable).
  /// @param other Instance a copier (non utilise).
  /// @return Reference vers l'instance courante (jamais atteint).
  GpioButton &operator=(const GpioButton &other) = delete;

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
  /// Boucle executee sur le thread dedie : echantillonne la ligne GPIO,
  /// applique l'anti-rebond et met _pressed a jour lors d'un appui detecte.
  void monitorLoop();

  std::string _chipName;        ///< Nom du controleur GPIO (ex: "gpiochip1").
  unsigned int _lineOffset = 0; ///< Numero de la ligne correspondant au bouton.
  std::atomic<bool> _pressed{
      false}; ///< Indique qu'un appui est en attente de consommation.
  std::atomic<bool> _running{false}; ///< Controle l'execution de monitorLoop().
  std::thread _thread; ///< Thread dedie a la surveillance du GPIO.
};

} // namespace Server
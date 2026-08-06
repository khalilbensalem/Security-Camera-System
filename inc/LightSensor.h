/**
 * @file LightSensor.h
 * @brief Lecture de la luminosite ambiante via une photoresistance connectee
 *        a l'Odroid-C2 par le sous-systeme iio.
 */
#pragma once

#include <optional>
#include <string>

namespace Server {

/**
 * @class LightSensor
 * @brief Lit la valeur brute d'un canal ADC expose par le sous-systeme IIO
 *        du noyau Linux, relie a la photoresistance connectee a l'Odroid-C2.
 *
 * A l'initialisation, la classe recherche le canal in_voltage0_raw sous
 * /sys/bus/iio/devices/ (numero de device iio variable, canal fixe). Ce
 * canal a ete confirme empiriquement sur le montage du groupe (chip
 * meson-gxbb-saradc) comme etant celui relie a la photoresistance ; voir
 * les constantes documentees dans LightSensor.cpp si le montage differe.
 */
class LightSensor {
public:
  LightSensor() = default;

  /// Constructeur par copie desactive (non copiable).
  /// @param other Instance a copier (non utilise).
  LightSensor(const LightSensor &other) = delete;

  /// Affectation par copie desactivee (non copiable).
  /// @param other Instance a copier (non utilise).
  /// @return Reference vers l'instance courante (jamais atteint).
  LightSensor &operator=(const LightSensor &other) = delete;

  /**
   * @brief Recherche et memorise le chemin sysfs du canal ADC de la
   *        photoresistance sous /sys/bus/iio/devices/.
   * @return true si le canal a ete trouve, false sinon.
   */
  bool init();

  /**
   * @brief Lit la valeur brute courante du canal ADC.
   *
   * Le driver meson-gxbb-saradc de l'Odroid-C2 peut retourner EINVAL de
   * facon transitoire lors de lectures rapprochees ; readRaw() reessaie
   * automatiquement quelques fois avant d'abandonner (voir LightSensor.cpp).
   * @return La valeur lue, ou std::nullopt en cas d'echec persistant.
   */
  std::optional<int> readRaw() const;

  /**
   * @brief Indique si la luminosite mesuree par le capteur est faible.
   * @return true si la lecture depasse le seuil de luminosite faible (la
   *         valeur brute augmente quand la scene s'assombrit sur ce
   *         montage). Retourne false si la lecture echoue (le capteur est
   *         alors considere "lumineux" par defaut, ce qui fera ressortir
   *         une panne materielle via un etat SENSOR_ERROR plutot que de la
   *         masquer silencieusement en NO_LIGHT).
   */
  bool isDark() const;

private:
  std::string
      _rawPath; ///< Chemin sysfs vers le fichier in_voltageX_raw trouve.
};

} // namespace Server
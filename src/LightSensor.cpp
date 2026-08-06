/**
 * @file LightSensor.cpp
 * @brief Implementation de LightSensor.
 */
#include "LightSensor.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace Server {

namespace {

/// Repertoire racine des peripheriques IIO exposes par le noyau Linux.
constexpr auto IIO_ROOT = "/sys/bus/iio/devices";

/// Canal ADC confirme empiriquement comme etant celui de la photoresistance
/// (chip meson-gxbb-saradc de l'Odroid-C2) : mesure a la lumiere ~890-910,
/// mesure occultee ~1020 (proche du maximum de l'ADC, 1023 sur 10 bits).
/// Les 7 autres canaux du meme chip restent quasi constants quel que soit
/// l'eclairage (bruit ou non connectes).
constexpr auto VOLTAGE_CHANNEL_FILE = "in_voltage0_raw";

/// Au-dessus de ce seuil (valeur brute ADC), la scene est consideree comme
/// sombre. Place a mi-chemin entre la plage eclairee mesuree (~890-910) et
/// la plage occultee mesuree (~1020), avec marge contre le bruit de mesure.
constexpr int NO_LIGHT_THRESHOLD = 960;

} // namespace

bool LightSensor::init() {
  DIR *root = opendir(IIO_ROOT);
  if (root == nullptr) {
    std::cerr << "LightSensor: impossible d'ouvrir " << IIO_ROOT << std::endl;
    return false;
  }

  dirent *deviceEntry = nullptr;
  while ((deviceEntry = readdir(root)) != nullptr) {
    const std::string deviceName = deviceEntry->d_name;
    if (deviceName == "." || deviceName == "..") {
      continue;
    }

    const std::string candidate =
        std::string(IIO_ROOT) + "/" + deviceName + "/" + VOLTAGE_CHANNEL_FILE;
    if (access(candidate.c_str(), R_OK) == 0) {
      _rawPath = candidate;
      break;
    }
  }
  closedir(root);

  if (_rawPath.empty()) {
    std::cerr << "LightSensor: canal " << VOLTAGE_CHANNEL_FILE
              << " introuvable sous " << IIO_ROOT << std::endl;
    return false;
  }

  std::cout << "LightSensor: canal detecte -> " << _rawPath << std::endl;
  return true;
}

std::optional<int> LightSensor::readRaw() const {
  // Lecture bas niveau (open/read/close) plutot que std::ifstream : les
  // attributs sysfs de l'IIO doivent etre relus par un open() frais a
  // chaque fois (pas de seek), et cette approche donne acces a errno pour
  // diagnostiquer precisement un echec (permission, device non pret, etc.).
  const int fd = open(_rawPath.c_str(), O_RDONLY);
  if (fd < 0) {
    std::cerr << "LightSensor: open() a echoue sur " << _rawPath << " : "
              << std::strerror(errno) << std::endl;
    return std::nullopt;
  }

  char buffer[32] = {};
  const auto bytesRead = read(fd, buffer, sizeof(buffer) - 1);
  const int readErrno = errno;
  close(fd);

  if (bytesRead <= 0) {
    std::cerr << "LightSensor: read() a echoue sur " << _rawPath << " : "
              << std::strerror(readErrno) << std::endl;
    return std::nullopt;
  }

  try {
    return std::stoi(std::string(buffer, static_cast<size_t>(bytesRead)));
  } catch (const std::exception &e) {
    std::cerr << "LightSensor: valeur illisible sur " << _rawPath << " (\""
              << buffer << "\") : " << e.what() << std::endl;
    return std::nullopt;
  }
}

bool LightSensor::isDark() const {
  const auto raw = readRaw();
  if (!raw) {
    return false;
  }
  // Confirme empiriquement sur le montage du groupe : la valeur brute
  // augmente quand la scene s'assombrit (voir VOLTAGE_CHANNEL_FILE).
  return raw.value() > NO_LIGHT_THRESHOLD;
}

} // namespace Server

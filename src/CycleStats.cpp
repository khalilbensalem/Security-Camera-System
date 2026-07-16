/**
 * @file CycleStats.cpp
 * @brief Implementation de la classe CycleStats.
 */
#include "CycleStats.h"

#include <ctime>
#include <iomanip>
#include <iostream>

namespace Client {

void CycleStats::recordCycle(std::chrono::steady_clock::duration cycleDuration) {
    ++_cycleCount;
    _accumulatedDuration += cycleDuration;

    // Affichage seulement si une seconde complete s'est ecoulee
    const auto now = std::chrono::steady_clock::now();
    if (now - _lastReportTime >= std::chrono::seconds(1)) {
        report();
        _cycleCount = 0;
        _accumulatedDuration = std::chrono::steady_clock::duration{0};
        _lastReportTime = now;
    }
}

void CycleStats::report() const {
    // Recuperation de la date et l'heure courantes
    const auto nowC = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(nowC);

    // Calcul de la duree moyenne d'un cycle sur la derniere seconde
    double avgMs = 0.0;
    if (_cycleCount > 0) {
        avgMs = std::chrono::duration<double, std::milli>(_accumulatedDuration).count() / _cycleCount;
    }

    std::cout << std::put_time(std::localtime(&nowTime), "%Y-%m-%d %H:%M:%S") << " | "
              << "Cycles/s: " << _cycleCount << " | "
              << "Duree moyenne d'un cycle: " << std::fixed << std::setprecision(2) << avgMs << " ms"
              << std::endl;
}

}  // namespace Client
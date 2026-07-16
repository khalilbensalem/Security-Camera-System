/**
 * @file CycleStats.h
 * @brief Comptabilise les cycles du client et affiche des statistiques
 *        au moins une fois par seconde dans le terminal.
 */
#pragma once

#include <chrono>

namespace Client {

/**
 * @class CycleStats
 * @brief Accumule les durees de cycle et journalise un resume chaque seconde
 *        (date/heure, cycles par seconde, duree moyenne d'un cycle).
 */
class CycleStats {
public:
  /**
   * @brief Enregistre un nouveau cycle et affiche un resume si une seconde
   *        s'est ecoulee depuis le dernier affichage.
   * @param cycleDuration Duree du cycle qui vient de se terminer.
   */
  void recordCycle(std::chrono::steady_clock::duration cycleDuration);

private:
  void report() const;

  int _cycleCount = 0;
  std::chrono::steady_clock::duration _accumulatedDuration{0};
  std::chrono::steady_clock::time_point _lastReportTime =
      std::chrono::steady_clock::now();
};

} // namespace Client
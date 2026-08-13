/**
 * @file Protocol.h
 * @brief Constantes et codes de message du protocole TCP/IP.
 */
#pragma once

#include <chrono>
#include <cstdint>

namespace Protocol {

/// Port TCP/IP utilise par le client et le serveur
constexpr uint16_t PORT = 4099;

/// Intervalle entre deux cycles GET_FRAME, cote client
constexpr int CYCLE_INTERVAL_MS = 30;

/// Delai maximal tolere pour la reception d'un message TCP/IP
constexpr int RECV_TIMEOUT_MS = 60;

/// Duree pendant laquelle aucune reponse du serveur a GET_FRAME n'est
/// toleree avant de declarer la connexion perdue. Superieure au
/// timeout de reception (RECV_TIMEOUT_MS) pour absorber la gigue normale du
/// reseau sans declencher de fausses alertes.
constexpr int CONNECTION_LOST_TIMEOUT_MS = 300;

/// Codes de message echanges entre le client et le serveur
enum class MessageCode : uint8_t {
  GetFrame = 1,      ///< Client -> Serveur : demande la capture d'une image
  Stop = 2,          ///< Client -> Serveur : demande d'arret du programme
  FrameHdr = 101,    ///< Serveur -> Client : en-tete d'une image transmise
  StopAck = 102,     ///< Serveur -> Client : confirmation d'arret du programme
  ButtonPress = 103, ///< Serveur -> Client : appui detecte sur le bouton
  NoLight = 201,     ///< Serveur -> Client : luminosite insuffisante, aucune
                     ///< image transmise
  SensorError = 202, ///< Serveur -> Client : incoherence capteur/image,
                     ///< aucune image transmise
};

} // namespace Protocol
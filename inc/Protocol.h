/**
 * @file Protocol.h
 * @brief Constantes et codes de message du protocole TCP/IP.
 */
#pragma once

#include <cstdint>

namespace Protocol {

/// Port TCP/IP utilise par le client et le serveur.
constexpr uint16_t PORT = 4099;

/// Intervalle entre deux cycles GET_FRAME, cote client.
constexpr int CYCLE_INTERVAL_MS = 30;

/// Delai maximal tolere pour la reception d'un message TCP/IP.
constexpr int RECV_TIMEOUT_MS = 60;

/// Codes de message echanges entre le client et le serveur.
enum class MessageCode : uint8_t {
  GetFrame = 1,    ///< Client -> Serveur : demande la capture d'une image.
  Stop = 2,        ///< Client -> Serveur : demande d'arret du programme.
  FrameHdr = 101,  ///< Serveur -> Client : en-tete d'une image transmise.
  StopAck = 102,   ///< Serveur -> Client : confirmation d'arret.
  ButtonPress = 103, ///< Serveur -> Client : appui detecte sur le bouton.
};

} // namespace Protocol
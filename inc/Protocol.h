/**
 * @file Protocol.h
 * @brief Constantes et codes de message du protocole TCP/IP.
 */
#pragma once

#include <cstdint>

namespace Protocol {

// Port TCP/IP utilise par le client et le serveur
constexpr uint16_t PORT = 4099;

// Intervalle entre deux cycles GET_FRAME, cote client
constexpr int CYCLE_INTERVAL_MS = 30;

// Delai maximal tolere pour la reception d'un message TCP/IP
constexpr int RECV_TIMEOUT_MS = 60;

// Codes de message echanges entre le client et le serveur
enum class MessageCode : uint8_t {
  GetFrame = 1,
  Stop = 2,
  FrameHdr = 101,
  StopAck = 102,
  ButtonPress = 103,
};

} // namespace Protocol
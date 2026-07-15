/**
 * @file Protocol.h
 * @brief Constantes et codes de message du protocole TCP/IP.
 */
#pragma once

#include <cstdint>

namespace Protocol {

// Port TCP/IP utilise par le client et le serveur
constexpr uint16_t PORT = 4099;

// Codes de message echanges entre le client et le serveur
enum class MessageCode : uint8_t {
    GetFrame = 1,
    Stop = 2,
    FrameHdr = 101,
    StopAck = 102,
};

}  // namespace Protocol
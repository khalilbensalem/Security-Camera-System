/**
 * @file Protocol.h
 * @brief Constantes et codes de message du protocole TCP/IP.
 */
#pragma once

#include <chrono>
#include <cstdint>

namespace Protocol {

/// Port TCP/IP utilise par le client et le serveur.
constexpr uint16_t PORT = 4099;

/// Intervalle entre deux cycles GET_FRAME, cote client.
constexpr int CYCLE_INTERVAL_MS = 30;

/// Delai maximal tolere pour la reception d'un message TCP/IP.
constexpr int RECV_TIMEOUT_MS = 60;

/// Delai maximal tolere pour l'envoi d'un message TCP/IP. Borne le temps de
/// blocage de send() si le pair devient injoignable (ex: cable Ethernet
/// debranche), situation qui ne genere ni FIN ni RST et ne serait sinon
/// jamais detectee par le mecanisme d'erreur habituel des sockets.
constexpr int SEND_TIMEOUT_MS = 500;

/// Duree d'inactivite (aucun message recu) au-dela de laquelle le client
/// courant est considere comme deconnecte, meme en l'absence de FIN/RST
/// (ex: perte physique du lien reseau). Grande devant le cycle client de
/// 30 ms pour tolerer la gigue normale, mais petite devant la contrainte de
/// reprise en moins de 10 s du Livrable 5.
constexpr std::chrono::milliseconds CLIENT_IDLE_TIMEOUT{1000};

/// Duree minimale pendant laquelle un nouvel etat (NO_LIGHT, SENSOR_ERROR,
/// FRAME_HDR) doit persister sans interruption avant d'etre transmis au
/// client ; evite les oscillations rapides autour d'un seuil de luminosite.
constexpr std::chrono::milliseconds STATE_DEBOUNCE{200};

/// Codes de message echanges entre le client et le serveur.
enum class MessageCode : uint8_t {
  GetFrame = 1,      ///< Client -> Serveur : demande la capture d'une image.
  Stop = 2,          ///< Client -> Serveur : demande d'arret du programme.
  FrameHdr = 101,    ///< Serveur -> Client : en-tete d'une image transmise.
  StopAck = 102,     ///< Serveur -> Client : confirmation d'arret.
  ButtonPress = 103, ///< Serveur -> Client : appui detecte sur le bouton.
  NoLight = 201,     ///< Serveur -> Client : luminosite ambiante insuffisante.
  SensorError = 202, ///< Serveur -> Client : incoherence capteur/image.
};

} // namespace Protocol
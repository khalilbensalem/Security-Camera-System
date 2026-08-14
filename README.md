# Security Camera System — Odroid-C2 ↔ Workstation

A client-server video surveillance system built for an [Odroid-C2](https://www.hardkernel.com/shop/odroid-c2/)
embedded board running an embedded Linux distribution built with the
[Yocto Project](https://www.yoctoproject.org/), communicating with a client
application over a custom TCP/IP protocol.

Built for ELE4205 (Operating Systems and Hardware Interfaces), Polytechnique
Montréal — Summer 2026.

## What it does

- Streams live JPEG-compressed video from a USB camera to a desktop client
  at ~30 fps over a lightweight, hand-rolled TCP/IP protocol.
- Reads a physical GPIO push-button on the embedded board; a press triggers
  an image save on the client side.
- Reads ambient light via a photoresistor over the Linux **IIO** subsystem
  (`sysfs`), cross-checking it against image brightness to detect
  inconsistencies (occluded camera, faulty sensor, etc.).
- **Survives real network failures**: automatic reconnection after a
  dropped Ethernet cable or a server restart, with a fail-fast recovery
  strategy that avoids TCP stream desynchronization (see below).
- Displays live diagnostics: cycle timing stats, elapsed time in error
  states, connection status.
- Starts automatically at board boot via a SysVinit service (this target
  doesn't run systemd).

## Repository layout

```
odroid/   → server: runs on the Odroid-C2, owns the camera, GPIO, and light sensor
poste/    → client: runs on the workstation, displays video, handles user input
```

Each side has its own README with build instructions and implementation
details — see [`odroid/README.md`](./odroid/README.md) and
[`poste/README.md`](./poste/README.md).

## Architecture

```
┌─────────────────┐      TCP/IP (port 4099)      ┌──────────────────┐
│   Odroid-C2      │◄─────────────────────────────►│   Workstation    │
│   (server)       │      30 ms request cycle      │   (client)       │
│                   │                                │                   │
│  USB Camera ──┐   │                                │  OpenCV display   │
│  GPIO button ─┤   │                                │  Image save on    │
│  Photoresistor┘   │                                │  button press     │
└─────────────────┘                                └──────────────────┘
```

- The client always initiates requests (`GET_FRAME`); the server responds.
- Messages are single-byte codes for control, with `uint32_t` fields
  (network byte order) for frame IDs and payload sizes.
- The server is a single-threaded, strictly synchronous request/response
  loop for networking, with **dedicated background threads** for the
  camera and the button — both I/O sources that can block unpredictably
  and must never stall the network cycle.

## The interesting part: building in robustness

The most substantial engineering work here wasn't the initial feature set —
it was making the system survive real-world failures without an identifier
field in the protocol to correlate requests and responses. That constraint
shaped every robustness decision:

- **No tolerance window for partial reads.** An early version tolerated a
  few failed reads before declaring the connection lost, to absorb network
  jitter. That's exactly what caused a nasty bug: a partial read left
  orphaned bytes in the TCP buffer, which the next cycle then misread as a
  new message header — cascading failures. The fix was to treat *any*
  read failure as fatal and immediately reconnect on a fresh socket,
  rather than trying to "catch up" on a socket that might be
  out of sync.
- **Non-blocking reconnection.** A blocking `connect()` on a dead link can
  take seconds to time out — incompatible with the required 500 ms retry
  cadence. The client uses a non-blocking socket with `select()` to bound
  each attempt.
- **A synchronous server has no way to preempt itself.** A `STOP` command
  sent while the server is mid-response to a `GET_FRAME` has to wait —
  the client retries the stop handshake rather than assuming a single
  60 ms window is enough.
- **Dedicated capture thread for the camera**, after discovering that
  `cv::VideoCapture::read()` has no timeout and can block for hundreds of
  milliseconds under low light (auto-exposure), plus that the USB camera
  physically re-enumerates during board boot — requiring the server to
  scan for the camera device dynamically rather than assume a fixed path.

## Build & run

See the per-component READMEs for toolchain setup (CMake, cross-compilation
for the Odroid-C2, OpenCV/libgpiod dependencies).
